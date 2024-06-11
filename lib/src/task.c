#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/select.h>
#include "defines.h"

#include "task.h"
#include "pipe.h"

void task_A(struct pipe_struct *pipes) {
    pipe_struct* a = find_pipe_by_name(pipes, "pipe_AB");

    close(a->read_fd);


    int value = 42; // Initial value
    char message[BUF_SIZE];
    snprintf(message, sizeof(message), "%d", value);
    write(a->write_fd, message, strlen(message));
    printf("Task A (PID %d) sent value: %d\n", getpid(), value);

    close(a->write_fd);

    exit(0);    
}

//void task_B(int read_fd, int write_fd) {
void task_B(struct pipe_struct *pipes) {
    pipe_struct* a = find_pipe_by_name(pipes, "pipe_AB");
    pipe_struct* b = find_pipe_by_name(pipes, "pipe_BC");

    close(a->write_fd);
    close(b->read_fd);


    char buffer[BUF_SIZE];
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(a->read_fd, &read_fds);

    // Wait for data to be available in the pipe
    if (select(a->read_fd + 1, &read_fds, NULL, NULL, NULL) > 0) {
        if (FD_ISSET(a->read_fd, &read_fds)) {
        ssize_t num_bytes = read(a->read_fd, buffer, BUF_SIZE);
        if (num_bytes > 0) {
            buffer[num_bytes] = '\0'; // Null-terminate the string
            int value = atoi(buffer);
            value++;
            snprintf(buffer, sizeof(buffer), "%d", value);
            write(b->write_fd, buffer, strlen(buffer));
            printf("Task B (PID %d) received value: %d, incremented to: %d\n", getpid(), value-1, value);
        }
        }
    }

    close(a->read_fd);
    close(b->write_fd);

    exit(0);  
}

void task_C(struct pipe_struct *pipes) {
    char buffer[BUF_SIZE];

    pipe_struct* b = find_pipe_by_name(pipes, "pipe_BC");

    close(b->write_fd);

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(b->read_fd, &read_fds);

    // Wait for data to be available in the pipe
    if (select(b->read_fd + 1, &read_fds, NULL, NULL, NULL) > 0) {
        if (FD_ISSET(b->read_fd, &read_fds)) {
            ssize_t num_bytes = read(b->read_fd, buffer, BUF_SIZE);
            if (num_bytes > 0) {
                buffer[num_bytes] = '\0'; // Null-terminate the string
                int value = atoi(buffer);
                printf("Task C (PID %d) received final value: %d\n", getpid(), value);
            }
        }
    }

    close(b->read_fd);

    exit(0);    
}