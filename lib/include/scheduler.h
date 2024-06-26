#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdbool.h>
#include <stdarg.h>
#include "defines.h"
#include "task.h"

typedef struct {
    int m_coreID;
    float m_weight;
    bool m_active;
    int runs;
} core;

typedef struct {
    int m_cores[NUM_OF_CORES];
    float m_weights[NUM_OF_CORES];
} result;

typedef struct {
    task m_tasks[NUM_OF_TASKS];
    core m_cores[NUM_OF_CORES];
    time_t m_activationTime;
    time_t m_log_timeout;
    int m_replicates[3];
    int m_voter;
    int m_counter;
    result m_results[NUM_OF_SAMPLES];
} scheduler;


void handle_signal(int sig);
bool is_pipe_content_available(int pipe_fd);
void close_pipes(int, ...);

void init_scheduler(scheduler *s);
void run_tasks(scheduler *s);
void monitor_tasks(scheduler *s);
void cleanup_tasks(scheduler *s);
int find_core(core *c);
void printResults(scheduler *s);
bool active(scheduler *s);
void add_task(scheduler *s, int id, const char *name, int period, void (*function)(void));
void start_scheduler(scheduler *s);
void log_results(scheduler *s);
void write_results_to_csv(scheduler *s);
long current_time_in_ms();

#endif