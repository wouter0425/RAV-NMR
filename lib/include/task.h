#ifndef TASK_H
#define TASK_H

#include <stdbool.h>
#include <pthread.h>
#include <stdarg.h>

//#include "pipe.h"

// Define the struct for pipe endpoints


typedef struct input {
    int fd;
    struct input *next;
} input;

typedef struct {
    char *name;
    int cpu_id;
    int m_active;
    bool m_fireable;
    pid_t pid;
    void (*function)(void);
    //pipe_struct *pipes;
    input *inputs;
    int m_success;
    int m_fails;
    bool m_voter;
    bool m_replicate;
    bool m_finished;
} task;

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