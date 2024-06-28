#include <signal.h>
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

void handle_signal(int sig) {
    if (sig == SIGINT) {
        printf("Scheduler received SIGINT, shutting down...\n");
        exit(EXIT_SUCCESS);
    }
}

void scheduler::init_scheduler()
{
    // init the cores
    for (int i = 0; i < NUM_OF_CORES; i++)
    {
        core *c = new core(i, MAX_CORE_WEIGHT, false, 0);
        m_cores.push_back(c);
    }

    // exit handler function
    signal(SIGINT, handle_signal);

    // Specify the CPU core to run the scheduler on
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    CPU_SET(0, &cpuset);
    
    m_activationTime = time(NULL);
    m_log_timeout = time(NULL);

    if (sched_setaffinity(0, sizeof(cpuset), &cpuset) != 0) {
        perror("sched_setaffinity");
        exit(EXIT_FAILURE);
    }
}

void scheduler::run_tasks()
{
#ifdef DEBUG_SCHEDULER
        //printf("******* Run loop *******\n");
#endif

    // Fork and set CPU affinity for each task
    for (int i = 0; i < NUM_OF_TASKS && m_tasks[i]->get_fireable() == true; i++) {
        m_tasks[i]->set_cpu_id(find_core());

#ifdef DEBUG_SCHEDULER
        printf("Task: %s \n", m_tasks[i]->get_name().c_str());
#endif

        pid_t pid = fork();


        if (pid == -1) {
            printf("error \n");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Set CPU affinity for the task
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(m_tasks[i]->get_cpu_id(), &cpuset);
            CPU_SET(m_tasks[i]->get_cpu_id(), &cpuset);

            if (prctl(PR_SET_NAME, (unsigned long) m_tasks[i]->get_name().c_str()) < 0) {
                perror("prctl()");
            }

            if (sched_setaffinity(0, sizeof(cpuset), &cpuset) != 0) {
                perror("sched_setaffinity");
                exit(EXIT_FAILURE);
            }

            m_tasks[i]->m_function();

            exit(EXIT_SUCCESS);

        } else {
            m_tasks[i]->set_pid(pid);
            m_tasks[i]->set_active(true);
            m_cores[m_tasks[i]->get_cpu_id()]->set_active(true);
        }
    }
#ifdef DEBUG_SCHEDULER
    printf("\n");
#endif
}

void scheduler::monitor_tasks()
{
#ifdef DEBUG_SCHEDULER
    printf("******* Monitor loop *******\n");
#endif

    for (int i = 0; i < NUM_OF_TASKS; i++) {
        // Only launch when input is full and/or the task is activated
        if (task_input_full(m_tasks[i]) && !m_tasks[i]->get_active()) {
            m_tasks[i]->set_fireable(true);
        }
        else {
            m_tasks[i]->set_fireable(false);
        }

        // Monitor active tasks
        if (!m_tasks[i]->get_fireable() && m_tasks[i]->get_active()) {
            
#ifdef DEBUG_SCHEDULER
            printf("Task: %s \n", m_tasks[i]->get_name().c_str());
#endif
            int status;
            pid_t result = waitpid(m_tasks[i]->get_pid(), &status, WNOHANG);

            // Task has finished
            if (result == 0) {
                continue;
            }
            else if (WIFEXITED(status)) {
                // Task ended successfull
                if (WEXITSTATUS(status) == 0) {
                    // Increase the core reliability after a successfull run
                    m_tasks[i]->set_success(m_tasks[i]->get_success() + 1);
                    m_cores[m_tasks[i]->get_cpu_id()]->increase_weight();                    

                    // Prevent runaway core weight
                    if (m_cores[m_tasks[i]->get_cpu_id()]->get_weight() > MAX_CORE_WEIGHT) {
                        m_cores[m_tasks[i]->get_cpu_id()]->set_weight(100);
                    }

                } else {
                    // Decrease reliability after a process returned with an non-zero exit code
                    m_tasks[i]->increment_fails();
                    m_cores[m_tasks[i]->get_cpu_id()]->decrease_weight();
                }

            }
            else if (WIFSIGNALED(status)) {
                // Decrease reliability after a process crash on a core
                m_cores[m_tasks[i]->get_cpu_id()]->decrease_weight();
                m_tasks[i]->increment_fails();
            }

            // Update task and core state
            m_tasks[i]->set_active(false);
            m_cores[m_tasks[i]->get_cpu_id()]->increase_runs();
            m_cores[m_tasks[i]->get_cpu_id()]->set_active(false);

            // TODO: Ugly fix to distinguish between normal and replicate task
            if (m_tasks[i]->get_replicate()) {
                m_tasks[i]->set_finished(true);
            }
        }
    }

#ifdef NMR
    // TODO: Ugly fix to check if all replicates have finished
    int counter = 0;
    for (int i = 0; i < NUM_OF_TASKS; i++) {
        if (m_tasks[i]->get_replicate() && m_tasks[i]->get_finished()) {
            counter++;
        }
    }

    if (counter == 3) {
        m_tasks[m_voter].set_fireable(true);
    }
    else {
        m_tasks[m_voter].set_fireable(false);
    }
#endif
}

