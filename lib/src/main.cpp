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
#include <flight_controller.h>

/* Pipes have to be declared in the global scope*/
#if defined(NMR) || defined(RAVNMR)

Pipe *AB_1;
Pipe *AB_2;
Pipe *AB_3;

Pipe *BV_1;
Pipe *BV_2;
Pipe *BV_3;

Pipe *VC;

#else

Pipe *AB;
Pipe *BC;

#endif


int main()
{
#if defined(NMR)
    /* Initialize the scheduler */
    scheduler* s = scheduler::declare_scheduler("NMR");
    s->init_scheduler();

    /* Declare the pipes */
    AB_1 = Pipe::declare_pipe("pipe_AB_1");    
    AB_2 = Pipe::declare_pipe("pipe_AB_2");    
    AB_3 = Pipe::declare_pipe("pipe_AB_3");    
    BV_1 = Pipe::declare_pipe("pipe_BV_1");    
    BV_2 = Pipe::declare_pipe("pipe_BV_2");    
    BV_3 = Pipe::declare_pipe("pipe_BV_3");    
    VC = Pipe::declare_pipe("pipe_VC");

    /* Declare the tasks */
    task* task_A_1 = task::declare_task("task_A_1", 150, 0, 0, read_sensors);
    task* task_B_1 = task::declare_task("task_B_1", 0, 0, 1, process_data_1);
    task* task_B_2 = task::declare_task("task_B_2", 0, 0, 1, process_data_2);    
    task* task_B_3 = task::declare_task("task_B_3", 0, 0, 1, process_data_3);
    task* task_C_1 = task::declare_task("task_C_1", 0, 0, 2, control_actuators);

    /* Setup the task inputs */
    task_B_1->add_input(AB_1, 4);
    task_B_2->add_input(AB_2, 4);
    task_B_3->add_input(AB_3, 4);
    task_C_1->add_input(VC, 4);

    /* Create the voter and add replicates */
    voter* v = voter::declare_voter("voter", 0, 0, 3, majority_voter, voter_type::standard);
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
#elif defined(RAVNMR)
    /* Initialize the scheduler */
    scheduler* s = scheduler::declare_scheduler("RAV-NMR");
    s->init_scheduler();

    /* Declare the pipes */
    AB_1 = Pipe::declare_pipe("pipe_AB_1");    
    AB_2 = Pipe::declare_pipe("pipe_AB_2");    
    AB_3 = Pipe::declare_pipe("pipe_AB_3");    
    BV_1 = Pipe::declare_pipe("pipe_BV_1");    
    BV_2 = Pipe::declare_pipe("pipe_BV_2");    
    BV_3 = Pipe::declare_pipe("pipe_BV_3");    
    VC = Pipe::declare_pipe("pipe_VC");

    /* Declare the tasks */
    task* task_A_1 = task::declare_task("task_A_1", 150, 0, 0, read_sensors);
    task* task_B_1 = task::declare_task("task_B_1", 0, 0, 1, process_data_1);
    task* task_B_2 = task::declare_task("task_B_2", 0, 0, 1, process_data_2);    
    task* task_B_3 = task::declare_task("task_B_3", 0, 0, 1, process_data_3);
    task* task_C_1 = task::declare_task("task_C_1", 0, 0, 2, control_actuators);

    /* Setup the task inputs */
    task_B_1->add_input(AB_1, 4);
    task_B_2->add_input(AB_2, 4);
    task_B_3->add_input(AB_3, 4);
    task_C_1->add_input(VC, 4);

    /* Create the voter and add replicates */
    voter* v = voter::declare_voter("voter", 0, 0, 3, majority_voter, voter_type::weighted);
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
#else
    /* Initialize the scheduler */
    scheduler* s = scheduler::declare_scheduler("baseline");
    s->init_scheduler();

    /* Declare the pipes */
    AB = Pipe::declare_pipe("pipe_AB");
    BC = Pipe::declare_pipe("pipe_BC");

    /* Declare the tasks */
    task* task_A = task::declare_task("task_A", 150, 0, 0, read_sensors);
    task* task_B = task::declare_task("task_B", 0, 0, 1, process_data);
    task* task_C = task::declare_task("task_C", 0, 0, 2, control_actuators);

    /* Setup the task inputs */
    task_B->add_input(AB, 4);
    task_C->add_input(BC, 4);

    /* Add tasks to the scheduler */
    s->add_task(task_A);
    s->add_task(task_B);
    s->add_task(task_C);
#endif

    /* Start the scheduler loop */
    s->start_scheduler();

    /* Optional: write the result in .tsv format */
    s->write_results_to_tsv();    

    /* Free any allocated memory */
    s->cleanup_scheduler();

    return 0;
}
