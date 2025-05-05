#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <string.h>
#include <algorithm>
#include <queue>

#include <writer.h>
#include <scheduler.h>
#include <pipe.h>

scheduler* scheduler::declare_scheduler()
{
    scheduler* s = new scheduler();
    return s;
}

void scheduler::init_scheduler()
{
    // init the cores
    for (int i = 0; i < NUM_OF_CORES; i++)
    {
        core *c = new core(i, MAX_CORE_WEIGHT, false, 0);
        m_cores.push_back(c);
    }   
    
    // Set current time
    m_activationTime = time(NULL);
    m_log_timeout = time(NULL);

    // Specify the CPU core to run the scheduler on
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);    
    CPU_SET(SCHEDULER_CORE, &cpuset);

    if (sched_setaffinity(0, sizeof(cpuset), &cpuset) != 0) 
    {
        perror("sched_setaffinity");
        exit(EXIT_FAILURE);
    }
}

void scheduler::start_scheduler()
{
    while(active())
    {
        monitor_tasks();
        run_tasks();
        log_results();
    }

    printResults();
}

void scheduler::monitor_tasks()
{
    int status;
    pid_t result;
    
    unsigned long current_time = current_time_in_ms();

    priority_queue<task*, vector<task*>, CompareTask> task_queue;
    for (task* t : m_tasks) { task_queue.push(t); }

    //for (auto& task : m_tasks) 

    while (!task_queue.empty())
    {   
        task* task = task_queue.top();
        task_queue.pop();

        if (!task->offset_elapsed(m_activationTime, current_time))
            continue;
        
        if (task->get_state() == task_state::running) 
        {            
            result = waitpid(task->get_pid(), &status, WNOHANG);

            if (result == 0) 
            {                
                if (task->is_stuck(current_time, status, result)) 
                {
                    task->set_state(task_state::crashed);
                    handle_task_completion(task, 1, result);

                    continue;
                }
            } 
            else 
            {
                task->set_latest(status, result);
                handle_task_completion(task, status, result);

                continue;
            }

        }

        if (task->task_input_full(task) && task->get_state() != task_state::running && task->period_elapsed(current_time))
        {            
            task->set_state(task_state::fireable);
            int core_id;

            if (task->get_voter())
            {
                voter* v = static_cast<voter*>(task);

                v->get_voter_type() == voter_type::weighted ? core_id = find_core(true) : core_id = find_core(false);
            }
            else
            {
                core_id = find_core(false);
            }

            if (core_id != -1)
            {
                task->set_cpu_id(core_id);
            }
            else
            {
                task->set_state(task_state::idle);
            }
        }
    }
}

void scheduler::handle_task_completion(task *t, int status, pid_t result)
{
    auto &core = m_cores[t->get_cpu_id()];

    if (result == -1)
        t->set_state(task_state::idle);
    else if (WIFEXITED(status)) 
    {
        if (WEXITSTATUS(status) == 0) 
        {
            t->set_success(t->get_success() + 1);
            core->update_weight(MAX_CORE_WEIGHT / MAX_SCORE_BUFFER);

            if (core->get_weight() > MAX_CORE_WEIGHT)
                core->set_weight(MAX_CORE_WEIGHT);

            t->set_state(task_state::idle);
        } 
        else 
        {            
            (WEXITSTATUS(status) == 2) ? t->increment_errors() : t->increment_fails();
            
            core->update_weight(0);
            t->set_state(task_state::crashed);
        }
    } 
    else if (WIFSIGNALED(status)) 
    {
        t->increment_fails();
        core->update_weight(0);
        t->set_state(task_state::crashed);

    }
    
    core->increase_runs();
    core->set_active(false);

    t->incrementRuntime();
}

