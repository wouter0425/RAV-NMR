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
#include <signal.h>

#include <scheduler.h>
#include <task.h>
#include <voter.h>
#include <pipe.h>
#include <defines.h>
#include <user_functions.h>

void handle_signal(int sig);

/* Pipes have to be declared in the global scope*/
Pipe *AB_1;
Pipe *AB_2;
Pipe *AB_3;

Pipe *BC_1;
Pipe *BC_2;
Pipe *BC_3;

Pipe *CD_1;

scheduler* s;

int main()
{
    // Register the signal handler for SIGINT
    signal(SIGINT, handle_signal);

    /* Initialize the scheduler */
    s = scheduler::declare_scheduler();
    s->setOutputDirectory("weightedProof");
    s->init_scheduler();

    /* Declare the pipes */
    AB_1 = Pipe::declare_pipe("pipe_AB_1");    
    AB_2 = Pipe::declare_pipe("pipe_AB_2");    
    AB_3 = Pipe::declare_pipe("pipe_AB_3");    
    BC_1 = Pipe::declare_pipe("pipe_BC_1");    
    BC_2 = Pipe::declare_pipe("pipe_BC_2");    
    BC_3 = Pipe::declare_pipe("pipe_BC_3");    
    CD_1 = Pipe::declare_pipe("pipe_CD");

    /* Declare the tasks */
    task* task_A_1 = task::declare_task("task_A_1", 150, 0, 0, read_sensors);
    task* task_B_1 = task::declare_task("task_B_1", 0, 0, 1, process_data_1);
    task* task_B_2 = task::declare_task("task_B_2", 0, 0, 1, process_data_2);    
    task* task_B_3 = task::declare_task("task_B_3", 0, 0, 1, process_data_3);
    task* task_C_1 = task::declare_task("task_C_1", 0, 0, 2, control_actuators);

    /* Setup the task inputs */
    task_C_1->add_input(CD_1, 4);
    task_B_1->add_input(AB_1, 4);
    task_B_3->add_input(AB_3, 4);
    task_B_2->add_input(AB_2, 4);

    /* Create the voter and add replicates */
    voter* v = voter::declare_voter("voter", 0, 0, 1, majority_voter, voter_type::weighted);
    v->add_replicate(task_B_1);
    v->add_replicate(task_B_2);
    v->add_replicate(task_B_3);

    /* Add tasks to the scheduler */
    s->add_task(task_A_1);
    s->add_task(task_B_1);
    s->add_task(task_B_2);
    s->add_task(task_B_3);
    s->add_task(v);   
    s->add_task(task_C_1);

    // No NMR
    // AB_1 = Pipe::declare_pipe("pipe_AB_1");    
    // CD_1 = Pipe::declare_pipe("pipe_CD_1");

    // task* task_A = task::declare_task("task_A", 150, 0, 0, read_sensors_0);
    // task* task_B = task::declare_task("task_B", 0, 0, 1, process_data_0);
    // task* task_C = task::declare_task("task_C", 0, 0, 2, control_actuators);

    // task_B->add_input(AB_1, 4);
    // task_C->add_input(CD_1, 4);

    // s->add_task(task_A);
    // s->add_task(task_B);
    // s->add_task(task_C);
    

    // Scheduler loop
    while(s->active())
    {
        s->monitor_tasks();
        s->run_tasks();
        s->log_results();
    }

    s->write_results_to_csv();

    s->printResults();

    s->cleanup_tasks();

    return 0;
}

void handle_signal(int sig) 
{
    s->printResults();
    s->write_results_to_csv();

    if (sig == SIGINT) 
    {
        printf("Scheduler received SIGINT, shutting down...\n");
        exit(EXIT_SUCCESS);
    }
}