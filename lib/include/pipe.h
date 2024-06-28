#ifndef PIPE_H
#define PIPE_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include "defines.h"
#include "task.h"

class Pipe {
    private:
        int m_read_fd;               // File descriptor for the read end
        int m_write_fd;              // File descriptor for the write end
        char *m_name;              // Name of the pipe
        struct Pipe *m_next;         // Pointer to the next pipe in the list

    public:
        Pipe(int read_fd, int write_fd, const char* name, Pipe *next);

        int get_read_fd() { return m_read_fd; }
        void set_read_fd(int fd) { m_read_fd = fd; }

        int get_write_fd() { return m_write_fd; }
        void set_write_fd(int fd) { m_write_fd = fd; }

        char* get_name() { return m_name; }        

        struct Pipe* get_next() { return m_next; }
        void set_next(struct Pipe* p) { m_next = p; }


};

void add_input(input **inputs, int fd);
bool task_input_full(task *t);
void open_pipe_read_end(Pipe *pipe);
void open_pipe_write_end(Pipe *pipe);
void close_pipe_read_end(Pipe *pipe);
void close_pipe_write_end(Pipe *pipe);

bool read_from_pipe(Pipe *pipe, char *buffer, size_t buf_size);
void write_to_pipe(Pipe *pipe, const char *buffer);



// Declare the pipes
#ifndef NMR
extern Pipe *AB;
extern Pipe *BC;
#else
extern Pipe *AB_1;
extern Pipe *AB_2;
extern Pipe *AB_3;
extern Pipe *BC_1;
extern Pipe *BC_2;
extern Pipe *BC_3;
extern Pipe *CD_1;
#endif

Pipe *declare_pipe(const char *pipe_name);

#endif