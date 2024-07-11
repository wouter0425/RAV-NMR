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

/**
 * @brief Initializes the scheduler by setting up cores and CPU affinity.
 *
 * This function performs the following steps:
 * - Initializes the cores by creating `NUM_OF_CORES` core objects with initial parameters and adds them to the `m_cores` list.
 * - Sets the activation time and log timeout to the current time.
 * - Sets the CPU affinity to ensure the scheduler runs on a specific core (`SCHEDULER_CORE`).
 *
 * If setting the CPU affinity fails, the function prints an error message and exits the program.
 */
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

/**
 * @brief Monitors and manages the state of all tasks in the scheduler.
 * 
 * This function monitors the execution of all tasks in the scheduler, checking their status and handling 
 * task completion. It also determines when new tasks should be launched based on their input and period.
 * 
 * The function does the following:
 * - Retrieves the current time.
 * - Iterates through all tasks to monitor their state.
 * - For each task, it checks if the task's startup offset has elapsed. If not, it skips the task.
 * - For tasks that are currently running, it checks the state of the child process using `waitpid`.
 *   - If the task is still running (`result == 0`), it checks if the task is stuck. If it is, it marks the 
 *     task as crashed, increments the failure count, and decreases the core's weight.
 *   - If the task has finished or an error occurred, it sets the latest status and result for the task, and 
 *     calls `handle_task_completion` to process the task's completion.
 * - If the task is not running and its input is full, and the period has elapsed, it prepares the task for 
 *   launching:
 *   - It sets the task to fireable and attempts to find a core to run the task on.
 *   - If a core is found, it assigns the core ID to the task. Otherwise, it marks the task as not fireable.
 */
void scheduler::monitor_tasks()
{
    int status;
    pid_t result;
    
    unsigned long current_time = current_time_in_ms();

    for (int i = 0; i < m_tasks.size(); i++) 
    {
        auto &task = m_tasks[i];
        
        if (!task->offset_elapsed(m_activationTime, current_time))
            continue;
        
        if (task->get_state() == task_state::running) 
        {            
            result = waitpid(task->get_pid(), &status, WNOHANG);

            if (result == 0) 
            {                
                if (task->is_stuck(current_time, status, result)) 
                {
                    task->increment_fails();
                    m_cores[task->get_cpu_id()]->decrease_weight();
                    task->set_state(task_state::crashed);
                }
            } 
            else 
            {                
                task->set_latest(status, result);
                handle_task_completion(task, status, result);
            }
        }

        if (task_input_full(task) && task->get_state() != task_state::running && task->period_elapsed(current_time))
        {
            task->set_fireable(true);
            int core_id;

#ifdef NMR
            // The voter gets the most reliable core
            voter* v = static_cast<voter*>(task);
            !v ? core_id = find_core() : core_id = find_core(true);
#endif            
            (core_id != -1) ? task->set_cpu_id(core_id) : task->set_fireable(false);
        }
        else
        {
            task->set_fireable(false);
        }
    }
}

/**
 * @brief Handles the completion of a task and updates its state and associated core metrics.
 *
 * @param t Pointer to the task that has completed.
 * @param status The status code returned by the task's process upon completion.
 * @param result The result of the waitpid function call used to check the task's status.
 * 
 * This function processes the completion status of a given task and updates the task's state 
 * and the associated core's metrics accordingly. It handles different scenarios based on the 
 * result and status of the task:
 * - If the result is -1, it sets the task's state to idle.
 * - If the task exited normally (WIFEXITED), it checks the exit status:
 *   - If the exit status is 0, the task is considered successful, and the core's weight is increased.
 *   - If the exit status is non-zero, it increments the task's failure count, decreases the core's weight, and sets the task's state to crashed.
 * - If the task was terminated by a signal (WIFSIGNALED), it increments the task's failure count, decreases the core's weight, and sets the task's state to crashed.
 * 
 * Finally, it increments the number of runs for the core and marks the core as inactive.
 */
void scheduler::handle_task_completion(task *t, int status, pid_t result)
{
    auto &core = m_cores[t->get_cpu_id()];

    if (result == -1) 
    {
        t->set_state(task_state::idle);
    } 
    else if (WIFEXITED(status)) 
    {
        if (WEXITSTATUS(status) == 0) 
        {
            t->set_success(t->get_success() + 1);
            core->increase_weight();

            if (core->get_weight() > MAX_CORE_WEIGHT)
                core->set_weight(MAX_CORE_WEIGHT);

            t->set_state(task_state::idle);
        } 
        else 
        {
            t->increment_fails();
            core->decrease_weight();
            t->set_state(task_state::crashed);
        }
    } 
    else if (WIFSIGNALED(status)) 
    {
        t->increment_fails();
        core->decrease_weight();
        t->set_state(task_state::crashed);
    }

    core->increase_runs();
    core->set_active(false);
}

/**
 * @brief Runs fireable tasks by forking processes and setting their CPU affinity.
 *
 * This function iterates through all tasks and performs the following steps for each fireable task:
 * - Sets the start time and increments the run count.
 * - Forks a new process for the task.
 * - In the child process, sets the CPU affinity for the task and runs the task.
 * - In the parent process, sets the task's PID, state to running, and records the core run.
 *
 * If forking fails, the function exits the program.
 */
