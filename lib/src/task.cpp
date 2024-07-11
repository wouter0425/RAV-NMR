#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/select.h>
#include "defines.h"

#include "task.h"
#include "pipe.h"

task::task()
{

}

task::task(const string& name, int period, int offset, int priority, void (*function)(void))
{
    m_name = name;
    m_function = function;
    m_inputs = NULL;
    m_fails = 0;
    m_success = 0;
    m_active = false;
    m_replicate = false;
    m_voter = false;
    m_finished = false;
    m_period = period;
    m_offset = offset;
    m_startTime = 0;
    m_priority = priority;
    m_state = task_state::idle;

    // Set only cyclic tasks to be fireable
    period ? m_fireable = true : m_fireable = false;

    for (int i = 0; i < NUM_OF_CORES; i++)
        m_coreRuns[i] = 0;
}

bool task::offset_elapsed(unsigned long int startTime, unsigned long int currentTime)
{
    if (!m_offset)
        return true;
    else if (currentTime - startTime > m_offset)
        return true;
}

bool task::period_elapsed(unsigned long int currentTime)
{
    // Acyclic tasks exception
    if (!m_period) {        
        return true;
    }
    else if (currentTime - m_startTime > m_period) {                
        return true;
    }

    return false;
}

bool task::is_stuck(unsigned long int elapsedTime, int status, pid_t result)
{
    if (elapsedTime - m_startTime > MAX_STUCK_TIME  && m_latestResult == result && m_latestStatus == status)
        return true;

    return false;
}

voter::voter(const string& name, int period, int offset, int priority, void (*function)(void))
        : task(name, period, offset, priority, function)
{
    set_voter(true);
}

void voter::add_replicate(task *t)
{
    m_replicates.push_back(t);
    m_replicateMonitor.push_back({t->get_name(), false});
}

bool voter::check_replicate_state(task_state state)
{
    for (int i = 0; i < m_replicates.size(); i++) 
    {        
        if (m_replicates[i]->get_state() != state)
            return false;
    }
    return true;
}

bool voter::get_voter_fireable()
{
    // Check if the voter is armed
    if (!m_armed)
    {
        // Check and update the armed status of each replicate
        bool armed = true;
        for (size_t i = 0; i < m_replicates.size(); ++i)
        {
            if (m_replicates[i]->get_state() == task_state::running)
                m_replicateMonitor[i].armed = true;

            if (!m_replicateMonitor[i].armed)
                armed = false;
        }

        m_armed = armed;
        return false;
    }

    // Check if all replicates are no longer running
    for (size_t i = 0; i < m_replicates.size(); ++i)
    {
        if (m_replicates[i]->get_state() != task_state::running)
            m_replicateMonitor[i].armed = false;

        if (m_replicateMonitor[i].armed)
            return false;
    }

    // If all replicates are no longer running, reset armed state and return true
    m_armed = false;            
    return true;
}

#ifndef NMR
void task_A(void) {
    int value = 42;
    char buffer[BUF_SIZE];

    snprintf(buffer, sizeof(buffer), "%d", value);

    for (int i = 0; i < TASK_BUSY_TIME; i++)
        usleep(1);

    write_to_pipe(AB, buffer);

#ifdef DEBUG
    printf("task A: write: %d \n", value);
#endif

    exit(0);
}

void task_B(void) {
    char buffer[BUF_SIZE];

    // Read from pipe AB
    if (read_from_pipe(AB, buffer, BUF_SIZE)) {
        int value = atoi(buffer);
        value++;
        snprintf(buffer, sizeof(buffer), "%d", value);
        
        for (int i = 0; i < TASK_BUSY_TIME; i++)
            usleep(1);

        // Write to pipe BC
        write_to_pipe(BC, buffer);

#ifdef DEBUG
        printf("task B: read: %d \t write: %d \n", value - 1, value);
#endif
        exit(0);
    }

    exit(1);
}

void task_C(void) {
    char buffer[BUF_SIZE];

    if (read_from_pipe(BC, buffer, BUF_SIZE)) {
        int value = atoi(buffer);
        
        for (int i = 0; i < TASK_BUSY_TIME; i++)
            usleep(1);

#ifdef DEBUG
        printf("task C: read: %d \n", value);
#endif
        exit(0);
    }

#ifdef DEBUG
    printf("C crashed \n");
#endif
    exit(1);
}

