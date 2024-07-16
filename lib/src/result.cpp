#include <result.h>

result::result(vector<task*> tasks, vector<core*> cores, long mSeconds)
{
    for (long unsigned int i = 0; i < cores.size(); i++)
    {
        m_cores.push_back(cores[i]->get_runs());
    }

    for (long unsigned int i = 0; i < cores.size(); i++)
    {
        m_weights.push_back(cores[i]->get_weight());
    }

    for (long unsigned int i = 0; i < tasks.size(); i++)
    {
        m_tasks.push_back(tasks[i]->get_success());
    }

    m_time = mSeconds;
}