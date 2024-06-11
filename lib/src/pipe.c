#include  <sys/select.h>
#include <unistd.h>

#include "pipe.h"
#include "task.h"

pipe_struct *declare_pipe(const char *pipe_name) {
    int pipe_fds[2];
    if (pipe(pipe_fds) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pipe_struct *new_pipe = (pipe_struct *)malloc(sizeof(pipe_struct));
    if (new_pipe == NULL) {
        perror("Failed to allocate memory for new pipe");
        exit(EXIT_FAILURE);
    }

    new_pipe->read_fd = pipe_fds[0];
    new_pipe->write_fd = pipe_fds[1];
    new_pipe->name = strdup(pipe_name); // Allocate and copy the pipe name
    new_pipe->next = NULL;

    return new_pipe;
}

// Function to find a pipe by name
// pipe_struct *find_pipe_by_name(task *t, const char *pipe_name) {
//     pipe_struct *current = t->pipes;
//     while (current != NULL) {
//         if (strcmp(current->name, pipe_name) == 0) {
//             return current;
//         }
//         current = current->next;
//     }
//     return NULL;
// }

pipe_struct *find_pipe_by_name(pipe_struct *pipes, const char *pipe_name) {
    pipe_struct *current = pipes;
    while (current != NULL) {
        if (strcmp(current->name, pipe_name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Function to add a pipe to a task's linked list
void add_pipe(task *t, pipe_struct *new_pipe) {
    if (t->pipes == NULL) {
        t->pipes = new_pipe;
    } else {
        pipe_struct *current = t->pipes;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_pipe;
    }
}

// Function to close all pipes in a task's linked list
// void close_pipes(task *t) {
//     pipe_struct *current = t->pipes;
//     while (current != NULL) {
//         close(current->read_fd);
//         close(current->write_fd);
//         free(current->name);
//         pipe_struct *to_free = current;
//         current = current->next;
//         free(to_free);
//     }
//     t->pipes = NULL;
// }

bool is_pipe_content_available(int pipe_fd) {
    // Create a set of file descriptors
    fd_set read_fds;
    FD_ZERO(&read_fds);        // Clear the set
    FD_SET(pipe_fd, &read_fds); // Add the pipe file descriptor to the set

    // Create a timeval struct for the timeout (0 seconds, 0 microseconds)
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    // Use select to check for available data in the pipe
    int result = select(pipe_fd + 1, &read_fds, NULL, NULL, &timeout);

    if (result > 0) {
        // If result is positive, data is available to read
        return true;
    } else if (result == 0) {
        // If result is zero, no data is available within the timeout
        return false;
    } else {
        // If result is negative, an error occurred
        perror("select");
        return false;
    }
}

// Function to close a specific pipe by name
// void close_pipe_by_name(task *t, const char *pipe_name) {
//     pipe_struct *prev = NULL;
//     pipe_struct *current = t->pipes;

//     while (current != NULL) {
//         if (strcmp(current->name, pipe_name) == 0) {
//             if (prev == NULL) {
//                 t->pipes = current->next;
//             } else {
//                 prev->next = current->next;
//             }
//             close(current->read_fd);
//             close(current->write_fd);
//             free(current->name);
//             free(current);
//             return;
//         }
//         prev = current;
//         current = current->next;
//     }
// }

// Function to reopen a specific pipe by name
// void open_pipe_by_name(task *t, const char *pipe_name) {
//     pipe_struct *current = find_pipe_by_name(t, pipe_name);
//     if (current == NULL) {
//         fprintf(stderr, "Pipe %s not found\n", pipe_name);
//         return;
//     }

//     int pipe_fds[2];
//     if (pipe(pipe_fds) == -1) {
//         perror("pipe");
//         exit(EXIT_FAILURE);
//     }

//     current->read_fd = pipe_fds[0];
//     current->write_fd = pipe_fds[1];
// }