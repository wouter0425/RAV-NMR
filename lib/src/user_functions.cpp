#include <pipe.h>

#include "math.h"
#include <time.h>
#include "timer.h"
#include <array>

#include "../../benchmark/include/FlightController.h"

#ifndef NMR
void task_A(void) {
    int value = 42;
    char buffer[BUF_SIZE];

    snprintf(buffer, sizeof(buffer), "%d", value);

    Timer timer;
    while (!timer.hasElapsedMilliseconds(TASK_BUSY_TIME)) {}

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

        Timer timer;
        while (!timer.hasElapsedMilliseconds(TASK_BUSY_TIME)) {}
        
        snprintf(buffer, sizeof(buffer), "%d", value);

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

        Timer timer;
        while (!timer.hasElapsedMilliseconds(TASK_BUSY_TIME)) {}

        if (value != END_VALUE)
            exit(2);        

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

// void task_A(void) 
// {
//     //double roll, pitch;
//     int val = 1;
//     char buffer[4] = {0};

//     //srand(time(0));

//     //readSensors(&roll, &pitch);

//     snprintf(buffer, sizeof(buffer), "%d", val);

//     write_to_pipe(AB, buffer);

//     exit(0);
// }

// void task_B(void) 
// {
//     double roll, pitch, yaw;
//     double stabilization_outputs[3];
//     char read_buffer[17] = {0};
//     char write_buffer[26] = {0};

//     if (!(read_from_pipe(AB, read_buffer, 17) && sscanf(read_buffer, "%lf %lf", &roll, &pitch) == 2))    
//     {
//         printf("Buffer B: %s \t size: %d \n", read_buffer, size(read_buffer));
//         exit(1);
//     }

//     calculateStabilization(roll, pitch, yaw, stabilization_outputs);

//     snprintf(write_buffer, sizeof(write_buffer), "%.2f %.2f %.2f", roll, pitch, yaw);
//     write_to_pipe(BC, write_buffer);

//     exit(0);
// }

// void task_C(void) 
// {
//     char buffer[26] = {0};
//     double roll, pitch, yaw;

//     if (!(read_from_pipe(BC, buffer, 24) && sscanf(buffer, "%lf %lf %lf", &roll, &pitch, &yaw) == 3))    
//     {        
//         printf("Buffer C: %s \n", buffer);
//         exit(1);
//     }

//     controlMotors(roll, pitch, yaw);

//     exit(0);
// }


#else

void task_A_1(void)
{
    #ifdef DEBUG
    printf("task A-1\n");
    #endif

    // Simulated sensor values
    double accelRoll     = 12.5;
    double accelPitch    = -8.0;
    double gyroRollRate  = 0.2;
    double gyroPitchRate = -0.1;

    // Complementary filter settings
    double alpha           = 0.98;
    double estimated_roll  = 0.0;
    double estimated_pitch = 0.0;

    // Busy loop to simulate some processing time
    Timer timer;
    while (!timer.hasElapsedMilliseconds(TASK_BUSY_TIME))
    {
        // Apply complementary filter
        estimated_roll  = alpha * (estimated_roll  + gyroRollRate  * 0.01)
                        + (1 - alpha) * accelRoll;
        estimated_pitch = alpha * (estimated_pitch + gyroPitchRate * 0.01)
                        + (1 - alpha) * accelPitch;
    }

    // Decide command based on thresholds
    Command sensorCommand = NO_ACTION;
    if (std::fabs(estimated_roll) > 10.0) {
        sensorCommand = STABILIZE_ROLL;
    } else if (std::fabs(estimated_pitch) > 10.0) {
        sensorCommand = STABILIZE_PITCH;
    }

#ifdef DEBUG
    printf("DEBUG A: A->B cmd=%s roll=%.2f pitch=%.2f\n", 
           commandToStr(sensorCommand), estimated_roll, estimated_pitch);
#endif

    // Send only what Task B needs: command, roll, pitch
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%d %.2f %.2f",
             static_cast<int>(sensorCommand), estimated_roll, estimated_pitch);

    write_to_pipe(AB_1, buffer);
    write_to_pipe(AB_2, buffer);
    write_to_pipe(AB_3, buffer);

    exit(0);
}

