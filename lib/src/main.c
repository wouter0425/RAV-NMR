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
    scheduler s;
    init_scheduler(&s);

    while(1)
    {
        run_tasks(&s);
        monitor_tasks(&s);
    }

    cleanup_tasks(&s);

    return 0;
}