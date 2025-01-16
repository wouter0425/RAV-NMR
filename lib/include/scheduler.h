/**
 * @file scheduler.h
 * @brief This file contains functions for initializing, monitoring, and managing tasks and cores in a scheduler.
 * 
 * The file includes functions for initializing the scheduler, monitoring tasks, handling task completion,
 * running tasks, adding tasks and voters, cleaning up tasks, finding tasks and cores, checking if the scheduler
 * is active, printing results, logging results, and writing results to a file. It also defines the 
 * scheduler, class and its associated methods.
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdbool.h>
#include <stdarg.h>
#include <vector>
#include <string>

#include "defines.h"
#include "task.h"
#include "core.h"
#include "result.h"
#include "voter.h"

class scheduler {
    private:
        vector<task*> m_tasks;
        vector<core*> m_cores;
        vector<result*> m_results;
        time_t m_activationTime;
        time_t m_log_timeout;
        int m_replicates[3];
        int m_voter;
        int m_runs { 0 };

    public:

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
        void init_scheduler();

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
        void monitor_tasks();

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
        void handle_task_completion(task *t, int status, pid_t result);

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
        void run_tasks();

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
        void add_task(const string& name, int period, int offset, int priority, void (*function)(void));

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
        void add_voter(const string& name, int period, int offset, int priority, void (*function)(void), voter_type type);
        
        /**
         * @brief Cleans up all tasks by terminating their processes.
         *
         * This function iterates through all tasks, sends a SIGTERM signal to terminate each task's process,
         * and waits for the process to ensure they are terminated.
         * It then prints a message indicating that the scheduler is shutting down.
         */
        void cleanup_tasks();


        task* get_task(int i) { return m_tasks[i]; }

        /**
        * @brief Finds a task by its name.
        *
        * @param name The name of the task to find.
        * @return A pointer to the task if found, otherwise NULL.
        *
        * This function iterates through the `m_tasks` list and compares each task's name with the specified name.
        * If a matching task is found, it returns a pointer to the task. Otherwise, it returns NULL.
        */
        task* find_task(string name);

        /**
         * @brief Finds a free core for a task to run on.
         *
         * @return The ID of the free core if found, otherwise -1.
         *
         * This function iterates through the cores (excluding the scheduler core) to find a free core for the task.
         * - If `isVoter` is true, it finds the most reliable inactive core based on weight and run count.
         * - If `isVoter` is false, it finds any inactive core with the fewest runs.
         *
         * If no free core is found, it returns -1. Otherwise, it marks the found core as active and returns its ID.
         */
        int find_core(bool isVoter);
        
        /**
         * @brief Checks if the scheduler is active based on the run time or task iterations.
         *
         * @return True if the scheduler is active, otherwise false.
         *
         * The function determines if the scheduler should continue running based on the following conditions:
         * - If `TIME_BASED` is defined, it checks if the current time minus the activation time is less than `MAX_RUN_TIME`.
         * - Otherwise, it checks if the total number of successes and failures for the first task is less than `MAX_ITERATIONS`.
         */
        bool active();


        void printResults();
        void start_scheduler(scheduler *s);
        void log_results();
        void write_results_to_csv();
        string generateOutputString(const string& prefix);
        long current_time_in_ms();

};


#endif