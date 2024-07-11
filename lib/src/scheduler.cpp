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
#include "scheduler.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <string.h>
#include "pipe.h"
#include "defines.h"

void scheduler::init_scheduler()
{
    // init the cores
    for (int i = 0; i < NUM_OF_CORES; i++)
    {
        core *c = new core(i, MAX_CORE_WEIGHT, false, 0);
        m_cores.push_back(c);
    }   

    // Specify the CPU core to run the scheduler on
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);    
    CPU_SET(SCHEDULER_CORE, &cpuset);
    
    m_activationTime = time(NULL);
    m_log_timeout = time(NULL);

    if (sched_setaffinity(0, sizeof(cpuset), &cpuset) != 0) {
        perror("sched_setaffinity");
        exit(EXIT_FAILURE);
    }
}

// void scheduler::monitor_tasks()
// {
//     unsigned long current_time = current_time_in_ms();

//     for (int i = 0; i < m_tasks.size(); i++) {
//         auto &task = m_tasks[i];

//         // Handle task offset
//         if (!task->offset_elapsed(m_activationTime, current_time)) {
//             continue;
//         }

//         // Monitor active tasks
//         if (!task->get_fireable() && task->get_state() == task_state::running) {
//             int status;
//             pid_t result = waitpid(task->get_pid(), &status, WNOHANG);

//             task->set_latest(status, result);

//             if (result == 0) {
//                 if (task->is_stuck(current_time, status, result)) {
//                     task->increment_fails();
//                     m_cores[task->get_cpu_id()]->decrease_weight();
//                     task->set_state(task_state::crashed);
//                 } else {
//                     // Task is still running
//                     continue;
//                 }
//             } else if (result == -1) {
//                 // Handle waitpid error
//                 perror("waitpid");
//                 task->set_state(task_state::idle);
//             } else if (WIFEXITED(status)) {
//                 if (WEXITSTATUS(status) == 0) {
//                     task->set_success(task->get_success() + 1);
//                     auto &core = m_cores[task->get_cpu_id()];
//                     core->increase_weight();

//                     if (core->get_weight() > MAX_CORE_WEIGHT) {
//                         core->set_weight(MAX_CORE_WEIGHT);
//                     }

//                     task->set_state(task_state::idle);
//                 } else {
//                     task->increment_fails();
//                     m_cores[task->get_cpu_id()]->decrease_weight();
//                     task->set_state(task_state::crashed);
//                 }
//             } else if (WIFSIGNALED(status)) {
//                 m_cores[task->get_cpu_id()]->decrease_weight();
//                 task->increment_fails();
//                 task->set_state(task_state::crashed);
//             }

//             // Update task and core state
//             auto &core = m_cores[task->get_cpu_id()];
//             core->increase_runs();
//             core->set_active(false);            

//             // Ensure the task state is set to idle if it was running
//             if (task->get_state() == task_state::running) {
//                 task->set_state(task_state::idle);
//             }

//         }
//         // if (i == m_tasks.size() - 1){
//         //     for (auto core : m_cores)
//         //         core->get_active() ? printf("core %d:\t active \n", core->get_coreID()) : printf("core %d:\t idle \n", core->get_coreID());
//         // }

//                 // Only launch when input is full and/or the task is activated
//         if (task_input_full(task) && task->get_state() != task_state::running && task->period_elapsed(current_time)) {
//             task->set_fireable(true);
//             int core_id = find_core();

//             if (core_id != -1) { // Assuming valid core IDs are non-negative
//                 task->set_cpu_id(core_id);                
//                 task->add_core_run(core_id);
//             } else {
//                 // No cores available
//                 task->set_fireable(false);
//             }
//         } else {
//             task->set_fireable(false);
//         }
//     }


// }

void scheduler::monitor_tasks()
{
    unsigned long current_time = current_time_in_ms();

    for (int i = 0; i < m_tasks.size(); i++) {
        auto &task = m_tasks[i];

        // Handle task offset
        if (!task->offset_elapsed(m_activationTime, current_time)) {
            continue;
        }

        // Monitor active tasks
        if (task->get_state() == task_state::running) {
            int status;
            pid_t result = waitpid(task->get_pid(), &status, WNOHANG);

            if (result == 0) {
                // Task is still running, check if it's stuck
                if (task->is_stuck(current_time, status, result)) {
                    task->increment_fails();
                    m_cores[task->get_cpu_id()]->decrease_weight();
                    task->set_state(task_state::crashed);
                }
            } 
            else {
                // Task has finished or an error occurred
                task->set_latest(status, result);
                handle_task_completion(task, status, result);
            }
        }

        // Launch tasks if input is full and period has elapsed
        if (task_input_full(task) && task->get_state() != task_state::running && task->period_elapsed(current_time)) {
            task->set_fireable(true);
            int core_id;

            voter* v = static_cast<voter*>(task);
            !v ? core_id = find_core() : core_id = find_core(true);

            if (core_id != -1) {
                task->set_cpu_id(core_id);
                task->add_core_run(core_id);
            } 
            else {
                task->set_fireable(false);
            }
        }
        else {
            task->set_fireable(false);
        }
    }
}


