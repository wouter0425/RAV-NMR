#include  <sys/select.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

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
    new_pipe->name = strdup(pipe_name);
    new_pipe->next = NULL;

    return new_pipe;
}

void add_input(input **inputs, int fd) {
    input *new_input = (input *)malloc(sizeof(input));

    if (new_input == NULL) {
        perror("Failed to allocate memory for new input");
        exit(EXIT_FAILURE);
    }

    new_input->fd = fd;
    new_input->next = NULL;

    if (*inputs == NULL) {
        // Update the head pointer if the list is empty
        *inputs = new_input;
    } else {
        input *current = *inputs;
        while (current->next != NULL) {
            current = current->next;
        }
        // Append the new input to the end of the list
        current->next = new_input;
    }
}

bool task_input_full(task *t) {
    fd_set read_fds;
    struct timeval timeout;
    int max_fd = 0;
    input *current = t->inputs;

    // Initialize the set of file descriptors to be checked.
    FD_ZERO(&read_fds);

    //Add each file descriptor to the set and find the maximum file descriptor value.
    while (current != NULL) {
        FD_SET(current->fd, &read_fds);
        if (current->fd > max_fd) {
            max_fd = current->fd;
        }
        current = current->next;
    }

    //Set the timeout value (e.g., 0 seconds and 0 microseconds means no waiting).
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    //Use select to check if data is available on any of the file descriptors.
    int result = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);

    if (result < 0) {
        perror("select");
        return false;
    }

    // Check if all file descriptors are ready.
    current = t->inputs;

    while (current != NULL) {
        if (!FD_ISSET(current->fd, &read_fds)) {
            //printf("FD_ISSET: %d \n", FD_ISSET(current->fd, &read_fds));
            //printf("not ready: %s \n", t->name);
            return false;
        }
        current = current->next;
    }
    //printf("ready: %s \n", t->name);
    return true;
}

void open_pipe_read_end(pipe_struct *pipe) {
    close(pipe->write_fd);  // Close the write end if it's open
}

void open_pipe_write_end(pipe_struct *pipe) {
    close(pipe->read_fd);
}

void close_pipe_read_end(pipe_struct *pipe) {
    close(pipe->read_fd);
}

void close_pipe_write_end(pipe_struct *pipe) {
    close(pipe->write_fd);
}

bool read_from_pipe(pipe_struct *pipe, char *buffer, size_t buf_size) {
    // Ensure the write end is closed
    open_pipe_read_end(pipe);

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(pipe->read_fd, &read_fds);

    // Wait until the pipe is ready for reading
    int ready = select(pipe->read_fd + 1, &read_fds, NULL, NULL, NULL);
    if (ready > 0) {
        // Leave space for null terminator
        ssize_t num_bytes = read(pipe->read_fd, buffer, buf_size - 1);
        if (num_bytes > 0) {
            // Null-terminate the string
            buffer[num_bytes] = '\0';

            // Close the read end after use
            close_pipe_read_end(pipe);
            return true;
        }
    }

    // Close the read end if no data is read
    close_pipe_read_end(pipe);
    return false;
}

void write_to_pipe(pipe_struct *pipe, const char *buffer) {
    // Ensure the read end is closed
    open_pipe_write_end(pipe);

    // Include the null terminator
    write(pipe->write_fd, buffer, strlen(buffer) + 1);

    // Close the write end after use
    close_pipe_write_end(pipe);
}