void task_B_1(void)
{
    #ifdef DEBUG
    printf("task B-1\n");
    #endif

    // Read from pipe AB_1
    char buffer[64] = {0};
    if (!read_from_pipe(AB_1, buffer, sizeof(buffer))) {
        //printf("DEBUG B: read_from_pipe(AB_1) failed\n");
        exit(1);
    }

    int cmdInt = 0;
    double roll_in=0.0, pitch_in=0.0;
    int parsed = sscanf(buffer, "%d %lf %lf", &cmdInt, &roll_in, &pitch_in);
    if (parsed != 3) {
        //printf("DEBUG B: parse error, buffer=%s\n", buffer);
        exit(1);
    }
    Command sensorCommand = static_cast<Command>(cmdInt);

    // We'll store a "running" stabilized angles. Start from the input.
    double stabilizedRoll  = roll_in;
    double stabilizedPitch = pitch_in;
    double stabilizedYaw   = 0.0;  // assume 0 for demonstration

    Timer timer;
    while (!timer.hasElapsedMilliseconds(TASK_BUSY_TIME))
    {
        // Convert degrees to radians
        double radRoll  = stabilizedRoll  * M_PI / 180.0;
        double radPitch = stabilizedPitch * M_PI / 180.0;

        // Build rotationMatrix (rotation around Y by 'roll')
        std::array<std::array<double,3>,3> rotationMatrix = {{
            { std::cos(radRoll),  0.0, std::sin(radRoll) },
            { 0.0,                1.0, 0.0               },
            { -std::sin(radRoll), 0.0, std::cos(radRoll) }
        }};

        // Build correctionMatrix (rotation around X by 'pitch')
        std::array<std::array<double,3>,3> correctionMatrix = {{
            { 1.0,              0.0,               0.0 },
            { 0.0,  std::cos(radPitch), -std::sin(radPitch) },
            { 0.0,  std::sin(radPitch),  std::cos(radPitch) }
        }};

        // Multiply them to get resultMatrix
        std::array<std::array<double,3>,3> resultMatrix = {{{0}}};
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                resultMatrix[i][j] = 0.0;
                for (int k = 0; k < 3; k++) {
                    resultMatrix[i][j] += rotationMatrix[i][k] * correctionMatrix[k][j];
                }
            }
        }

        // For demonstration, interpret each row's sum as new angles
        // This ensures the matrix multiplication actually changes the output.
        double row0 = resultMatrix[0][0] + resultMatrix[0][1] + resultMatrix[0][2];
        double row1 = resultMatrix[1][0] + resultMatrix[1][1] + resultMatrix[1][2];
        double row2 = resultMatrix[2][0] + resultMatrix[2][1] + resultMatrix[2][2];

        // We'll treat row0 as the new "roll," row1 as "pitch," row2 as "yaw"
        stabilizedRoll  = row0;
        stabilizedPitch = row1;
        stabilizedYaw   = row2;
    }

#ifdef DEBUG    
    printf("DEBUG B: B->C cmd=%s inRoll=%.2f inPitch=%.2f outRoll=%.2f outPitch=%.2f outYaw=%.2f\n",
           commandToStr(sensorCommand), roll_in, pitch_in, stabilizedRoll, stabilizedPitch, stabilizedYaw);
#endif

    // Send only what Task C needs: command, stabilizedRoll, stabilizedPitch, stabilizedYaw
    snprintf(buffer, sizeof(buffer), "%d %.2f %.2f %.2f",
             sensorCommand, stabilizedRoll, stabilizedPitch, stabilizedYaw);
    write_to_pipe(BC_1, buffer);

    exit(0);
}