void scheduler::handle_task_completion(task *t, int status, pid_t result)
{
    auto &core = m_cores[t->get_cpu_id()];

    if (result == -1) {
        // Handle waitpid error
        perror("waitpid");
        t->set_state(task_state::idle);
    } 
    else if (WIFEXITED(status)) 
    {
        if (WEXITSTATUS(status) == 0) {
            t->set_success(t->get_success() + 1);
            core->increase_weight();
            if (core->get_weight() > MAX_CORE_WEIGHT) {
                core->set_weight(MAX_CORE_WEIGHT);
            }
            t->set_state(task_state::idle);
        } 
        else {
            t->increment_fails();
            core->decrease_weight();
            t->set_state(task_state::crashed);
        }
    } 
    else if (WIFSIGNALED(status)) {
        t->increment_fails();
        core->decrease_weight();
        t->set_state(task_state::crashed);
    }

    core->increase_runs();
    core->set_active(false);
}

void scheduler::run_tasks()
{
    // Fork and set CPU affinity for each task
    for (int i = 0; i < m_tasks.size(); i++) {
        if (m_tasks[i]->get_fireable()) {
            m_tasks[i]->set_startTime(current_time_in_ms());     
            m_tasks[i]->increment_runs();        

            pid_t pid = fork();

            if (pid == -1) {
                exit(EXIT_FAILURE);
            } 
            else if (pid == 0) {
                // Set CPU affinity for the task
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                CPU_SET(m_tasks[i]->get_cpu_id(), &cpuset);

                if (prctl(PR_SET_NAME, (unsigned long) m_tasks[i]->get_name().c_str()) < 0) {
                    perror("prctl()");
                }

                if (sched_setaffinity(0, sizeof(cpuset), &cpuset) != 0) {
                    perror("sched_setaffinity");
                    exit(EXIT_FAILURE);
                }

                m_tasks[i]->run();

                exit(EXIT_SUCCESS);

            } 
            else {
                m_tasks[i]->set_pid(pid);
                m_tasks[i]->set_state(task_state::running);
            }
        }
    }
}

void scheduler::add_task(const string& name, int period, int offset, int priority, void (*function)(void))
{
    task* t = new task(name, period, offset, priority, function);

    m_tasks.push_back(t);

    return;
}

void scheduler::add_voter(const string& name, int period, int offset, int priority, void (*function)(void))
{
    voter* v = new voter(name, period, offset, priority, function);

    m_tasks.push_back(dynamic_cast<task*>(v));

    return;
}

void scheduler::cleanup_tasks()
{
    // Cleanup: kill all child processes
    for (int i = 0; i < NUM_OF_TASKS; i++)
    {
        kill(m_tasks[i]->get_pid(), SIGTERM);
        waitpid(m_tasks[i]->get_pid(), NULL, 0); // Ensure they are terminated
    }

    printf("Scheduler shutting down...\n");
}

task* scheduler::find_task(string name)
{
    for (auto& t : m_tasks)
    {
        // if equal
        if (!t->get_name().compare(name))
            return t;
    }

    return NULL;
}

int scheduler::find_core(bool isVoter)
{
    // Always skip the first core, this is used for the scheduler
    int core_id = -1;

    for (int i = 0; i < NUM_OF_CORES && i != SCHEDULER_CORE; i++)
    {
        if (isVoter)
        {
            // If core is inactive and is more reliable
            if (!m_cores[i]->get_active()) {
                if (core_id == -1 || 
                    m_cores[i]->get_weight() > m_cores[core_id]->get_weight() || 
                    (m_cores[i]->get_weight() == m_cores[core_id]->get_weight() && m_cores[i]->get_runs() < m_cores[core_id]->get_runs())) {
                    core_id = i;
                }
            }
        }
        else
        {
            if (!m_cores[i]->get_active() && (core_id == -1 || m_cores[i]->get_runs() < m_cores[core_id]->get_runs()))
                core_id = i;
        }
    }

    if (core_id == -1)
        return -1;  // No free core found    

    // Change the core state
    m_cores[core_id]->set_active(true);    

    return core_id;
}

bool scheduler::active()
{
#ifdef TIME_BASED
    time_t currentTime = time(NULL);

    // Convert current time and activation time to milliseconds
    long currentTimeMs = currentTime * 1000;
    long activationTimeMs = m_activationTime * 1000;

    if (currentTimeMs - activationTimeMs < MAX_RUN_TIME)
        return true;
    else
        return false;

#else    

#ifdef RUN_LOG
    if (m_runs != m_tasks[0]->get_runs())
    {
        printf("run: %d \n", m_tasks[0]->get_runs());
    }
    m_runs = m_tasks[0]->get_runs();
    
#endif
    if (m_tasks[0]->get_success() + m_tasks[0]->get_fails() >= MAX_ITERATIONS)
        return false;
    else
        return true;
#endif
}

