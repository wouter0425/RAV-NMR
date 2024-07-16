#include <voter.h>

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
    for (auto& replicate : m_replicates) 
    {        
        if (replicate->get_state() != state)
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