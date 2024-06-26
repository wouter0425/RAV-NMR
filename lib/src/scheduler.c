#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include "scheduler.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include "pipe.h"
#include "defines.h"

void handle_signal(int sig) {
    if (sig == SIGINT) {
        printf("Scheduler received SIGINT, shutting down...\n");
        exit(EXIT_SUCCESS);
    }
}

void init_scheduler(scheduler *s)
{
    // init the cores
    for (int i = 0; i < NUM_OF_CORES; i++)
    {
        s->m_cores[i].m_coreID = i;
        s->m_cores[i].m_weight = MAX_CORE_WEIGHT;
        s->m_cores[i].m_active = false;
        s->m_cores[i].runs = 0;
    }

    // exit handler function
    signal(SIGINT, handle_signal);

    // Specify the CPU core to run the scheduler on
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    CPU_SET(0, &cpuset);
    s->m_activationTime = time(NULL);
    s->m_log_timeout = time(NULL);
    s->m_counter = 0;

    printf("start time: %ld \n", s->m_activationTime);

    if (sched_setaffinity(0, sizeof(cpuset), &cpuset) != 0) {
        perror("sched_setaffinity");
        exit(EXIT_FAILURE);
    }
}

void run_tasks(scheduler *s)
{
    // Fork and set CPU affinity for each task
    for (int i = 0; i < NUM_OF_TASKS && s->m_tasks[i].m_fireable == true; i++) {
        s->m_tasks[i].cpu_id = find_core(s->m_cores);
        pid_t pid = fork();

        if (pid == -1) {
            printf("error \n");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Set CPU affinity for the task
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(s->m_tasks[i].cpu_id, &cpuset);
            CPU_SET(s->m_tasks[i].cpu_id, &cpuset);

            if (prctl(PR_SET_NAME, (unsigned long) s->m_tasks[i].name) < 0) {
                perror("prctl()");
            }

            if (sched_setaffinity(0, sizeof(cpuset), &cpuset) != 0) {
                perror("sched_setaffinity");
                exit(EXIT_FAILURE);
            }

            s->m_tasks[i].function();

            exit(EXIT_SUCCESS);

        } else {
            s->m_tasks[i].pid = pid;
            s->m_tasks[i].m_active = true;
            s->m_cores[s->m_tasks[i].cpu_id].m_active = true;
        }
    }
}

void monitor_tasks(scheduler *s)
{
    for (int i = 0; i < NUM_OF_TASKS; i++) {
        // Only launch when input is full and/or the task is activated
        if (task_input_full(&s->m_tasks[i]) && !s->m_tasks[i].m_active) {
            s->m_tasks[i].m_fireable = true;
        }
        else {
            s->m_tasks[i].m_fireable = false;
        }

        // Monitor active tasks
        if (!s->m_tasks[i].m_fireable && s->m_tasks[i].m_active) {
            int status;
            pid_t result = waitpid(s->m_tasks[i].pid, &status, WNOHANG);

            // Task has finished
            if (result == 0) {
                continue;
            }
            else if (WIFEXITED(status)) {
                // Task ended successfull
                if (WEXITSTATUS(status) == 0) {
                    // Increase the core reliability after a successfull run
                    s->m_tasks[i].m_success++;
                    s->m_cores[s->m_tasks[i].cpu_id].m_weight *= 1.1;

                    // Prevent runaway core weight
                    if (s->m_cores[s->m_tasks[i].cpu_id].m_weight > MAX_CORE_WEIGHT) {
                        s->m_cores[s->m_tasks[i].cpu_id].m_weight = 100;
                    }

                } else {
                    // Decrease reliability after a process returned with an non-zero exit code
                    s->m_tasks[i].m_fails++;
                    s->m_cores[s->m_tasks[i].cpu_id].m_weight *= 0.9;
                }

            }
            else if (WIFSIGNALED(status)) {
                // Decrease reliability after a process crash on a core
                s->m_cores[s->m_tasks[i].cpu_id].m_weight *= 0.9;
                s->m_tasks[i].m_fails++;
            }

            // Update task and core state
            s->m_tasks[i].m_active = false;
            s->m_cores[s->m_tasks[i].cpu_id].runs++;
            s->m_cores[s->m_tasks[i].cpu_id].m_active = false;

            // TODO: Ugly fix to distinguish between normal and replicate task
            if (s->m_tasks[i].m_replicate) {
                s->m_tasks[i].m_finished = true;
            }
        }
    }

#ifdef NMR
    // TODO: Ugly fix to check if all replicates have finished
    int counter = 0;
    for (int i = 0; i < NUM_OF_TASKS; i++) {
        if (s->m_tasks[i].m_replicate && s->m_tasks[i].m_finished) {
            counter++;
        }
    }

    if (counter == 3) {
        s->m_tasks[s->m_voter].m_fireable = true;
    }
    else {
        s->m_tasks[s->m_voter].m_fireable = false;
    }
#endif
}

