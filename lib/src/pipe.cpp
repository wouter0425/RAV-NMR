#include <sys/select.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string>

#include <pipe.h>
//#include <task.h>
#include <voter.h>

using namespace std;

Pipe::Pipe()
{

}

Pipe::Pipe(int read_fd, int write_fd, const char* name) 
{
    m_read_fd = read_fd;
    m_write_fd = write_fd;
    m_name = strdup(name);    
}

Pipe* Pipe::declare_pipe(const char* name) 
{
    int pipe_fds[2];
    if (pipe(pipe_fds) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
        return nullptr;
    }

    Pipe* p = new Pipe(pipe_fds[0], pipe_fds[1], name);
    return p;
}

// void add_input(input **inputs, int fd, int size) 
// {
//     input *new_input = (input *)malloc(sizeof(input));

//     if (new_input == NULL) 
//     {
//         perror("Failed to allocate memory for new input");
//         exit(EXIT_FAILURE);
//     }

//     new_input->fd = fd;
//     new_input->next = NULL;
//     new_input->size = size;

//     if (*inputs == NULL)
//         *inputs = new_input;
//     else
//     {
//         input *current = *inputs;
//         while (current->next != NULL)
//             current = current->next;

//         current->next = new_input;
//     }
// }



bool Pipe::read_data(char *buffer, size_t buf_size)
{
    //close(pipe->get_write_fd());
    close(m_write_fd);

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(get_read_fd(), &read_fds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    
    int ready = select(m_read_fd + 1, &read_fds, NULL, NULL, &timeout);    

    if (ready > 0) 
    {
        ssize_t num_bytes = read(m_read_fd, buffer, buf_size - 1);
        if (num_bytes > 0)
        {
            buffer[num_bytes] = '\0';
            close(m_read_fd);
            return true;
        }
    }
    
    close(m_read_fd);
    return false;
}

// bool read_from_pipe(Pipe *pipe, char *buffer, size_t buf_size)
// {    
//     close(pipe->get_write_fd());

//     fd_set read_fds;
//     FD_ZERO(&read_fds);
//     FD_SET(pipe->get_read_fd(), &read_fds);

//     struct timeval timeout;
//     timeout.tv_sec = 0;
//     timeout.tv_usec = 0;

//     int ready = select(pipe->get_read_fd() + 1, &read_fds, NULL, NULL, &timeout);

//     if (ready > 0) 
//     {
//         ssize_t num_bytes = read(pipe->get_read_fd(), buffer, buf_size - 1);
//         if (num_bytes > 0)
//         {
//             buffer[num_bytes] = '\0';
//             close(pipe->get_read_fd());
//             return true;
//         }
//     }
    
//     close(pipe->get_read_fd());
//     return false;
// }

void Pipe::write_data(const char *buffer) 
{    
    close(m_read_fd);

    write(m_write_fd, buffer, strlen(buffer) + 1);

    close(m_write_fd);
}

//void write_to_pipe(Pipe *pipe, const char *buffer);