void scheduler::run_tasks()
{
    priority_queue<task*, vector<task*>, CompareTask> task_queue;
    for (task* t : m_tasks) { task_queue.push(t); }

    // Fork and set CPU affinity for each task
    while (!task_queue.empty())
    {   
        task* task = task_queue.top();
        task_queue.pop();
    
        if (task->get_state() == task_state::fireable) 
        {
            task->set_startTime(current_time_in_ms());     
            task->increment_runs();

            auto customStartTime = std::chrono::high_resolution_clock::now();
            task->setStartTime(customStartTime);        

            pid_t pid = fork();

            if (pid == -1)
                exit(EXIT_FAILURE);
            else if (pid == 0) 
            {
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                CPU_SET(task->get_cpu_id(), &cpuset);

                if (prctl(PR_SET_NAME, (unsigned long) task->get_name().c_str()) < 0)
                    perror("prctl()");

                if (sched_setaffinity(0, sizeof(cpuset), &cpuset) != 0) 
                {
                    perror("sched_setaffinity");
                    exit(EXIT_FAILURE);
                }

                task->run();

                exit(EXIT_SUCCESS);

            } 
            else 
            {
                task->set_pid(pid);
                task->set_state(task_state::running);                
                task->add_core_run(task->get_cpu_id());
            }
        }
    }
}

void scheduler::add_task(task *t)
{
    m_tasks.push_back(t);

    return;
}

void scheduler::add_task(voter *v)
{
    m_tasks.push_back(dynamic_cast<task*>(v));

    return;
}

void scheduler::cleanup_tasks()
{    
    for (size_t i = 0; i < m_tasks.size(); i++)
    {
        kill(m_tasks[i]->get_pid(), SIGTERM);
        waitpid(m_tasks[i]->get_pid(), NULL, 0);
    }

    for (task* t : m_tasks)
    {
        delete t;
    }

    for (core* c : m_cores)
    {
        delete c;
    }

    printf("Scheduler shutting down...\n");
}

int scheduler::find_core(bool isVoter)
{
    // Always skip the first core, this is used for the scheduler
    int core_id = -1;

    for (int i = 0; i < NUM_OF_CORES; i++)
    {
        if (i == SCHEDULER_CORE)
            continue;

        if (isVoter)
        {            
            // If core is inactive and is more reliable
            if (!m_cores[i]->get_active()) 
            {
                if (core_id == -1 || 
                    m_cores[i]->get_weight() > m_cores[core_id]->get_weight() || 
                    (m_cores[i]->get_weight() == m_cores[core_id]->get_weight() && m_cores[i]->get_runs() < m_cores[core_id]->get_runs()))
                {
                    core_id = i;
                }
            }
        }
        else
        {
            if (!m_cores[i]->get_active() && 
                (core_id == -1 || m_cores[i]->get_runs() < m_cores[core_id]->get_runs()))
            {
                core_id = i;
            }
        }
    }

    // If no available core found, return
    if (core_id == -1)
    {        
        return -1;
    }

    if (isVoter && m_cores[core_id]->get_weight() < 80)
    {
        printf("not reliable \n");
        
        return -1;
    }

    
    m_cores[core_id]->set_active(true);    

    return core_id;
}

bool scheduler::active()
{
    usleep(1000); // Prevents busy loop

#ifdef TIME_BASED
    time_t currentTime = time(NULL);

    // Convert current time and activation time to milliseconds
    long currentTimeMs = currentTime * 1000;
    long activationTimeMs = m_activationTime * 1000;

    cout << "\rCurrent time: " << currentTimeMs - activationTimeMs << " of " << MAX_RUN_TIME << "\t" << std::flush;

    if (currentTimeMs - activationTimeMs < MAX_RUN_TIME)
        return true;
    else
        return false;

#else    
    cout << "\rCurrent run: " << m_tasks[0]->get_runs() << " of " << MAX_ITERATIONS <<  "\t" << std::flush;

    if (m_tasks[0]->get_runs()  >= MAX_ITERATIONS)
        return false;
    else
        return true;
#endif
}

