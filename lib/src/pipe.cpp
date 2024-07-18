#include <sys/select.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string>

#include <pipe.h>
#include <task.h>
#include <voter.h>

using namespace std;

Pipe::Pipe(int read_fd, int write_fd, const char* name, Pipe *next) 
{
    m_read_fd = read_fd;
    m_write_fd = write_fd;
    m_name = strdup(name);
    m_next = next;
}

Pipe *declare_pipe(const char *pipe_name) 
{
    int pipe_fds[2];
    if (pipe(pipe_fds) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    Pipe *p = new Pipe(pipe_fds[0], pipe_fds[1], pipe_name, NULL);

    return p;
}

void add_input(input **inputs, int fd) 
{
    input *new_input = (input *)malloc(sizeof(input));

    if (new_input == NULL) 
    {
        perror("Failed to allocate memory for new input");
        exit(EXIT_FAILURE);
    }

    new_input->fd = fd;
    new_input->next = NULL;

    if (*inputs == NULL)
        *inputs = new_input;
    else
    {
        input *current = *inputs;
        while (current->next != NULL)
            current = current->next;

        current->next = new_input;
    }
}

bool task_input_full(task *t) 
{
    if (t->get_voter()) 
    {
        voter* v = static_cast<voter*>(t);        
        if (!v)
            return false;

        return v->get_voter_fireable();
    } 
    else
    {
        fd_set read_fds;
        struct timeval timeout;
        int max_fd = 0;
        input *current = t->get_inputs();

        FD_ZERO(&read_fds);
        
        while (current != NULL) 
        {
            if (current->fd < 0) 
            {
                fprintf(stderr, "Invalid file descriptor: %d\n", current->fd);                
                return false;
            }

            FD_SET(current->fd, &read_fds);

            if (current->fd > max_fd)
                max_fd = current->fd;

            current = current->next;
        }
        
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        
        int result = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);

        if (result < 0)
        {
            perror("select");
            return false;
        }
        
        current = t->get_inputs();

        while (current != NULL) 
        {
            if (!FD_ISSET(current->fd, &read_fds))
                return false;

            current = current->next;
        }

    }

    return true;
}

void open_pipe_read_end(Pipe *pipe) {
    close(pipe->get_write_fd());
}

void open_pipe_write_end(Pipe *pipe) {
    close(pipe->get_read_fd());
}

void close_pipe_read_end(Pipe *pipe) {
    close(pipe->get_read_fd());
}

void close_pipe_write_end(Pipe *pipe) {
    close(pipe->get_write_fd());
}

bool read_from_pipe(Pipe *pipe, char *buffer, size_t buf_size)
{
    open_pipe_read_end(pipe);

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(pipe->get_read_fd(), &read_fds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    int ready = select(pipe->get_read_fd() + 1, &read_fds, NULL, NULL, &timeout);

    if (ready > 0) 
    {
        ssize_t num_bytes = read(pipe->get_read_fd(), buffer, buf_size - 1);
        if (num_bytes > 0)
        {
            buffer[num_bytes] = '\0';
            close_pipe_read_end(pipe);
            return true;
        }
    }
    
    close_pipe_read_end(pipe);
    return false;
}


void write_to_pipe(Pipe *pipe, const char *buffer) 
{
    open_pipe_write_end(pipe);

    write(pipe->get_write_fd(), buffer, strlen(buffer) + 1);

    close_pipe_write_end(pipe);
}
