#include <pipe.h>

#include "math.h"
#include <time.h>
#include "timer.h"
#include <array>

#include "../../benchmark/include/FlightController.h"

// Declare the pipes here
extern Pipe *AB_1;
extern Pipe *AB_2;
extern Pipe *AB_3;
extern Pipe *BC_1;
extern Pipe *BC_2;
extern Pipe *BC_3;
extern Pipe *CD_1;

/**************************************
 * No NMR
 ************************************ */ 
void read_sensors_0(void)
{
#ifdef DEBUG
    printf("task A\n");
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

    // Send only what Task B needs: command, roll, pitch
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%d %.2f %.2f",
             static_cast<int>(sensorCommand), estimated_roll, estimated_pitch);

    AB_1->write_data(buffer);

    exit(0);
}

void process_data_0(void)
{
#ifdef DEBUG
    printf("task B-1\n");
#endif

    // Read from pipe AB_1
    char buffer[64] = {0};
    if (!AB_1->read_data(buffer, sizeof(buffer))) {        
        exit(1);
    }

    if (strcmp(buffer, "1 12.60 -8.05" ))
        exit(2);

#ifdef DEBUG
    printf("Buffer: [%s] \n", buffer);
#endif

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

    // Send only what Task C needs: command, stabilizedRoll, stabilizedPitch, stabilizedYaw
    snprintf(buffer, sizeof(buffer), "%d %.2f %.2f %.2f",
             sensorCommand, stabilizedRoll, stabilizedPitch, stabilizedYaw);
    CD_1->write_data(buffer);    

    exit(0);
}


/**************************************
 * NMR
 ************************************ */ 
void read_sensors(void)
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

    // Send only what Task B needs: command, roll, pitch
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%d %.2f %.2f",
             static_cast<int>(sensorCommand), estimated_roll, estimated_pitch);

    AB_1->write_data(buffer);
    AB_2->write_data(buffer);
    AB_3->write_data(buffer);

    exit(0);
}

void process_data_1(void)
{
#ifdef DEBUG
    printf("task B-1\n");
#endif

    // Read from pipe AB_1
    char buffer[64] = {0};
    if (!AB_1->read_data(buffer, sizeof(buffer))) {        
        exit(1);
    }

    if (strcmp(buffer, "1 12.60 -8.05" ))
        exit(2);

#ifdef DEBUG
    printf("Buffer: [%s] \n", buffer);
#endif

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

    // Send only what Task C needs: command, stabilizedRoll, stabilizedPitch, stabilizedYaw
    snprintf(buffer, sizeof(buffer), "%d %.2f %.2f %.2f",
             sensorCommand, stabilizedRoll, stabilizedPitch, stabilizedYaw);
    BC_1->write_data(buffer);    

    exit(0);
}

void process_data_2(void) 
{
#ifdef DEBUG
    printf("task B-2\n");
#endif

    // Read from pipe AB_1
    char buffer[64] = {0};
    if (!AB_2->read_data(buffer, sizeof(buffer))) {        
        exit(1);
    }

    if (strcmp(buffer, "1 12.60 -8.05" ))
        exit(2);

    int cmdInt = 0;
    double roll_in=0.0, pitch_in=0.0;
    int parsed = sscanf(buffer, "%d %lf %lf", &cmdInt, &roll_in, &pitch_in);
    if (parsed != 3) {        
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

    // Send only what Task C needs: command, stabilizedRoll, stabilizedPitch, stabilizedYaw
    snprintf(buffer, sizeof(buffer), "%d %.2f %.2f %.2f",
             sensorCommand, stabilizedRoll, stabilizedPitch, stabilizedYaw);
    BC_2->write_data(buffer);    

    exit(0);
}

void process_data_3(void) 
{
#ifdef DEBUG
    printf("task B-3\n");
#endif

    // Read from pipe AB_1
    char buffer[64] = {0};
    if (!AB_3->read_data(buffer, sizeof(buffer))) {                
        exit(1);
    }

    if (strcmp(buffer, "1 12.60 -8.05" ))
        exit(2);

    int cmdInt = 0;
    double roll_in=0.0, pitch_in=0.0;
    int parsed = sscanf(buffer, "%d %lf %lf", &cmdInt, &roll_in, &pitch_in);
    if (parsed != 3) {        
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

    // Send only what Task C needs: command, stabilizedRoll, stabilizedPitch, stabilizedYaw
    snprintf(buffer, sizeof(buffer), "%d %.2f %.2f %.2f",
             sensorCommand, stabilizedRoll, stabilizedPitch, stabilizedYaw);
    BC_3->write_data(buffer);    

    exit(0);
}

void majority_voter(void) {
#ifdef DEBUG
    printf("Voter \n");
#endif

    char buffers[3][64] = {{0}};
    bool reads[3];

    // 1) Read from each B->C pipe
    reads[0] = BC_1->read_data(buffers[0], sizeof(buffers[0]));
    reads[1] = BC_2->read_data(buffers[1], sizeof(buffers[1]));
    reads[2] = BC_3->read_data(buffers[2], sizeof(buffers[2]));    

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

    Timer timer;
    while (!timer.hasElapsedMilliseconds(10)) { }

    // 3) Write final result to next pipe (CD_1) for Task C    
    CD_1->write_data(outputBuffer);
    exit(0);
}

void control_actuators(void) 
{
#ifdef DEBUG
    printf("Task C \n");
#endif

    char buffer[64] = {0};
    if (!CD_1->read_data(buffer, sizeof(buffer))) {                
        exit(1);
    }

    if (strcmp(buffer, "1 1.02 0.98 1.00" ))
        exit(2);

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

    exit(0);
}