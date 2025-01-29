#ifndef VOTER_H
#define VOTER_H

#include <vector>
#include <task.h>

using namespace std;

enum voter_type {
    standard,
    weighted
};

class voter : public task {
    private:
        vector<task*> m_replicates;
        vector<replicate> m_replicateMonitor;
        bool m_armed {false};
        voter_type m_voter_type;

    public:
        voter(const string& name, int period, int offset, int priority, void (*function)(void), voter_type type);
        static voter* declare_voter(const string& name, int period, int offset, int priority, void (*function)(void), voter_type type);
        bool check_replicate_state(task_state state);
        void add_replicate(task *t);
        bool get_voter_fireable();
        void set_armed(bool armed) { m_armed = armed; }
        bool get_armed() { return m_armed; }
        void set_voter_type(voter_type type) { m_voter_type = type; }
        voter_type get_voter_type() { return m_voter_type; }
};

#endif