void scheduler::run_tasks()
{
    // Fork and set CPU affinity for each task
    for (int i = 0; i < m_tasks.size(); i++) 
    {
        if (m_tasks[i]->get_fireable()) 
        {
            m_tasks[i]->set_startTime(current_time_in_ms());     
            m_tasks[i]->increment_runs();        

            pid_t pid = fork();

            if (pid == -1) 
            {
                exit(EXIT_FAILURE);
            } 
            else if (pid == 0) 
            {
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                CPU_SET(m_tasks[i]->get_cpu_id(), &cpuset);

                if (prctl(PR_SET_NAME, (unsigned long) m_tasks[i]->get_name().c_str()) < 0)
                    perror("prctl()");

                if (sched_setaffinity(0, sizeof(cpuset), &cpuset) != 0) 
                {
                    perror("sched_setaffinity");
                    exit(EXIT_FAILURE);
                }

                m_tasks[i]->run();

                exit(EXIT_SUCCESS);

            } 
            else 
            {
                m_tasks[i]->set_pid(pid);
                m_tasks[i]->set_state(task_state::running);                
                m_tasks[i]->add_core_run(m_tasks[i]->get_cpu_id());
            }
        }
    }
}

/**
 * @brief Adds a new task to the scheduler.
 *
 * @param name The name of the task.
 * @param period The period of the task.
 * @param offset The offset of the task.
 * @param priority The priority of the task.
 * @param function The function to be executed by the task.
 *
 * This function creates a new task object with the specified parameters and adds it to the `m_tasks` list.
 */
void scheduler::add_task(const string& name, int period, int offset, int priority, void (*function)(void))
{
    task* t = new task(name, period, offset, priority, function);

    m_tasks.push_back(t);

    return;
}

/**
 * @brief Adds a new voter task to the scheduler.
 *
 * @param name The name of the voter.
 * @param period The period of the voter.
 * @param offset The offset of the voter.
 * @param priority The priority of the voter.
 * @param function The function to be executed by the voter.
 *
 * This function creates a new voter object with the specified parameters, casts it to a task object, 
 * and adds it to the `m_tasks` list.
 */
void scheduler::add_voter(const string& name, int period, int offset, int priority, void (*function)(void))
{
    voter* v = new voter(name, period, offset, priority, function);

    m_tasks.push_back(dynamic_cast<task*>(v));

    return;
}

/**
 * @brief Cleans up all tasks by terminating their processes.
 *
 * This function iterates through all tasks, sends a SIGTERM signal to terminate each task's process,
 * and waits for the process to ensure they are terminated.
 * It then prints a message indicating that the scheduler is shutting down.
 */
void scheduler::cleanup_tasks()
{    
    for (int i = 0; i < NUM_OF_TASKS; i++)
    {
        kill(m_tasks[i]->get_pid(), SIGTERM);
        waitpid(m_tasks[i]->get_pid(), NULL, 0);
    }

    printf("Scheduler shutting down...\n");
}

/**
 * @brief Finds a task by its name.
 *
 * @param name The name of the task to find.
 * @return A pointer to the task if found, otherwise NULL.
 *
 * This function iterates through the `m_tasks` list and compares each task's name with the specified name.
 * If a matching task is found, it returns a pointer to the task. Otherwise, it returns NULL.
 */
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

/**
 * @brief Finds a free core for a task to run on.
 *
 * @param isVoter Indicates whether the task is a voter.
 * @return The ID of the free core if found, otherwise -1.
 *
 * This function iterates through the cores (excluding the scheduler core) to find a free core for the task.
 * - If `isVoter` is true, it finds the most reliable inactive core based on weight and run count.
 * - If `isVoter` is false, it finds any inactive core with the fewest runs.
 *
 * If no free core is found, it returns -1. Otherwise, it marks the found core as active and returns its ID.
 */
int scheduler::find_core(bool isVoter)
{
    // Always skip the first core, this is used for the scheduler
    int core_id = -1;

    for (int i = 0; i < NUM_OF_CORES && i != SCHEDULER_CORE; i++)
    {
        if (isVoter)
        {
            // If core is inactive and is more reliable
            if (!m_cores[i]->get_active()) 
            {
                if (core_id == -1 || m_cores[i]->get_weight() > m_cores[core_id]->get_weight() || (m_cores[i]->get_weight() == m_cores[core_id]->get_weight() && m_cores[i]->get_runs() < m_cores[core_id]->get_runs()))
                    core_id = i;
            }
        }
        else
        {
            if (!m_cores[i]->get_active() && (core_id == -1 || m_cores[i]->get_runs() < m_cores[core_id]->get_runs()))
                core_id = i;
        }
    }

    if (core_id == -1)
        return -1;
    
    m_cores[core_id]->set_active(true);    

    return core_id;
}

/**
 * @brief Checks if the scheduler is active based on the run time or task iterations.
 *
 * @return True if the scheduler is active, otherwise false.
 *
 * The function determines if the scheduler should continue running based on the following conditions:
 * - If `TIME_BASED` is defined, it checks if the current time minus the activation time is less than `MAX_RUN_TIME`.
 * - Otherwise, it checks if the total number of successes and failures for the first task is less than `MAX_ITERATIONS`.
 */
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
        printf("run: %d \n", m_tasks[0]->get_runs());
    
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