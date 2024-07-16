#ifndef VOTER_H
#define VOTER_H

#include <vector>
#include <task.h>

using namespace std;

class voter : public task {
    private:
        vector<task*> m_replicates;
        vector<replicate> m_replicateMonitor;
        bool m_armed {false};

    public:
        voter(const string& name, int period, int offset, int priority, void (*function)(void));
        bool check_replicate_state(task_state state);
        void add_replicate(task *t);
        bool get_voter_fireable();
        void set_armed(bool armed) { m_armed = armed; }
        bool get_armed() { return m_armed; }
};

#endif