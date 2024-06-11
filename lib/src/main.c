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
#include "defines.h"

int main()
{
    pthread_t scheduler_tid;
    sigset_t set;

    // Setup signal handler for clean exit
    signal(SIGINT, handle_signal);

    // Block all signals in the main thread
    sigfillset(&set);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    // Create the scheduler thread
    if (pthread_create(&scheduler_tid, NULL, scheduler_task, NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    // Unblock signals in the scheduler thread
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);

    // Wait for the scheduler thread to finish (it will when SIGINT is received)
    if (pthread_join(scheduler_tid, NULL) != 0) {
        perror("pthread_join");
        exit(EXIT_FAILURE);
    }

    return 0;
}