void scheduler::add_task(int id, const string& name, int period, void (*function)(void))
{
    //task t(name, period, function);

    //task* t = new task();
    task* t = new task(name, period, function);

    m_tasks.push_back(t);

    // m_tasks[id].set_name(name);
    // m_tasks[id].set_voter(false);
    // m_tasks[id].m_function = function;
    // m_tasks[id].set_inputs(NULL);
    // m_tasks[id].set_fails(0);
    // m_tasks[id].set_success(0);
    // m_tasks[id].set_active(false);
    // m_tasks[id].set_replicate(false);
    // m_tasks[id].set_finished(false);

    // if (period) {
    //     m_tasks[id].set_fireable(true);
    // }
    // else {
    //     m_tasks[id].set_fireable(false);
    // }

    return;
}

void scheduler::cleanup_tasks()
{
    // Cleanup: kill all child processes
    for (int i = 0; i < NUM_OF_TASKS; i++)
    {
        kill(m_tasks[i]->get_pid(), SIGTERM);
        waitpid(m_tasks[i]->get_pid(), NULL, 0); // Ensure they are terminated

        // free all related pointers
        //free(m_tasks[i]->get_name().c_str());
    }

    printf("Scheduler shutting down...\n");
}

int scheduler::find_core()
{
    // Always skip the first core, this is used for the scheduler
    int core_id = 1;

    for (int i = 1; i < NUM_OF_CORES; i++)
    {
#ifdef RELIABILITY_SCHEDULING
        // If core is inactive and is more reliable
        if (!m_cores[i]->get_active() && m_cores[i]->get_weight() > m_cores[core_id]->get_weight()) {
            core_id = i;
        }
        else if (!m_cores[i]->get_active() && m_cores[i]->get_weight() == m_cores[core_id]->get_weight()) {
            // if the weight is the same, but the core has more runs, balance the load
            if (m_cores[i]->get_runs() < m_cores[core_id]->get_runs()) {
                core_id = i;
            }
        }
#else
        if (!m_cores[i].get_active() && m_cores[i].get_runs() < m_cores[core_id].get_runs()) {
            core_id = i;
        }
#endif

    }

    // Change the core state
    m_cores[core_id]->set_active(true);

    return core_id;
}

bool scheduler::active()
{
    time_t currentTime = time(NULL);

    // Convert current time and activation time to milliseconds
    long currentTimeMs = currentTime * 1000;
    long activationTimeMs = m_activationTime * 1000;

    if (currentTimeMs - activationTimeMs < MAX_RUN_TIME)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void scheduler::printResults()
{
    for (int i = 0; i < NUM_OF_TASKS; i++)
    {
        printf("Task: %s \t succesfull runs: %d \t failed runs: %d \n", m_tasks[i]->get_name().c_str(), m_tasks[i]->get_success(), m_tasks[i]->get_fails());
    }

    for (int i = 1; i < NUM_OF_CORES; i++)
    {
        printf("Core: %d \t runs: %d \t weight: %f \n", m_cores[i]->get_coreID(), m_cores[i]->get_runs(), m_cores[i]->get_weight());
    }
}

result::result(vector<task*> tasks, vector<core*> cores, long mSeconds)
{
    for (int i = 0; i < cores.size(); i++)
    {
        m_cores.push_back(cores[i]->get_runs());
        //m_cores.push_back(tasks[i]->get_cpu_id());
    }

    for (int i = 0; i < cores.size(); i++)
    {
        m_weights.push_back(cores[i]->get_weight());
    }

    m_time = mSeconds;
}

void scheduler::log_results() {
    long currentTimeMs = current_time_in_ms();

    if ((currentTimeMs - m_log_timeout > MAX_LOG_INTERVAL)) {
        result *r = new result(m_tasks, m_cores, currentTimeMs);
        m_results.push_back(r);

        m_log_timeout = currentTimeMs;
    }
}

void scheduler::write_results_to_csv() {
    string task_results = generateOutputString("tasks");
    string weight_results = generateOutputString("weights");

    FILE *task_file = fopen(task_results.c_str(), "w");
    if (!task_file) {
        perror("Failed to open file");
        return;
    }

    int task_result_buffer[m_cores.size()];
    for (int i = 0; i < m_cores.size(); i++)
    {     
        task_result_buffer[i] = 0;

        fprintf(task_file, "core_%d", i);
        if (i < m_cores.size() - 1) {
            fprintf(task_file, "\t");
        }
    }

    fprintf(task_file, "\n");

    for (auto& result : m_results)
    {
        for (int j = 0; j < m_cores.size(); j++)
        {
            //fprintf(task_file, "%d", (m_cores[j]->get_runs() - task_result_buffer[j]));
            //fprintf(task_file, "%d", (m_cores[j]->get_runs()));
            fprintf(task_file, "%d", (result->m_cores[j]));

            task_result_buffer[j] = m_cores[j]->get_runs();

            if (j < m_cores.size() - 1) {
                fprintf(task_file, "\t");
            }
        }
        fprintf(task_file, "\n");
    }


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