void scheduler::printResults()
{
    for (size_t i = 0; i < m_tasks.size(); i++)
    {
        int iterations = 0;
        iterations += m_tasks[i]->get_success();
        iterations += m_tasks[i]->get_fails();
        iterations += m_tasks[i]->get_errors();
    
        printf("Task: %s \t total runs: %d \t successful runs: %d \t failed runs: %d \t error runs: %d \t total task time: %lld \t average task time: %f \t", 
            m_tasks[i]->get_name().c_str(),
            iterations,
            m_tasks[i]->get_success(), 
            m_tasks[i]->get_fails(), 
            m_tasks[i]->get_errors(),
            m_tasks[i]->getRuntime(),
            static_cast<double>(m_tasks[i]->getRuntime()) / iterations);
    
        m_tasks[i]->print_core_runs();
    }

    for (int i = 0; i < NUM_OF_CORES && i != SCHEDULER_CORE; i++)
        printf("Core: %d \t runs: %d \t weight: %f \t state %d \n", m_cores[i]->get_coreID(), m_cores[i]->get_runs(), m_cores[i]->get_weight(), m_cores[i]->get_active());

    for (size_t i = 0; i < m_tasks.size(); i++)
        printf("Task: %s \t state: %d \t input full: %d \t latest result %d \t latest status %d \t Average runtime: %lld \n", 
        m_tasks[i]->get_name().c_str(), m_tasks[i]->get_state(), m_tasks[i]->task_input_full(m_tasks[i]), m_tasks[i]->get_latestResult(), m_tasks[i]->get_latestStatus(), m_tasks[i]->getRuntime());
}

void scheduler::log_results() {
#ifndef LOGGING
    return;
#endif

    long currentTimeMs = current_time_in_ms();

    if ((currentTimeMs - m_log_timeout > MAX_LOG_INTERVAL))
    {
        result r(m_tasks, m_cores, (currentTimeMs - (m_activationTime * 1000)));
        m_results.push_back(r);

        m_log_timeout = currentTimeMs;
    }
}

void scheduler::write_results_to_csv() 
{
#ifndef LOGGING
    return;
#endif

    string directoryName = generateOutputString(m_outputDirectory);

    directoryName = "results/" + directoryName;
    if (create_directory(directoryName))
        return;

    string core_results = directoryName + "/cores.tsv";
    string weight_results = directoryName + "/weights.tsv";
    string task_results = directoryName + "/tasks.tsv";
    string summary_results = directoryName + "/summary.txt";

    FILE *core_file = fopen(core_results.c_str(), "w");
    FILE *weight_file = fopen(weight_results.c_str(), "w");
    FILE *task_file = fopen(task_results.c_str(), "w");
    FILE *summary_file = fopen(summary_results.c_str(), "w");

    string parameterFile = directoryName + "/parameters.txt";
    create_parameter_file(parameterFile);

    if (!core_file || !weight_file || !task_file || !summary_file) 
    {
        perror("Failed to open file");
        return;
    }

    // Core banner
    for (size_t i = 0; i < m_cores.size(); i++)
    {
        if (!i) 
        {
            fprintf(core_file, "time\t");
            fprintf(weight_file, "time\t");
        }

        fprintf(core_file, "core_%ld", i);
        fprintf(weight_file, "core_%ld", i);

        if (i < m_cores.size() - 1) 
        {
            fprintf(core_file, "\t");
            fprintf(weight_file, "\t");
        }
    }

    // Task banner
    for (size_t i = 0; i < m_tasks.size(); i++)
    {
        if (!i)
            fprintf(task_file, "time\t");
        
        fprintf(task_file, "%s", m_tasks[i]->get_name().c_str());
        if (i < m_tasks.size() - 1)        
            fprintf(task_file, "\t");
    }

    fprintf(core_file, "\n");
    fprintf(weight_file, "\n");
    fprintf(task_file, "\n");

    for (auto& result : m_results)
    {
        fprintf(core_file, "%ld\t", result.m_time);
        fprintf(weight_file, "%ld\t", result.m_time);
        fprintf(task_file, "%ld\t", result.m_time);

        for (size_t j = 0; j < m_cores.size(); j++)
        {
            fprintf(core_file, "%d", (result.m_cores[j]));
            fprintf(weight_file, "%f", (result.m_weights[j]));
            
            if (j < m_cores.size() - 1) 
            {
                fprintf(core_file, "\t");
                fprintf(weight_file, "\t");
            }
        }

        for (size_t j = 0; j < m_tasks.size(); j++)
        {
            fprintf(task_file, "%d", result.m_tasks[j]);

            if (j < m_tasks.size() - 1)
                fprintf(task_file, "\t");
        }

        fprintf(core_file, "\n");
        fprintf(weight_file, "\n");
        fprintf(task_file, "\n");
    }


    for (size_t i = 0; i < m_tasks.size(); i++)
    {
        int iterations = 0;
        iterations += m_tasks[i]->get_success();
        iterations += m_tasks[i]->get_fails();
        iterations += m_tasks[i]->get_errors();
    
        fprintf(summary_file, "Task: %s \t total runs: %d \t successful runs: %d \t failed runs: %d \t error runs: %d \t total task time: %lld \t average task time: %.2f \t", 
            m_tasks[i]->get_name().c_str(),
            iterations,
            m_tasks[i]->get_success(),
            m_tasks[i]->get_fails(),
            m_tasks[i]->get_errors(),
            m_tasks[i]->getRuntime(),
            static_cast<double>(m_tasks[i]->getRuntime()) / iterations);
    
        fprintf(summary_file, "%s", m_tasks[i]->write_core_runs().c_str());
    }
    

    for (int i = 0; i < NUM_OF_CORES && i != SCHEDULER_CORE; i++)
        fprintf(summary_file, "Core: %d \t runs: %d \t weight: %f \t state %d \n", m_cores[i]->get_coreID(), m_cores[i]->get_runs(), m_cores[i]->get_weight(), m_cores[i]->get_active());

        
    for (size_t i = 0; i < m_tasks.size(); i++)
    {
        fprintf(summary_file, "Task: %s \t state: %d \t input full: %d \t latest result %d \t latest status %d \n", 
        m_tasks[i]->get_name().c_str(), 
        m_tasks[i]->get_state(), 
        m_tasks[i]->task_input_full(m_tasks[i]), 
        m_tasks[i]->get_latestResult(), 
        m_tasks[i]->get_latestStatus());
    }

    fclose(core_file);
    fclose(weight_file);
    fclose(task_file);
    fclose(summary_file);
}