void task_B_2(void) 
{
    #ifdef DEBUG
    printf("task B-2\n");
    #endif

    // Read from pipe AB_1
    char buffer[64] = {0};
    if (!read_from_pipe(AB_2, buffer, sizeof(buffer))) {
        //printf("DEBUG B: read_from_pipe(AB_1) failed\n");
        exit(1);
    }

    int cmdInt = 0;
    double roll_in=0.0, pitch_in=0.0;
    int parsed = sscanf(buffer, "%d %lf %lf", &cmdInt, &roll_in, &pitch_in);
    if (parsed != 3) {
        //printf("DEBUG B: parse error, buffer=%s\n", buffer);
        exit(1);
    }
    Command sensorCommand = static_cast<Command>(cmdInt);

    // We'll store a "running" stabilized angles. Start from the input.
    double stabilizedRoll  = roll_in;
    double stabilizedPitch = pitch_in;
    double stabilizedYaw   = 0.0;  // assume 0 for demonstration

    Timer timer;
    while (!timer.hasElapsedMilliseconds(TASK_BUSY_TIME))
    {
        // Convert degrees to radians
        double radRoll  = stabilizedRoll  * M_PI / 180.0;
        double radPitch = stabilizedPitch * M_PI / 180.0;

        // Build rotationMatrix (rotation around Y by 'roll')
        std::array<std::array<double,3>,3> rotationMatrix = {{
            { std::cos(radRoll),  0.0, std::sin(radRoll) },
            { 0.0,                1.0, 0.0               },
            { -std::sin(radRoll), 0.0, std::cos(radRoll) }
        }};

        // Build correctionMatrix (rotation around X by 'pitch')
        std::array<std::array<double,3>,3> correctionMatrix = {{
            { 1.0,              0.0,               0.0 },
            { 0.0,  std::cos(radPitch), -std::sin(radPitch) },
            { 0.0,  std::sin(radPitch),  std::cos(radPitch) }
        }};

        // Multiply them to get resultMatrix
        std::array<std::array<double,3>,3> resultMatrix = {{{0}}};
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                resultMatrix[i][j] = 0.0;
                for (int k = 0; k < 3; k++) {
                    resultMatrix[i][j] += rotationMatrix[i][k] * correctionMatrix[k][j];
                }
            }
        }

        // For demonstration, interpret each row's sum as new angles
        // This ensures the matrix multiplication actually changes the output.
        double row0 = resultMatrix[0][0] + resultMatrix[0][1] + resultMatrix[0][2];
        double row1 = resultMatrix[1][0] + resultMatrix[1][1] + resultMatrix[1][2];
        double row2 = resultMatrix[2][0] + resultMatrix[2][1] + resultMatrix[2][2];

        // We'll treat row0 as the new "roll," row1 as "pitch," row2 as "yaw"
        stabilizedRoll  = row0;
        stabilizedPitch = row1;
        stabilizedYaw   = row2;
    }

#ifdef DEBUG
    printf("DEBUG B: B->C cmd=%s inRoll=%.2f inPitch=%.2f outRoll=%.2f outPitch=%.2f outYaw=%.2f\n",
           commandToStr(sensorCommand), roll_in, pitch_in, stabilizedRoll, stabilizedPitch, stabilizedYaw);
#endif
    // Send only what Task C needs: command, stabilizedRoll, stabilizedPitch, stabilizedYaw
    snprintf(buffer, sizeof(buffer), "%d %.2f %.2f %.2f",
             sensorCommand, stabilizedRoll, stabilizedPitch, stabilizedYaw);
    write_to_pipe(BC_2, buffer);

    exit(0);
}

