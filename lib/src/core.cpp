#include "core.h"

core::core(int id, float weight, bool active, int runs)
{
    m_coreID = id;
    m_weight = weight;
    m_active = active;
    m_runs = runs;

    for (int i = 0; i < CORE_BUFFER_SIZE; i++)
    {
        m_scoreBuffer.push(int(100 / CORE_BUFFER_SIZE));
    }
}

void core::update_weight(int result)
{
    m_weight -= m_scoreBuffer.front();
    m_scoreBuffer.pop();

    m_weight += result;
    m_scoreBuffer.push(result);
}