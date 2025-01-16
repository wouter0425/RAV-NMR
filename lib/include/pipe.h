/**
 * @file pipe.h
 * @brief This file contains functions for managing pipes and tasks in a Unix environment.
 * 
 * The file includes functions for creating pipes, adding inputs to a list,
 * checking if a task's inputs are full, and reading from or writing to pipes.
 * It also includes the definition of the Pipe class and associated methods.
 * 
 * Functions:
 * - Pipe::Pipe(int read_fd, int write_fd, const char* name, Pipe *next)
 * - Pipe *declare_pipe(const char *pipe_name)
 * - void add_input(input **inputs, int fd)
 * - bool task_input_full(task *t)
 * - bool read_from_pipe(Pipe *pipe, char *buffer, size_t buf_size)
 * - void write_to_pipe(Pipe *pipe, const char *buffer)
 */


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
        char *m_name;                // Name of the pipe
        struct Pipe *m_next;         // Pointer to the next pipe in the list

    public:
        Pipe(int read_fd, int write_fd, const char* name, Pipe *next);

        int get_read_fd() { return m_read_fd; }

        int get_write_fd() { return m_write_fd; }

        char* get_name() { return m_name; }
};

// Declare the pipes here
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

/**
 * @brief Creates a new pipe and returns a Pipe object.
 * 
 * @param pipe_name Name of the pipe.
 * @return Pointer to the created Pipe object.
 */
Pipe *declare_pipe(const char *pipe_name);

/**
 * @brief Adds a file descriptor to the list of inputs.
 * 
 * @param inputs Pointer to the head of the input list.
 * @param fd File descriptor to add.
 */
void add_input(input **inputs, int fd, int size);

/**
 * @brief Checks if the task's input is full.
 * 
 * @param t Pointer to the task object.
 * @return true if all inputs are ready to be read; false otherwise.
 */
bool task_input_full(task *t);

/**
 * @brief Reads data from a pipe into a buffer.
 * 
 * @param pipe Pointer to the Pipe object.
 * @param buffer Buffer to store the read data.
 * @param buf_size Size of the buffer.
 * @return true if data was successfully read; false otherwise.
 */
bool read_from_pipe(Pipe *pipe, char *buffer, size_t buf_size);

/**
 * @brief Writes data to a pipe.
 * 
 * @param pipe Pointer to the Pipe object.
 * @param buffer Buffer containing the data to write.
 */
void write_to_pipe(Pipe *pipe, const char *buffer);




#endif