void task_B_3(void) 
{
    #ifdef DEBUG
    printf("task B-3\n");
    #endif

    // Read from pipe AB_1
    char buffer[64] = {0};
    if (!read_from_pipe(AB_3, buffer, sizeof(buffer))) {
        //printf("DEBUG B: read_from_pipe(AB_1) failed\n");
        exit(1);
    }

    int cmdInt = 0;
    double roll_in=0.0, pitch_in=0.0;
    int parsed = sscanf(buffer, "%d %lf %lf", &cmdInt, &roll_in, &pitch_in);
    if (parsed != 3) {
        //printf("DEBUG B: parse error, buffer=%s\n", buffer);
        exit(1);
    }
    Command sensorCommand = static_cast<Command>(cmdInt);

    // We'll store a "running" stabilized angles. Start from the input.
    double stabilizedRoll  = roll_in;
    double stabilizedPitch = pitch_in;
    double stabilizedYaw   = 0.0;  // assume 0 for demonstration

    Timer timer;
    while (!timer.hasElapsedMilliseconds(TASK_BUSY_TIME))
    {
        // Convert degrees to radians
        double radRoll  = stabilizedRoll  * M_PI / 180.0;
        double radPitch = stabilizedPitch * M_PI / 180.0;

        // Build rotationMatrix (rotation around Y by 'roll')
        std::array<std::array<double,3>,3> rotationMatrix = {{
            { std::cos(radRoll),  0.0, std::sin(radRoll) },
            { 0.0,                1.0, 0.0               },
            { -std::sin(radRoll), 0.0, std::cos(radRoll) }
        }};

        // Build correctionMatrix (rotation around X by 'pitch')
        std::array<std::array<double,3>,3> correctionMatrix = {{
            { 1.0,              0.0,               0.0 },
            { 0.0,  std::cos(radPitch), -std::sin(radPitch) },
            { 0.0,  std::sin(radPitch),  std::cos(radPitch) }
        }};

        // Multiply them to get resultMatrix
        std::array<std::array<double,3>,3> resultMatrix = {{{0}}};
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                resultMatrix[i][j] = 0.0;
                for (int k = 0; k < 3; k++) {
                    resultMatrix[i][j] += rotationMatrix[i][k] * correctionMatrix[k][j];
                }
            }
        }

        // For demonstration, interpret each row's sum as new angles
        // This ensures the matrix multiplication actually changes the output.
        double row0 = resultMatrix[0][0] + resultMatrix[0][1] + resultMatrix[0][2];
        double row1 = resultMatrix[1][0] + resultMatrix[1][1] + resultMatrix[1][2];
        double row2 = resultMatrix[2][0] + resultMatrix[2][1] + resultMatrix[2][2];

        // We'll treat row0 as the new "roll," row1 as "pitch," row2 as "yaw"
        stabilizedRoll  = row0;
        stabilizedPitch = row1;
        stabilizedYaw   = row2;
    }

#ifdef DEBUG
    printf("DEBUG B: B->C cmd=%s inRoll=%.2f inPitch=%.2f outRoll=%.2f outPitch=%.2f outYaw=%.2f\n",
           commandToStr(sensorCommand), roll_in, pitch_in, stabilizedRoll, stabilizedPitch, stabilizedYaw);
#endif

    // Send only what Task C needs: command, stabilizedRoll, stabilizedPitch, stabilizedYaw
    snprintf(buffer, sizeof(buffer), "%d %.2f %.2f %.2f",
             sensorCommand, stabilizedRoll, stabilizedPitch, stabilizedYaw);
    write_to_pipe(BC_3, buffer);

    exit(0);
}

