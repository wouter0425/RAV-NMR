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

task::task(const string& name, unsigned long int period, unsigned long int offset, int priority, void (*function)(void))
{
    m_name = name;
    m_function = function;    
    m_period = period;
    m_offset = offset;
    m_priority = priority;
    m_state = task_state::idle;

    // Set only cyclic tasks to be fireable
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
    // Acyclic tasks exception
    if (!m_period || currentTime - m_startTime > m_period) {        
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
