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
        core(int id, float weight, bool active, int runs);

        int get_coreID() { return m_coreID; }
        void set_coreID(int coreID) { m_coreID = coreID; }

        float get_weight() { return m_weight; }
        void set_weight(float weight) { m_weight = weight; }
        void increase_weight() { m_weight *= INCREASE; }
        void decrease_weight() { m_weight *= DECREASE; }

        bool get_active() { return m_active; }
        void set_active(bool active) { m_active = active; }

        int get_runs() { return m_runs; }
        void set_runs(int runs) { m_runs = runs; }
        void increase_runs() { m_runs++; }
};

class result {
    private:

    public:
        long m_time;
        vector<int> m_cores;
        vector<float> m_weights;
        vector<int> m_tasks;
        result(vector<task*> tasks, vector<core*> cores, long mSeconds);

};

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
        task* get_task(int i) { return m_tasks[i]; }
        task* find_task(string name);

        void init_scheduler();
        void run_tasks();
        void monitor_tasks();
        void handle_task_completion(task *t, int status, pid_t result);
        void cleanup_tasks();
        int find_core(bool isVoter = false);
        
        void printResults();
        bool active();
        void add_task(const string& name, int period, int offset, int priority, void (*function)(void));
        void add_voter(const string& name, int period, int offset, int priority, void (*function)(void));
        void start_scheduler(scheduler *s);
        void log_results();
        void write_results_to_csv();

};


bool is_pipe_content_available(int pipe_fd);
//void close_pipes(int, ...);
string generateOutputString(const string& prefix);
long current_time_in_ms();

#endif