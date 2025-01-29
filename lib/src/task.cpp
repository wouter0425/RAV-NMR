#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <pthread.h>
#include <unistd.h>
#include <sys/select.h>
#include "defines.h"

#include "task.h"
#include "voter.h"

task::task(const string& name, unsigned long int period, unsigned long int offset, int priority, void (*function)(void))
{
    m_name = name;
    m_function = function;    
    m_period = period;
    m_offset = offset;
    m_priority = priority;
    m_state = task_state::idle;

    // Set only cyclic tasks to be fireable from the start
    period ? m_fireable = true : m_fireable = false;

    for (int i = 0; i < NUM_OF_CORES; i++)
        m_coreRuns[i] = 0;
}

bool task::offset_elapsed(unsigned long int startTime, unsigned long int currentTime)
{
    if (!m_offset || currentTime - startTime > m_offset)
        return true;
    
    return false;
}

bool task::period_elapsed(unsigned long int currentTime)
{
    if (!m_period || currentTime - m_startTime > m_period)
        return true;

    return false;
}

bool task::is_stuck(unsigned long int elapsedTime, int status, pid_t result)
{
    if (elapsedTime - m_startTime > MAX_STUCK_TIME  && m_latestResult == result && m_latestStatus == status)
        return true;

    return false;
}

void task::add_input(Pipe *p, int size) 
{
    input *new_input = (input *)malloc(sizeof(input));

    if (new_input == NULL) 
    {
        perror("Failed to allocate memory for new input");
        exit(EXIT_FAILURE);
    }

    new_input->fd = p->get_read_fd();
    new_input->next = NULL;
    new_input->size = size;

    if (m_inputs == NULL)
        m_inputs = new_input;
    else
    {
        input *current = m_inputs;
        while (current->next != NULL)
            current = current->next;

        current->next = new_input;
    }
}

task* task::declare_task(const string& name, unsigned long int period, unsigned long int offset, int priority, void (*function)(void))
{
    task* t  = new task(name, period, offset, priority, function);
    return t;
}

bool task::task_input_full(task *t) 
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

void task::print_core_runs() 
{            
    for (int i = 0; i < NUM_OF_CORES; i++)
        printf("Core %d: %d \t", i, m_coreRuns[i]);
    
    printf("\n");
}

string task::write_core_runs() const 
{
    ostringstream result;
    
    for (int i = 0; i < NUM_OF_CORES; i++) {
        result << "Core " << i << ": " << m_coreRuns[i] << "\t";
    }

    result << "\n";
    return result.str();
}