#ifndef RESULT_H
#define RESULT_H

#include <vector>
#include <task.h>
#include <core.h>

using namespace std;

class result {
    private:

    public:
        long m_time;
        vector<int> m_cores;
        vector<float> m_weights;
        vector<int> m_tasks;
        result(vector<task*> tasks, vector<core*> cores, long mSeconds);
};

#endif