void scheduler::printResults()
{
    for (int i = 0; i < NUM_OF_TASKS; i++)
    {
        printf("Task: %s \t succesfull runs: %d \t failed runs: %d \t", m_tasks[i]->get_name().c_str(), m_tasks[i]->get_success(), m_tasks[i]->get_fails());
        m_tasks[i]->print_core_runs();
    }

    for (int i = 0; i < NUM_OF_CORES && i != SCHEDULER_CORE; i++)
    {
        printf("Core: %d \t runs: %d \t weight: %f \t state %d \n", m_cores[i]->get_coreID(), m_cores[i]->get_runs(), m_cores[i]->get_weight(), m_cores[i]->get_active());
    }

    for (int i = 0; i < NUM_OF_TASKS; i++)
    {
        printf("Task: %s \t state: %d \t fireable: %d \t input full: %d \t latest result %d \t latest status %d \n", 
        m_tasks[i]->get_name().c_str(), m_tasks[i]->get_state(), m_tasks[i]->get_fireable(), task_input_full(m_tasks[i]), m_tasks[i]->get_latestResult(), m_tasks[i]->get_latestStatus());
    }
}

result::result(vector<task*> tasks, vector<core*> cores, long mSeconds)
{
    for (long unsigned int i = 0; i < cores.size(); i++)
    {
        m_cores.push_back(cores[i]->get_runs());
    }

    for (long unsigned int i = 0; i < cores.size(); i++)
    {
        m_weights.push_back(cores[i]->get_weight());
    }

    for (long unsigned int i = 0; i < tasks.size(); i++)
    {
        m_tasks.push_back(tasks[i]->get_success());
    }

    m_time = mSeconds;
}

void scheduler::log_results() {
    long currentTimeMs = current_time_in_ms();

    if ((currentTimeMs - m_log_timeout > MAX_LOG_INTERVAL)) {
        result *r = new result(m_tasks, m_cores, (currentTimeMs - (m_activationTime * 1000)));
        m_results.push_back(r);

        m_log_timeout = currentTimeMs;
    }
}

void scheduler::write_results_to_csv() 
{
    string core_results = generateOutputString("results/cores/cores");
    string weight_results = generateOutputString("results/weights/weights");
    string task_results = generateOutputString("results/tasks/tasks");

    FILE *core_file = fopen(core_results.c_str(), "w");
    FILE *weight_file = fopen(weight_results.c_str(), "w");
    FILE *task_file = fopen(task_results.c_str(), "w");

    if (!core_file || !weight_file || !task_file) 
    {
        perror("Failed to open file");
        return;
    }

    for (long unsigned int i = 0; i < m_cores.size(); i++)
    {
        if (!i) {
            fprintf(core_file, "time\t");
            fprintf(weight_file, "time\t");
        }

        fprintf(core_file, "core_%ld", i);
        fprintf(weight_file, "core_%ld", i);

        if (i < m_cores.size() - 1) {
            fprintf(core_file, "\t");
            fprintf(weight_file, "\t");
        }
    }

    for (long unsigned int i = 0; i < m_tasks.size(); i++)
    {
        if (!i) {
            fprintf(task_file, "time\t");
        }
        
        fprintf(task_file, "%s", m_tasks[i]->get_name().c_str());
        if (i < m_tasks.size() - 1) {
            fprintf(task_file, "\t");
        }
    }

    fprintf(core_file, "\n");
    fprintf(weight_file, "\n");
    fprintf(task_file, "\n");

    for (auto& result : m_results)
    {
        fprintf(core_file, "%ld\t", result->m_time);
        fprintf(weight_file, "%ld\t", result->m_time);
        fprintf(task_file, "%ld\t", result->m_time);

        for (long unsigned int j = 0; j < m_cores.size(); j++)
        {
            fprintf(core_file, "%d", (result->m_cores[j]));
            fprintf(weight_file, "%f", (result->m_weights[j]));
            
            if (j < m_cores.size() - 1) 
            {
                fprintf(core_file, "\t");
                fprintf(weight_file, "\t");
            }
        }

        for (long unsigned int j = 0; j < m_tasks.size(); j++)
        {
            fprintf(task_file, "%d", result->m_tasks[j]);

            if (j < m_tasks.size() - 1)
            {
                fprintf(task_file, "\t");
            }
        }

        fprintf(core_file, "\n");
        fprintf(weight_file, "\n");
        fprintf(task_file, "\n");
    }

    fclose(core_file);
    fclose(weight_file);
    fclose(task_file);
}

long current_time_in_ms() {
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return (spec.tv_sec * 1000) + (spec.tv_nsec / 1000000);
}

core::core(int id, float weight, bool active, int runs)
{
    m_coreID = id;
    m_weight = weight;
    m_active = active;
    m_runs = runs;
}

string generateOutputString(const string& prefix) {
    // Get the current time
    std::time_t now = std::time(nullptr);
    std::tm timeinfo = *std::localtime(&now);

    // Format the date and time
    std::ostringstream oss;
    oss << std::put_time(&timeinfo, "%Y-%m-%d_%H-%M-%S");
    string suffix = oss.str();

    // Construct the output string
    string output = prefix + "_" + suffix + ".txt";

    return output;
}