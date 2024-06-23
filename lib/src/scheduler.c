#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
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
        // Check
        if (task_input_full(&s->m_tasks[i]) && !s->m_tasks[i].m_active) {
            s->m_tasks[i].m_fireable = true;
        }
        else {
            s->m_tasks[i].m_fireable = false;
        }

        if (!s->m_tasks[i].m_fireable && s->m_tasks[i].m_active) {
            int status;
            pid_t result = waitpid(s->m_tasks[i].pid, &status, WNOHANG);

            // Task has finished
            if (result == 0) {
                continue; // Skip to the next task on error
            }
            else if (WIFEXITED(status)) {
                if (WEXITSTATUS(status) == 0) {
                    s->m_tasks[i].m_success++;
                    // Increase the core reliability after a successfull run
                    if (s->m_cores[s->m_tasks[i].cpu_id].m_weight < MAX_CORE_WEIGHT) {
                        s->m_cores[s->m_tasks[i].cpu_id].m_weight *= 1.1;
                    }
                } else {
                    s->m_tasks[i].m_fails++;
                    s->m_cores[s->m_tasks[i].cpu_id].m_weight *= 0.9;
                }
                //s->m_tasks[i].m_active = false;

            }
            else if (WIFSIGNALED(status)) {
                s->m_cores[s->m_tasks[i].cpu_id].m_weight *= 0.9;
                s->m_tasks[i].m_fails++;
            }

            s->m_tasks[i].m_active = false;
            s->m_cores[s->m_tasks[i].cpu_id].runs++;
            s->m_cores[s->m_tasks[i].cpu_id].m_active = false;


        }
    }
}

void start_scheduler(scheduler *s)
{
    while(active(s))
    {
        monitor_tasks(s);
        run_tasks(s);
        //usleep(200000);
    }
}

void add_task(scheduler *s, int id, const char *name, int period, void (*function)(void))
{
    s->m_tasks[id].name = strdup(name);
    s->m_tasks[id].function = function;
    s->m_tasks[id].inputs = NULL;
    s->m_tasks[id].m_fails = 0;
    s->m_tasks[id].m_success = 0;
    s->m_tasks[id].m_active = false;
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
    if (difftime(time(NULL), s->m_activationTime) < MAX_RUN_TIME)
    {
        return true;
    }
    else {
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