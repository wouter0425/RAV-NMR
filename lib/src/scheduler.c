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
    } 

    // init the pipes
    pipe_struct *AB = declare_pipe("pipe_AB");
    pipe_struct *BC = declare_pipe("pipe_BC");

    // init the tasks
    s->m_tasks[0].function = task_A;
    s->m_tasks[0].pipes = NULL;
    add_pipe(&s->m_tasks[0], AB);

    s->m_tasks[1].function = task_B;
    s->m_tasks[1].pipes = NULL;
    add_pipe(&s->m_tasks[1], AB);
    add_pipe(&s->m_tasks[1], BC);

    s->m_tasks[2].function = task_C;
    s->m_tasks[2].pipes = NULL;
    add_pipe(&s->m_tasks[2], BC);

    // Init tasks
    for (int i = 0; i < NUM_OF_TASKS; i++) {
        s->m_tasks[i].m_active = false;
        s->m_tasks[i].m_fireable = true;
    }

    // exit handler function    
    signal(SIGINT, handle_signal);

    // Specify the CPU core to run the scheduler on
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    CPU_SET(0, &cpuset);

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
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Set CPU affinity for the task
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(s->m_tasks[i].cpu_id, &cpuset);
            CPU_SET(s->m_tasks[i].cpu_id, &cpuset);

            if (sched_setaffinity(0, sizeof(cpuset), &cpuset) != 0) {
                perror("sched_setaffinity");
                exit(EXIT_FAILURE);
            }

            s->m_tasks[i].function(s->m_tasks[i].pipes);

            exit(EXIT_SUCCESS);
        } else {
            s->m_tasks[i].pid = pid;
            s->m_tasks[i].m_active = true;
            s->m_tasks[i].m_fireable = false;
        }
    }
}

void monitor_tasks(scheduler *s)
{
    for (int i = 0; i < NUM_OF_TASKS; i++) {
        int status;
        pid_t result = waitpid(s->m_tasks[i].pid, &status, WNOHANG);
        if (result == 0) {
            continue; // Task is still running
        } else if (result == -1) {
            // TODO: Maybe check here
        } else {
            if (WIFEXITED(status)) {
                printf("Task %d exited with status %d\n", i, WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("Task %d was killed by signal %d\n", i, WTERMSIG(status));
            }
            
            // restart the task and let the core know its available
            s->m_tasks[i].m_fireable = true;
            s->m_cores[s->m_tasks[i].cpu_id].m_active = false;
        }
    }        
    sleep(1); // Avoid busy loop   
}

void cleanup_tasks(scheduler *s)
{
    // Cleanup: kill all child processes
    for (int i = 0; i < NUM_OF_TASKS; i++) {
        kill(s->m_tasks[i].pid, SIGTERM);
        waitpid(s->m_tasks[i].pid, NULL, 0); // Ensure they are terminated
    }
    printf("Scheduler shutting down...\n");
}

int find_core(core *c)
{
    int core_id = 1;

    for (int i = 0; i < NUM_OF_CORES; i++)
    {
        if (c[i].m_active && c[i].m_weight > c[core_id].m_weight) {
            core_id = i;
        }
    }

    c[core_id].m_active = true;

    return core_id;
}