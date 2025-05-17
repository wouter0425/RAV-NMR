#ifndef DEFINES_H
#define DEFINES_H

/* Task related defines */
#define TASK_BUSY_TIME  50                  // The time a task is "busy". Used to simulate the time a task runs
#define MAX_READ_TIME   1000                // Max time (in milliseconds) a task waits for a pipe to become available
//#define TIME_BASED                        // Run the scheduler for x milliseconds
#define MAX_RUN_TIME 40000                  // The amount of time the scheduler runs of TIME_BASED is defined
#define ITERATION_BASED                     // Runs the scheduler for x iterations, based on the first added task
#define MAX_ITERATIONS 10000                // The number of times a scheduler runs if ITERATION_BASED is defined
#define MAX_STUCK_TIME 500                  // Max time (in milliseconds) a task may stay in the same state 

/* Scheduler related defines */
#define NUM_OF_CORES 4                      // Num of cores used by the scheduler
#define SCHEDULER_CORE  4                   // The core ID on which the scheduler runs, no other tasks will run on this core
#define MAX_CORE_WEIGHT 100.0               // Max (and start) reliability weight of a core
#define CORE_BUFFER_SIZE 4                  // Size of the buffer used in the pipes, 4 bytes for integer values

/* Log related defines*/
//#define DEBUG                             // Has each task print its name when it runs
#define LOGGING                             // Log the parameters (core weight & core/task utility)
#define MAX_LOG_INTERVAL    10              // Number of miliseconds between each log 

//#define NMR
//#define RAVNMR

#endif