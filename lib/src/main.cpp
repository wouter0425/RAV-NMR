#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string>
#include <signal.h>
#include <sys/select.h>

#include "scheduler.h"
#include "task.h"
#include "pipe.h"
#include "defines.h"

// Create the scheduler
scheduler s;

#ifndef NMR
Pipe *AB;
Pipe *BC;
#else
Pipe *AB_1;
Pipe *AB_2;
Pipe *AB_3;

Pipe *BC_1;
Pipe *BC_2;
Pipe *BC_3;

Pipe *CD_1;
#endif

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

    CD_1 = declare_pipe("pipe_CD");

    s.add_task("task_A_1", 1000, task_A_1); // 0
    s.add_task("task_B_1", 0, task_B_1);    // 1
    s.add_task("task_B_2", 0, task_B_2);    // 2
    s.add_task("task_B_3", 0, task_B_3);    // 3
    s.add_voter("voter", 0, voter_func);    // 4
    s.add_task("task_C_1", 0, task_C_1);    // 5

    add_input(&s.find_task("task_B_1")->get_inputs_ref(), AB_1->get_read_fd());
    add_input(&s.find_task("task_B_2")->get_inputs_ref(), AB_2->get_read_fd());
    add_input(&s.find_task("task_B_3")->get_inputs_ref(), AB_3->get_read_fd());
    add_input(&s.find_task("task_C_1")->get_inputs_ref(), CD_1->get_read_fd());

    voter* v = static_cast<voter*>(s.find_task("voter"));

    v->add_replicate(s.find_task("task_A_1"));
    v->add_replicate(s.find_task("task_A_2"));
    v->add_replicate(s.find_task("task_A_3"));

#else
    // Normal operation
    AB = declare_pipe("pipe_AB");
    BC = declare_pipe("pipe_BC");

    // Create the tasks
    s.add_task("task_A", 1000, task_A);
    s.add_task("task_B", 0, task_B);
    s.add_task("task_C", 0, task_C);

    // Set input for tasks
    add_input(&s.find_task("task_B")->get_inputs_ref(), AB->get_read_fd());
    //printf("task B: %d \n", AB->get_read_fd());

    add_input(&s.find_task("task_C")->get_inputs_ref(), BC->get_read_fd());
    //printf("task C: %d \n", BC->get_read_fd());
#endif

    s.init_scheduler();

    // Scheduler loop
    while(s.active())
    {
        s.monitor_tasks();
        s.run_tasks();
#ifdef LOGGING
        s.log_results();
#endif        
    }

#ifdef LOGGING
    s.write_results_to_csv();
#endif

    s.printResults();

    s.cleanup_tasks();

    exit(0);
    return 0;
}