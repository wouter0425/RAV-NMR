#ifndef TASK_H
#define TASK_H

#include <stdbool.h>
#include <pthread.h>
#include <stdarg.h>
#include <string.h>
#include <string>
//#include "pipe.h"

// Define the struct for pipe endpoints

using namespace std;

typedef struct input {
    int fd;
    struct input *next;
} input;

class task {
    private:
        string m_name;
        int m_cpu_id;
        bool m_active;
        bool m_fireable;
        pid_t m_pid;
        //void (*m_function)(void);
        input *m_inputs;
        int m_success;
        int m_fails;
        bool m_voter;
        bool m_replicate;
        bool m_finished;

    public:
        task(const string& name, int period, void (*function)(void));
        task();
        void (*m_function)(void);

        string get_name() { return m_name; }
        void set_name(string name) { m_name = name; }

        int get_cpu_id() { return m_cpu_id; }
        void set_cpu_id(int id) { m_cpu_id = id; }

        int get_active() { return m_active; }
        void set_active(int active) { m_active = active; }

        bool get_fireable() { return m_fireable; }
        void set_fireable(bool fireable) { m_fireable = fireable; }

        pid_t get_pid() { return m_pid; }
        void set_pid(pid_t p) { m_pid = p; }

        //void (*get_function())(void) { return m_function; }
        //void set_function(void (*func)(void)) { m_function = func; }

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
};

void task_A(void);
void task_B(void);
void task_C(void);

void task_A_1(void);
void task_B_1(void);
void task_B_2(void);
void task_B_3(void);
void task_C_1(void);

void voter(void);

#endif