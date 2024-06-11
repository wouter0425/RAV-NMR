#ifndef PIPE_H
#define PIPE_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include "task.h"

//void declare_pipe(int new_pipe[2]);
pipe_struct *declare_pipe(const char *pipe_name);
void add_pipe(task *t, pipe_struct *new_pipe);
//void close_pipes(task *t);
pipe_struct *declare_pipe(const char *pipe_name);
//pipe_struct *find_pipe_by_name(task *t, const char *pipe_name);
pipe_struct *find_pipe_by_name(pipe_struct *pipes, const char *pipe_name);
bool is_pipe_content_available(int pipe_fd);
//void close_pipe_by_name(task *t, const char *pipe_name);
//void open_pipe_by_name(task *t, const char *pipe_name);

#endif