void add_task(scheduler *s, int id, const char *name, int period, void (*function)(void))
{
    s->m_tasks[id].name = strdup(name);
    s->m_tasks[id].function = function;
    s->m_tasks[id].inputs = NULL;
    s->m_tasks[id].m_fails = 0;
    s->m_tasks[id].m_success = 0;
    s->m_tasks[id].m_active = false;
    s->m_tasks[id].m_replicate = false;
    s->m_tasks[id].m_voter = false;
    s->m_tasks[id].m_finished = false;

    // Set only cyclic tasks to be fireable
    if (period) {
        s->m_tasks[id].m_fireable = true;
    }
    else {
        s->m_tasks[id].m_fireable = false;
    }

    return;
}

void cleanup_tasks(scheduler *s)
{
    // Cleanup: kill all child processes
    for (int i = 0; i < NUM_OF_TASKS; i++)
    {
        kill(s->m_tasks[i].pid, SIGTERM);
        waitpid(s->m_tasks[i].pid, NULL, 0); // Ensure they are terminated

        // free all related pointers
        free(s->m_tasks[i].name);
    }

    printf("Scheduler shutting down...\n");
}

int find_core(core *c)
{
    // Always skip the first core, this is used for the scheduler
    int core_id = 1;

    for (int i = 1; i < NUM_OF_CORES; i++)
    {
#ifdef RELIABILITY_SCHEDULING
        // If core is inactive and is more reliable
        if (!c[i].m_active && c[i].m_weight > c[core_id].m_weight) {
            core_id = i;
        }
        else if (!c[i].m_active && c[i].m_weight == c[core_id].m_weight) {
            // if the weight is the same, but the core has more runs, balance the load
            if (c[i].runs < c[core_id].runs) {
                core_id = i;
                //printf("test \n");
            }
        }
#else
        if (!c[i].m_active && c[i].runs < c[core_id].runs) {
            core_id = i;
        }
#endif

    }

    // Change the core state
    c[core_id].m_active = true;

    return core_id;
}

bool active(scheduler *s)
{
    time_t currentTime = time(NULL);

    // Convert current time and activation time to milliseconds
    long currentTimeMs = currentTime * 1000;
    long activationTimeMs = s->m_activationTime * 1000;

    if (currentTimeMs - activationTimeMs < MAX_RUN_TIME)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void printResults(scheduler *s)
{
    for (int i = 0; i < NUM_OF_TASKS; i++)
    {
        printf("Task: %s \t succesfull runs: %d \t failed runs: %d \n", s->m_tasks[i].name, s->m_tasks[i].m_success, s->m_tasks[i].m_fails);
    }

    for (int i = 1; i < NUM_OF_CORES; i++)
    {
        printf("Core: %d \t runs: %d \t weight: %f \n", s->m_cores[i].m_coreID, s->m_cores[i].runs, s->m_cores[i].m_weight);
    }
}

void log_results(scheduler *s) {
    long currentTimeMs = current_time_in_ms();

    if ((currentTimeMs - s->m_log_timeout > MAX_LOG_INTERVAL) && (s->m_counter < NUM_OF_SAMPLES)) {
        for (int i = 0; i < NUM_OF_CORES; i++) {
            s->m_results[s->m_counter].m_cores[i] = s->m_cores[i].runs;
            s->m_results[s->m_counter].m_weights[i] = s->m_cores[i].m_weight;
        }

        s->m_log_timeout = currentTimeMs;
        s->m_counter++;
        printf("Log entry added. Counter: %d\n", s->m_counter);
    } else if ((currentTimeMs - s->m_log_timeout > MAX_LOG_INTERVAL) && (s->m_counter >= NUM_OF_SAMPLES)) {
        printf("Sample limit reached. No more entries added.\n");
    }
}

void write_results_to_csv(scheduler *s) {
    FILE *file = fopen("output.txt", "w");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    // Write the header
    for (int i = 1; i < NUM_OF_CORES; i++) {
        fprintf(file, "core_%d", i);
        if (i < NUM_OF_CORES - 1) {
            fprintf(file, "\t");
        }
    }
    for (int i = 1; i < NUM_OF_CORES; i++) {
        fprintf(file, "\tweight_%d", i);
    }
    fprintf(file, "\n");

    // Write the data
    for (int i = 1; i < s->m_counter; i++) {
        for (int j = 1; j < NUM_OF_CORES; j++) {
            fprintf(file, "%d", s->m_results[i].m_cores[j]);
            if (j < NUM_OF_CORES - 1) {
                fprintf(file, "\t");
            }
        }
        for (int j = 1; j < NUM_OF_CORES; j++) {
            fprintf(file, "\t%.2f", s->m_results[i].m_weights[j]);
        }
        fprintf(file, "\n");
    }

    fclose(file);
}

long current_time_in_ms() {
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return (spec.tv_sec * 1000) + (spec.tv_nsec / 1000000);
}