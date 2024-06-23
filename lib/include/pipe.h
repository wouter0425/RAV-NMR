#ifndef PIPE_H
#define PIPE_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include "task.h"

typedef struct pipe_struct {
    int read_fd;               // File descriptor for the read end
    int write_fd;              // File descriptor for the write end
    char *name;                // Name of the pipe
    struct pipe_struct *next;  // Pointer to the next pipe in the list
} pipe_struct;

// Create the pipes
extern pipe_struct *AB;
extern pipe_struct *AB_1;
extern pipe_struct *AB_2;
extern pipe_struct *AB_3;

extern pipe_struct *BC;
extern pipe_struct *BC_1;
extern pipe_struct *BC_2;
extern pipe_struct *BC_3;

extern pipe_struct *CD;


//void declare_pipe(int new_pipe[2]);
pipe_struct *declare_pipe(const char *pipe_name);
void add_pipe(task *t, pipe_struct *new_pipe);
void add_input(input **inputs, int fd);
//void close_pipes(task *t);
pipe_struct *declare_pipe(const char *pipe_name);
//pipe_struct *find_pipe_by_name(task *t, const char *pipe_name);
pipe_struct *find_pipe_by_name(pipe_struct *pipes, const char *pipe_name);
bool is_pipe_content_available(int pipe_fd);
bool task_input_full(task *t);
int count_content(pipe_struct *p);
void freeInputs(input *i);
void freePipes(pipe_struct *p);
//void close_pipe_by_name(task *t, const char *pipe_name);
//void open_pipe_by_name(task *t, const char *pipe_name);

void open_pipe_read_end(pipe_struct *pipe);
void open_pipe_write_end(pipe_struct *pipe);
void close_pipe_read_end(pipe_struct *pipe);
void close_pipe_write_end(pipe_struct *pipe);

bool read_from_pipe(pipe_struct *pipe, char *buffer, size_t buf_size);
void write_to_pipe(pipe_struct *pipe, const char *buffer);

#endif