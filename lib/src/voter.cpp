#include <voter.h>

voter::voter(const string& name, int period, int offset, int priority, void (*function)(void), voter_type type)
        : task(name, period, offset, priority, function)
{
    set_voter(true);
    set_voter_type(type);
}

voter* voter::declare_voter(const string& name, int period, int offset, int priority, void (*function)(void), voter_type type)
{
    voter* v = new voter(name, period, offset, priority,function, type);
    return v;
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
    int at_least_one_survivor = false;

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
        {
            m_replicateMonitor[i].armed = false;

            if (m_replicates[i]->get_state() != task_state::crashed)
            {
                at_least_one_survivor = true;
            }
        }

        if (m_replicateMonitor[i].armed)
            return false;
    }

    // Check if there is atleast one replicate with input
    if (!at_least_one_survivor)
        return false;

    // If all replicates are no longer running, reset armed state and return true
    m_armed = false;
    return true;
}