#ifndef TASK_H
#define TASK_H

#include <stdbool.h>
#include <pthread.h>
#include <stdarg.h>

//#include "pipe.h"

// Define the struct for pipe endpoints
typedef struct pipe_struct {
    int read_fd;               // File descriptor for the read end
    int write_fd;              // File descriptor for the write end
    char *name;                // Name of the pipe
    struct pipe_struct *next;  // Pointer to the next pipe in the list
} pipe_struct;

typedef struct {
    int cpu_id;
    int m_active;
    bool m_fireable;
    pid_t pid;
    void (*function)(struct pipe_struct *);
    pipe_struct *pipes;
} task;

void task_A(struct pipe_struct *pipes);
void task_B(struct pipe_struct *pipes);
void task_C(struct pipe_struct *pipes);

#endif