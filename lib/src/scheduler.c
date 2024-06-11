#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "scheduler.h"
#include "task.h"
#include "defines.h"

typedef struct {
    int m_coreID;
    int m_weight;
} core;

typedef struct {
    task m_tasks[NUM_OF_TASKS];
    core m_cores[NUM_OF_CORES];
} scheduler;

static scheduler m_scheduler;

void handle_signal(int sig) {
    if (sig == SIGINT) {
        printf("Scheduler received SIGINT, shutting down...\n");
        exit(EXIT_SUCCESS);
    }
}

void *scheduler_task(void *arg) {
    task tasks[NUM_OF_TASKS];
    int pipe_AB[2], pipe_BC[2];

    // Create pipes for inter-task communication
    if (pipe(pipe_AB) == -1 || pipe(pipe_BC) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    
    // Setup signal handler for clean exit
    signal(SIGINT, handle_signal);

    // Fork and set CPU affinity for each task
    for (int i = 0; i < NUM_OF_TASKS; i++) {
        tasks[i].cpu_id = i;

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Set CPU affinity for the task
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(tasks[i].cpu_id, &cpuset);
            if (sched_setaffinity(0, sizeof(cpuset), &cpuset) != 0) {
                perror("sched_setaffinity");
                exit(EXIT_FAILURE);
            }

            if (i == 0) {
                close(pipe_AB[0]); // Close read end of pipe A->B
                task_A(pipe_AB[1]);
                close(pipe_AB[1]);
            } else if (i == 1) {
                close(pipe_AB[1]); // Close write end of pipe A->B
                close(pipe_BC[0]); // Close read end of pipe B->C
                task_B(pipe_AB[0], pipe_BC[1]);
                close(pipe_AB[0]);
                close(pipe_BC[1]);
            } else if (i == 2) {
                close(pipe_BC[1]); // Close write end of pipe B->C
                task_C(pipe_BC[0]);
                close(pipe_BC[0]);
            }

            exit(EXIT_SUCCESS);
        } else {
            tasks[i].pid = pid;
        }
    }

    // Monitor tasks and handle exit
    while (1) {
        for (int i = 0; i < NUM_OF_TASKS; i++) {
            int status;
            pid_t result = waitpid(tasks[i].pid, &status, WNOHANG);
            if (result == 0) {
                continue; // Task is still running
            } else if (result == -1) {
                perror("waitpid");
                exit(EXIT_FAILURE);
            } else {
                if (WIFEXITED(status)) {
                    printf("Task %d exited with status %d\n", i, WEXITSTATUS(status));
                } else if (WIFSIGNALED(status)) {
                    printf("Task %d was killed by signal %d\n", i, WTERMSIG(status));
                }
            }
        }
        sleep(1); // Avoid busy loop
    }

    // Cleanup: kill all child processes
    for (int i = 0; i < NUM_OF_TASKS; i++) {
        kill(tasks[i].pid, SIGTERM);
        waitpid(tasks[i].pid, NULL, 0); // Ensure they are terminated
    }

    printf("Scheduler shutting down...\n");

    return NULL;
}
