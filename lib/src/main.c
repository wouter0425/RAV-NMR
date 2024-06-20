#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/select.h>

#include "scheduler.h"
#include "task.h"
#include "pipe.h"
#include "defines.h"

// Create the scheduler
scheduler s;

pipe_struct *AB;
pipe_struct *BC;
pipe_struct *CD;

pipe_struct *AB_1;
pipe_struct *AB_2;
pipe_struct *AB_3;

pipe_struct *BC_1;
pipe_struct *BC_2;
pipe_struct *BC_3;


int main()
{
#ifdef NMR
    // NMR operation
    AB_1 = declare_pipe("pipe_AB_1");
    AB_2 = declare_pipe("pipe_AB_2");
    AB_3 = declare_pipe("pipe_AB_3");

    BC_1 = declare_pipe("pipe_BC_1");
    BC_2 = declare_pipe("pipe_BC_2");
    BC_3 = declare_pipe("pipe_BC_3");

    CD = declare_pipe("pipe_CD");

    add_task(&s, 0, "task_A_1", task_A_1);
    add_task(&s, 1, "task_B_1", task_B_1);
    add_task(&s, 2, "task_B_2", task_B_2);
    add_task(&s, 3, "task_B_3", task_B_3);
    add_task(&s, 4, "voter", voter);
    add_task(&s, 5, "task_C_1", task_C_1);

    add_input(s.m_tasks[2].inputs, AB_2->read_fd);

    add_input(s.m_tasks[3].inputs, AB_3->read_fd);

    add_input(s.m_tasks[4].inputs, BC_1->read_fd);
    add_input(s.m_tasks[4].inputs, BC_2->read_fd);
    add_input(s.m_tasks[4].inputs, BC_3->read_fd);

    add_input(s.m_tasks[5].inputs, CD->read_fd);
#else
    // Normal operation
    AB = declare_pipe("pipe_AB");
    BC = declare_pipe("pipe_BC");

    // Create the tasks
    add_task(&s, 0, "task_A", task_A);
    add_task(&s, 1, "task_B", task_B);
    add_task(&s, 2, "task_C", task_C);

    add_input(s.m_tasks[1].inputs, AB->read_fd);
    add_input(s.m_tasks[2].inputs, BC->read_fd);
#endif

    init_scheduler(&s);

    start_scheduler(&s);

    printResults(&s);

    cleanup_tasks(&s);

    exit(0);
    return 0;
}