void voter_func(void) {
    #ifdef DEBUG
    printf("Voter \n");
    #endif
    char buffers[3][64] = {{0}};
    bool reads[3];

    // 1) Read from each B->C pipe
    reads[0] = read_from_pipe(BC_1, buffers[0], sizeof(buffers[0]));
    reads[1] = read_from_pipe(BC_2, buffers[1], sizeof(buffers[1]));
    reads[2] = read_from_pipe(BC_3, buffers[2], sizeof(buffers[2]));

    // 2) You could parse each buffer, compare the floats, do a majority vote
    //    For now, let's do a simplistic "string compare" approach (like you do).
    char outputBuffer[64] = {0};

    if (reads[0] && reads[1] && strcmp(buffers[0], buffers[1]) == 0) {
        strncpy(outputBuffer, buffers[0], sizeof(outputBuffer));
    } else if (reads[0] && reads[2] && strcmp(buffers[0], buffers[2]) == 0) {
        strncpy(outputBuffer, buffers[0], sizeof(outputBuffer));
    } else if (reads[1] && reads[2] && strcmp(buffers[1], buffers[2]) == 0) {
        strncpy(outputBuffer, buffers[1], sizeof(outputBuffer));
    } else {
        // If no two match, pick the first valid
        if (reads[0]) strncpy(outputBuffer, buffers[0], sizeof(outputBuffer));
        else if (reads[1]) strncpy(outputBuffer, buffers[1], sizeof(outputBuffer));
        else if (reads[2]) strncpy(outputBuffer, buffers[2], sizeof(outputBuffer));
        else exit(1); // no data
    }

    // 3) Write final result to next pipe (CD_1) for Task C
    write_to_pipe(CD_1, outputBuffer);
    exit(0);
}

void task_C_1(void) 
{
    #ifdef DEBUG
    printf("Task C \n");
    #endif

    char buffer[64] = {0};
    if (!read_from_pipe(CD_1, buffer, sizeof(buffer))) {        
        exit(1);
    }

    int cmdInt = 0;
    double roll_in=0.0, pitch_in=0.0, yaw_in=0.0;
    int parsed = sscanf(buffer, "%d %lf %lf %lf", &cmdInt, &roll_in, &pitch_in, &yaw_in);
    if (parsed != 4) {        
        exit(1);
    }
    Command finalCmd = static_cast<Command>(cmdInt);

    // Busy loop to simulate PID
    Timer timer;
    double motorRollOutput=0.0, motorPitchOutput=0.0, motorYawOutput=0.0;
    double Kp=0.8, Ki=0.1, Kd=0.2;
    static double prevErrorRoll=0.0, integralRoll=0.0;
    static double prevErrorPitch=0.0, integralPitch=0.0;
    static double prevErrorYaw=0.0, integralYaw=0.0;

    while (!timer.hasElapsedMilliseconds(TASK_BUSY_TIME))
    {
        // Let roll_in, pitch_in, yaw_in represent the "errors"
        double errorRoll  = roll_in;
        double errorPitch = pitch_in;
        double errorYaw   = yaw_in;

        // Basic PID for roll
        integralRoll += errorRoll;
        double derivativeRoll = (errorRoll - prevErrorRoll);
        motorRollOutput = Kp * errorRoll + Ki * integralRoll + Kd * derivativeRoll;
        prevErrorRoll   = errorRoll;

        // Pitch
        integralPitch += errorPitch;
        double derivativePitch = (errorPitch - prevErrorPitch);
        motorPitchOutput = Kp * errorPitch + Ki * integralPitch + Kd * derivativePitch;
        prevErrorPitch   = errorPitch;

        // Yaw
        integralYaw += errorYaw;
        double derivativeYaw = (errorYaw - prevErrorYaw);
        motorYawOutput = Kp * errorYaw + Ki * integralYaw + Kd * derivativeYaw;
        prevErrorYaw   = errorYaw;
    }

#ifdef DEBUG
    printf("DEBUG C: finalCmd=%s roll=%.2f pitch=%.2f yaw=%.2f rollOut=%.2f pitchOut=%.2f yawOut=%.2f\n",
           commandToStr(finalCmd),
           roll_in, pitch_in, yaw_in,
           motorRollOutput, motorPitchOutput, motorYawOutput);
#endif

    exit(0);
}

#endif