void scheduler::create_parameter_file(string &path)
{
    FILE *injection_file = fopen(path.c_str(), "w");

    if (!injection_file)
        return;

    fprintf(injection_file, "*** Parameter file *** \n");
    fprintf(injection_file, "task busy time: %d \n", TASK_BUSY_TIME);
    fprintf(injection_file, "max read time: %d \n", MAX_READ_TIME);
#ifdef ITERATION_BASED
    fprintf(injection_file, "iterations: %d \n", MAX_ITERATIONS);
#elif
    fprintf(injection_file, "run time: %d \n", MAX_RUN_TIME);
#endif
    fprintf(injection_file, "cores: %d \n", NUM_OF_CORES);

    for (core* c : m_cores)
    {
        fprintf(injection_file, "\t core: %d \n", c->get_coreID());
    }

    fprintf(injection_file, "scheduler core: %d \n", SCHEDULER_CORE);
    fprintf(injection_file, "core buffer: %d \n", MAX_SCORE_BUFFER);
    fprintf(injection_file, "max stuck time: %d \n\n", MAX_STUCK_TIME);
    fprintf(injection_file, "Task descriptions: \n");
    fprintf(injection_file, "Name: \t Offset \t Period: \t Priority: \n");
    for (task* t : m_tasks)
    {
        fprintf(injection_file, "%s \t %ld \t %ld \t %d \n", t->get_name().c_str(), t->get_offset(), t->get_period(), t->get_priority());
    }


}

long scheduler::current_time_in_ms() 
{
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return (spec.tv_sec * 1000) + (spec.tv_nsec / 1000000);
}

string scheduler::generateOutputString(const string& prefix) 
{
    // Get the current time
    std::time_t now = std::time(nullptr);
    std::tm timeinfo = *std::localtime(&now);

    // Format the date and time
    std::ostringstream oss;
    oss << std::put_time(&timeinfo, "%Y-%m-%d_%H-%M-%S");
    string suffix = oss.str();
    
    string output = prefix + "_" + suffix + ".txt";

    return output;
}