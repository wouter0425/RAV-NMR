#ifndef TASK_H
#define TASK_H

#include <stdbool.h>
#include <pthread.h>
#include <stdarg.h>
#include <string>
#include <vector>
//#include "pipe.h"

// Define the struct for pipe endpoints

using namespace std;

enum task_state {
    idle,
    waiting,
    scheduled,
    running,
    crashed,
};

typedef struct input {
    int fd;
    struct input *next;
} input;

typedef struct run_log {
    time_t time;
    bool success;
} run_log;

typedef struct replicate {
    string name;
    bool armed;
} replicate;

class task {
    private:
        string m_name;
        int m_cpu_id;
        bool m_active;
        bool m_fireable;
        int m_priority;
        pid_t m_pid;
        void (*m_function)(void);
        input *m_inputs;
        int m_success;
        int m_fails;
        bool m_voter;
        bool m_replicate;
        int m_runs { 0 };
        bool m_finished;
        int m_period;
        int m_offset;
        unsigned long int m_startTime;
        task_state m_state;
        int m_coreRuns[NUM_OF_CORES];
        //vector<run_log> m_results;

        pid_t m_latestResult;
        int m_latestStatus;

    public:

        task(const string& name, int period, int offset, int priority, void (*function)(void));
        task();

        bool period_elapsed(unsigned long int currentTime);
        bool offset_elapsed(unsigned long int startTime, unsigned long int currentTime);

        void increment_runs() { m_runs++; }
        int get_runs() { return m_runs; }

        string get_name() { return m_name; }
        void set_name(string name) { m_name = name; }

        int get_cpu_id() { return m_cpu_id; }
        void set_cpu_id(int id) { m_cpu_id = id; }

        int get_active() { return m_active; }
        void set_active(int active) { m_active = active; }

        bool get_fireable() { return m_fireable; }
        void set_fireable(bool fireable) { m_fireable = fireable; }

        int get_priority() { return m_priority; }
        void set_priority(int priority) { m_priority = priority; }

        pid_t get_pid() { return m_pid; }
        void set_pid(pid_t p) { m_pid = p; }

        void run() { m_function(); }

        void set_startTime(unsigned long int startTime) { m_startTime = startTime; }

        bool is_stuck(unsigned long int elapsedTime, int status, pid_t result); 

        input* get_inputs() { return m_inputs; }
        input*& get_inputs_ref() { return m_inputs; }
        void set_inputs(input* in) { m_inputs = in; }

        int get_success() { return m_success; }
        void set_success(int success) { m_success = success; }
        void increment_success() { m_success++; }

        int get_fails() { return m_fails; }
        void set_fails(int fails) { m_fails = fails; }
        void increment_fails() { m_fails++; }

        bool get_voter() { return m_voter; }
        void set_voter(bool voter) { m_voter = voter; }

        bool get_replicate() { return m_replicate; }
        void set_replicate(bool replicate) { m_replicate = replicate; }

        bool get_finished() { return m_finished; }
        void set_finished(bool finished) { m_finished = finished; }

        task_state get_state() { return m_state; }
        void set_state(task_state state) { m_state = state; }

        void set_latest(int status, pid_t result) { m_latestStatus = status; m_latestResult = result; }
        int get_latestStatus() { return m_latestStatus; }
        pid_t get_latestResult() { return m_latestResult; }

        void add_core_run(int core) { m_coreRuns[core]++; };
        void print_core_runs() 
        {            
            for (int i = 0; i < NUM_OF_CORES; i++)
            {
                printf("Core %d: %d \t", i, m_coreRuns[i]);
            }        
            printf("\n");
        }
};

class voter : public task {
    private:
        vector<task*> m_replicates;
        vector<replicate> m_replicateMonitor;
        bool m_armed {false};

    public:
        voter(const string& name, int period, int offset, int priority, void (*function)(void));
        bool check_replicate_state(task_state state);
        void add_replicate(task *t);
        bool get_voter_fireable();
        void set_armed(bool armed) { m_armed = armed; }
        bool get_armed() { return m_armed; } 

};

void hello_world(void);

void task_A(void);
void task_B(void);
void task_C(void);

void task_A_1(void);
void task_B_1(void);
void task_B_2(void);
void task_B_3(void);
void task_C_1(void);

void voter_func(void);

#endif