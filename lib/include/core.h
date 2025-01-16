#ifndef CORE_H
#define CORE_H

#include "defines.h"
#include <queue>

using namespace std;

class core {
    private:
        int m_coreID;
        float m_weight;
        bool m_active;
        int m_runs;   
        queue<int> m_scoreBuffer;

    public:
        core(int id, float weight, bool active, int runs);

        int get_coreID() { return m_coreID; }
        void set_coreID(int coreID) { m_coreID = coreID; }

        float get_weight() { return m_weight; }
        void set_weight(float weight) { m_weight = weight; }

        void update_weight(int result);
        
        // void increase_weight(bool result);
        // void decrease_weight(bool result);

        bool get_active() { return m_active; }
        void set_active(bool active) { m_active = active; }

        int get_runs() { return m_runs; }
        void set_runs(int runs) { m_runs = runs; }
        void increase_runs() { m_runs++; }
};

#endif

