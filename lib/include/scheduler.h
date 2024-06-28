#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdbool.h>
#include <stdarg.h>
#include <vector>
#include <string>

#include "defines.h"
#include "task.h"

class core {
    private:
        int m_coreID;
        float m_weight;
        bool m_active;
        int m_runs;

    public:
        int get_coreID() { return m_coreID; }
        void set_coreID(int coreID) { m_coreID = coreID; }

        float get_weight() { return m_weight; }
        void set_weight(float weight) { m_weight = weight; }
        void increase_weight() { m_weight += INCREASE; }
        void decrease_weight() { m_weight += DECREASE; }

        bool get_active() { return m_active; }
        void set_active(bool active) { m_active = active; }

        int get_runs() { return m_runs; }
        void set_runs(int runs) { m_runs = runs; }
        void increase_runs() { m_runs++; }
};

class result {
    private:

    public:
        int m_cores[NUM_OF_CORES];
        float m_weights[NUM_OF_CORES];
};

class scheduler {
    private:
        task m_tasks[NUM_OF_TASKS];
        core m_cores[NUM_OF_CORES];
        //vector<task> m_tasks;
        //vector<core> m_cores;
        time_t m_activationTime;
        time_t m_log_timeout;
        int m_replicates[3];
        int m_voter;
        int m_counter;
        result m_results[NUM_OF_SAMPLES];

    public:
        task* get_task(int i) { return &m_tasks[i]; }
        void set_replicate(int i, int j) { m_replicates[i] = j; }
        void set_voter(int i) { m_voter = i; }

        void init_scheduler();
        void run_tasks();
        void monitor_tasks();
        void cleanup_tasks();
        int find_core();
        void printResults();
        bool active();
        void add_task(int id, const string& name, int period, void (*function)(void));
        void start_scheduler(scheduler *s);
        void log_results();
        void write_results_to_csv();

};



void handle_signal(int sig);
bool is_pipe_content_available(int pipe_fd);
//void close_pipes(int, ...);

long current_time_in_ms();

#endif