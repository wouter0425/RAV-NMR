#ifndef TASK_H
#define TASK_H

#include <pthread.h>

typedef struct {
    int cpu_id;
    //int pipe_fd[2];
    pid_t pid;
} task;

#endif