#else
void task_A_1(void) {
    int value = 42;
    char buffer[BUF_SIZE];

    snprintf(buffer, sizeof(buffer), "%d", value);

    for (int i = 0; i < TASK_BUSY_TIME; i++)
        usleep(1);

    write_to_pipe(AB_1, buffer);
    write_to_pipe(AB_2, buffer);
    write_to_pipe(AB_3, buffer);

#ifdef DEBUG
    printf("task A: write: %d \n", value);
#endif

    exit(0);
}

void task_B_1(void) {
    char buffer[BUF_SIZE];

    // Read from pipe AB
    if (read_from_pipe(AB_1, buffer, BUF_SIZE)) {
        int value = atoi(buffer);
        value++;
        snprintf(buffer, sizeof(buffer), "%d", value);

        for (int i = 0; i < TASK_BUSY_TIME; i++)
            usleep(1);

        // Write to pipe BC
        write_to_pipe(BC_1, buffer);

 #ifdef DEBUG
        printf("task B: read: %d \t write: %d \n", value - 1, value);
#endif
        exit(0);        
    }

    exit(1);
}

void task_B_2(void) {
    char buffer[BUF_SIZE];

    // Read from pipe AB
    if (read_from_pipe(AB_2, buffer, BUF_SIZE)) {
        int value = atoi(buffer);
        value++;
        snprintf(buffer, sizeof(buffer), "%d", value);
        
        for (int i = 0; i < TASK_BUSY_TIME; i++)
            usleep(1);

        // Write to pipe BC
        write_to_pipe(BC_2, buffer);

#ifdef DEBUG
        printf("task B: read: %d \t write: %d \n", value - 1, value);
#endif
        exit(0);       
    }

    exit(1);
}

void task_B_3(void) {
    char buffer[BUF_SIZE];

    // Read from pipe AB
    if (read_from_pipe(AB_3, buffer, BUF_SIZE)) {
        int value = atoi(buffer);
        value++;
        snprintf(buffer, sizeof(buffer), "%d", value);
        
        for (int i = 0; i < TASK_BUSY_TIME; i++)
            usleep(1);

        // Write to pipe BC
        write_to_pipe(BC_3, buffer);

#ifdef DEBUG
        printf("task B: read: %d \t write: %d \n", value - 1, value);
#endif

        exit(0);
    }

    exit(1);
}

void task_C_1(void) {
    char buffer[BUF_SIZE];

    if (read_from_pipe(CD_1, buffer, BUF_SIZE)) {
        int value = atoi(buffer);
        
        for (int i = 0; i < TASK_BUSY_TIME; i++)
            usleep(1);

#ifdef DEBUG
    printf("task C: read: %d \n", value);
#endif

        exit(0);
    }

    exit(1);
}

void voter_func(void) {
    char buffer1[BUF_SIZE], buffer2[BUF_SIZE], buffer3[BUF_SIZE];
    char outputBuffer[BUF_SIZE];
    bool read1, read2, read3;

    read1 = read_from_pipe(BC_1, buffer1, BUF_SIZE);
    read2 = read_from_pipe(BC_1, buffer2, BUF_SIZE);
    read3 = read_from_pipe(BC_1, buffer3, BUF_SIZE);

    if (read1 && read2 && strcmp(buffer1, buffer2) == 0) {
        strncpy(outputBuffer, buffer1, BUF_SIZE);
    } else if (read1 && read3 && strcmp(buffer1, buffer3) == 0) {
        strncpy(outputBuffer, buffer1, BUF_SIZE);
    } else if (read2 && read3 && strcmp(buffer2, buffer3) == 0) {
        strncpy(outputBuffer, buffer2, BUF_SIZE);
    } else if (read1) {
        strncpy(outputBuffer, buffer1, BUF_SIZE);
    } else if (read2) {
        strncpy(outputBuffer, buffer2, BUF_SIZE);
    } else if (read3) {
        strncpy(outputBuffer, buffer3, BUF_SIZE);
    } else {
        // If no valid data is read, handle the error or set a default value
        //strncpy(outputBuffer, "Nop", BUF_SIZE);
    }

#ifdef DEBUG
    printf("Voter \n");
#endif

    write_to_pipe(CD_1, outputBuffer);

    exit(0);
}
#endif