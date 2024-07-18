#include <pipe.h>

#ifndef NMR
void task_A(void) {
    int value = 42;
    char buffer[BUF_SIZE];

    snprintf(buffer, sizeof(buffer), "%d", value);

    for (int i = 0; i < TASK_BUSY_TIME; i++)
        usleep(1);

    write_to_pipe(AB, buffer);

#ifdef DEBUG
    printf("task A: write: %d \n", value);
#endif

    exit(0);
}

void task_B(void) {
    char buffer[BUF_SIZE];

    // Read from pipe AB
    if (read_from_pipe(AB, buffer, BUF_SIZE)) {
        int value = atoi(buffer);

        if (value != START_VALUE)
            exit(2);

        value++;
        snprintf(buffer, sizeof(buffer), "%d", value);
        
        for (int i = 0; i < TASK_BUSY_TIME; i++)
            usleep(1);

        // Write to pipe BC
        write_to_pipe(BC, buffer);

#ifdef DEBUG
        printf("task B: read: %d \t write: %d \n", value - 1, value);
#endif
        exit(0);
    }

    exit(1);
}

void task_C(void) {
    char buffer[BUF_SIZE];

    if (read_from_pipe(BC, buffer, BUF_SIZE)) {
        int value = atoi(buffer);

        if (value != END_VALUE)
            exit(2);
        
        for (int i = 0; i < TASK_BUSY_TIME; i++)
            usleep(1);

#ifdef DEBUG
        printf("task C: read: %d \n", value);
#endif
        exit(0);
    }

#ifdef DEBUG
    printf("C crashed \n");
#endif
    exit(1);
}

#else
void task_A_1(void) {
    int value = 42;
    char buffer[BUF_SIZE];

    snprintf(buffer, sizeof(buffer), "%d", value);

    for (int i = 0; i < TASK_BUSY_TIME; i++)
        usleep(1);

    write_to_pipe(AB_1, buffer);
    write_to_pipe(AB_2, buffer);
    write_to_pipe(AB_3, buffer);

#ifdef DEBUG
    printf("task A: write: %d \n", value);
#endif

    exit(0);
}

void task_B_1(void) {
    char buffer[BUF_SIZE];

    // Read from pipe AB
    if (read_from_pipe(AB_1, buffer, BUF_SIZE)) {
        int value = atoi(buffer);

        if (value != START_VALUE)
            exit(2);

        value++;
        snprintf(buffer, sizeof(buffer), "%d", value);

        for (int i = 0; i < TASK_BUSY_TIME; i++)
            usleep(1);

        // Write to pipe BC
        write_to_pipe(BC_1, buffer);

 #ifdef DEBUG
        printf("task B_1: read: %d \t write: %d \n", value - 1, value);
#endif
        exit(0);        
    }

    exit(1);
}

void task_B_2(void) {
    char buffer[BUF_SIZE];

    // Read from pipe AB
    if (read_from_pipe(AB_2, buffer, BUF_SIZE)) {
        int value = atoi(buffer);

        if (value != START_VALUE)
            exit(2);

        value++;
        snprintf(buffer, sizeof(buffer), "%d", value);
        
        for (int i = 0; i < TASK_BUSY_TIME; i++)
            usleep(1);

        // Write to pipe BC
        write_to_pipe(BC_2, buffer);

#ifdef DEBUG
        printf("task B_2: read: %d \t write: %d \n", value - 1, value);
#endif
        exit(0);       
    }

    exit(1);
}

void task_B_3(void) {
    char buffer[BUF_SIZE];

    // Read from pipe AB
    if (read_from_pipe(AB_3, buffer, BUF_SIZE)) {
        int value = atoi(buffer);

        if (value != START_VALUE)
            exit(2);

        value++;
        snprintf(buffer, sizeof(buffer), "%d", value);
        
        for (int i = 0; i < TASK_BUSY_TIME; i++)
            usleep(1);

        // Write to pipe BC
        write_to_pipe(BC_3, buffer);

#ifdef DEBUG
        printf("task B_3: read: %d \t write: %d \n", value - 1, value);
#endif

        exit(0);
    }

    exit(1);
}

void task_C_1(void) {
    char buffer[BUF_SIZE];

    if (read_from_pipe(CD_1, buffer, BUF_SIZE)) {
        int value = atoi(buffer);

        if (value != END_VALUE)
            exit(2);
        
        for (int i = 0; i < TASK_BUSY_TIME; i++)
            usleep(1);

#ifdef DEBUG
    printf("task C: read: %d \n", value);
#endif

        exit(0);
    }

    exit(1);
}

void voter_func(void) {
    const int num_buffers = 3;
    char buffers[num_buffers][BUF_SIZE] = {0};
    bool reads[num_buffers] = {false};
    char outputBuffer[BUF_SIZE] = {0};

    // Read from pipes
    reads[0] = read_from_pipe(BC_1, buffers[0], BUF_SIZE);
    reads[1] = read_from_pipe(BC_2, buffers[1], BUF_SIZE);
    reads[2] = read_from_pipe(BC_3, buffers[2], BUF_SIZE);

    // Compare and vote on the data
    if (reads[0] && reads[1] && strcmp(buffers[0], buffers[1]) == 0)
        strncpy(outputBuffer, buffers[0], BUF_SIZE);
    else if (reads[0] && reads[2] && strcmp(buffers[0], buffers[2]) == 0)
        strncpy(outputBuffer, buffers[0], BUF_SIZE);
    else if (reads[1] && reads[2] && strcmp(buffers[1], buffers[2]) == 0)
        strncpy(outputBuffer, buffers[1], BUF_SIZE);
    else {
        // Handle case where no two buffers match
        if (reads[0])
            strncpy(outputBuffer, buffers[0], BUF_SIZE);
        else if (reads[1])
            strncpy(outputBuffer, buffers[1], BUF_SIZE);
        else if (reads[2])
            strncpy(outputBuffer, buffers[2], BUF_SIZE);
        else {
            // If no valid data is read, the voter should not have been spawned
            exit(1);
        }
    }

#ifdef DEBUG
    printf("Voter output: %s\n", outputBuffer);
#endif

    // Write the result to the output pipe
    write_to_pipe(CD_1, outputBuffer);

    exit(0);
}

#endif