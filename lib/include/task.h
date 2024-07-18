#ifndef TASK_H
#define TASK_H

#include <stdbool.h>
#include <pthread.h>
#include <stdarg.h>
#include <string>
#include <vector>
#include <defines.h>

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
        bool m_active { false };
        bool m_fireable;
        int m_priority;
        pid_t m_pid;
        void (*m_function)(void);
        input *m_inputs { NULL };
        int m_success { 0 };
        int m_fails { 0 };
        int m_errors { 0 };
        bool m_voter { false };
        int m_runs { 0 };
        bool m_finished { false } ;
        unsigned long int m_period;
        unsigned long int m_offset;
        unsigned long int m_startTime { 0 };
        task_state m_state;
        int m_coreRuns[NUM_OF_CORES];
        pid_t m_latestResult;
        int m_latestStatus;

    public:

        /**
         * @brief Parameterized constructor for the task class.
         * 
         * @param name Name of the task.
         * @param period The period at which the task should be executed.
         * @param offset The offset time before the task starts executing.
         * @param priority The priority of the task.
         * @param function Pointer to the function that the task will execute.
         */
        task(const string& name, unsigned long int period, unsigned long int offset, int priority, void (*function)(void));

        /**
         * @brief Checks if the offset time has elapsed since the task started.
         * 
         * @param startTime The start time of the task.
         * @param currentTime The current time.
         * @return true if the offset has elapsed, false otherwise.
         */
        bool offset_elapsed(unsigned long int startTime, unsigned long int currentTime);


        /**
         * @brief Checks if the period time has elapsed since the task started.
         * 
         * @param currentTime The current time.
         * @return true if the period has elapsed, false otherwise.
         */
        bool period_elapsed(unsigned long int currentTime);
        
        /**
         * @brief Checks if the task is stuck based on elapsed time, status, and result.
         * 
         * @param elapsedTime The elapsed time since the task started.
         * @param status The current status of the task.
         * @param result The result of the task's execution.
         * @return true if the task is stuck, false otherwise.
         */
        bool is_stuck(unsigned long int elapsedTime, int status, pid_t result); 
        
        
        /**
         * @brief Prints the number of times the task has run on each core.
         */
        void print_core_runs();

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

        int get_priority() const { return m_priority; }
        void set_priority(int priority) { m_priority = priority; }

        pid_t get_pid() { return m_pid; }
        void set_pid(pid_t p) { m_pid = p; }

        void run() { if(m_function != NULL ) m_function(); }

        void set_startTime(unsigned long int startTime) { m_startTime = startTime; }

        input* get_inputs() { return m_inputs; }
        input*& get_inputs_ref() { return m_inputs; }
        void set_inputs(input* in) { m_inputs = in; }

        int get_success() { return m_success; }
        void set_success(int success) { m_success = success; }
        void increment_success() { m_success++; }

        int get_fails() { return m_fails; }
        void increment_fails() { m_fails++; }

        int get_errors() { return m_errors; }
        void increment_errors() { m_errors++; }

        bool get_voter() { return m_voter; }
        void set_voter(bool voter) { m_voter = voter; }

        bool get_finished() { return m_finished; }
        void set_finished(bool finished) { m_finished = finished; }

        task_state get_state() { return m_state; }
        void set_state(task_state state) { m_state = state; }

        void set_latest(int status, pid_t result) { m_latestStatus = status; m_latestResult = result; }
        int get_latestStatus() { return m_latestStatus; }
        pid_t get_latestResult() { return m_latestResult; }

        void add_core_run(int core) { m_coreRuns[core]++; };
};

#endif