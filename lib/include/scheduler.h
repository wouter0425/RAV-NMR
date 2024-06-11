#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdbool.h>
#include <stdarg.h>
#include "defines.h"
#include "task.h"
typedef struct {
    int m_coreID;
    int m_weight;
} core;

typedef struct {
    task m_tasks[NUM_OF_TASKS];
    core m_cores[NUM_OF_CORES];
} scheduler;

void handle_signal(int sig);
bool is_pipe_content_available(int pipe_fd);
void close_pipes(int, ...);

void init_scheduler(scheduler *s);
void run_tasks(scheduler *s);
void monitor_tasks(scheduler *s);
void cleanup_tasks(scheduler *s);

#endif