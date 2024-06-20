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

void add_input(input *inputs, int fd)
{
    input *new_input = (input *)malloc(sizeof(input));

    if (new_input == NULL) {
        perror("Failed to allocate memory for new pipe");
        exit(EXIT_FAILURE);
    }

    new_input->fd = fd;
    new_input->next = NULL;

    if (inputs == NULL) {
        inputs = new_input;
    } else {
        input *current = inputs;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_input;
    }
}

int count_content(pipe_struct *p)
{
    int counter = 0;

    pipe_struct *current = p;
    counter = 0;
    while (current != NULL) {
        current = current->next;
        counter++;
    }

    return counter;
}

void freeInputs(input *i)
{
    input* tmp;

    while (i != NULL)
    {
        tmp = i;
        i = i->next;
        free(tmp);
    }
}

void freePipes(pipe_struct *p)
{
    pipe_struct* tmp;

    while (p != NULL)
    {
        tmp = p;
        p = p->next;
        free(tmp);
    }
}

bool is_pipe_content_available(int pipe_fd) {
    int bytes_available;
    if (ioctl(pipe_fd, FIONREAD, &bytes_available) < 0) {
        perror("ioctl");
        return false;
    }
    return bytes_available > 0;
}

bool task_input_full(task *t) {
    input *current = t->inputs;
    int counter = 0;
    while (current != NULL) {
        if (!is_pipe_content_available(current->fd)) {
            return false;
        }
        current = current->next;
        counter++;
    }
    return true;
}

void open_pipe_read_end(pipe_struct *pipe) {
    close(pipe->write_fd);  // Close the write end if it's open
}

void open_pipe_write_end(pipe_struct *pipe) {
    close(pipe->read_fd);   // Close the read end if it's open
}

void close_pipe_read_end(pipe_struct *pipe) {
    close(pipe->read_fd);
}

void close_pipe_write_end(pipe_struct *pipe) {
    close(pipe->write_fd);
}

bool read_from_pipe(pipe_struct *pipe, char *buffer, size_t buf_size) {
    open_pipe_read_end(pipe);  // Ensure the write end is closed

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(pipe->read_fd, &read_fds);

    // Wait until the pipe is ready for reading
    int ready = select(pipe->read_fd + 1, &read_fds, NULL, NULL, NULL);
    if (ready > 0) {
        ssize_t num_bytes = read(pipe->read_fd, buffer, buf_size - 1); // Leave space for null terminator
        if (num_bytes > 0) {
            buffer[num_bytes] = '\0'; // Null-terminate the string
            close_pipe_read_end(pipe);  // Close the read end after use
            return true;
        }
    }

    close_pipe_read_end(pipe);  // Close the read end if no data is read
    return false;
}

void write_to_pipe(pipe_struct *pipe, const char *buffer) {
    open_pipe_write_end(pipe);  // Ensure the read end is closed

    write(pipe->write_fd, buffer, strlen(buffer) + 1); // Include the null terminator

    close_pipe_write_end(pipe);  // Close the write end after use
}