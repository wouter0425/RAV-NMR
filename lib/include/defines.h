#ifndef DEFINES_H
#define DEFINES_H

#define NUM_OF_CORES 8
#define MAX_CORE_WEIGHT 100.0
#define BUF_SIZE 4
//#define TRUE 1
//#define FALSE 0

//#define DEBUG
//#define DEBUG_SCHEDULER
#define LOGGING

#define RELIABILITY_SCHEDULING
//#define NMR

#ifdef NMR
#define NUM_OF_TASKS 6
#else
#define NUM_OF_TASKS 3
#endif

/* Runtime in seconds */
#define MAX_RUN_TIME 10000
#define MAX_LOG_INTERVAL    100
#define NUM_OF_SAMPLES  10000

#define INCREASE    1.1
#define DECREASE    0.9

#endif