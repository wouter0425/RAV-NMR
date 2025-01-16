#include "../include/FlightController.h"

#include "timer.h"
#include "defines.h"

#include <stdio.h>
#include <math.h>
#include <array>
#include <stdlib.h>
#include <time.h>

// Simulate raw accelerometer and gyroscope data
void readRawSensorValues(double *accel_roll, double *accel_pitch, double *gyro_roll, double *gyro_pitch) 
{
    *accel_roll = (rand() % 100 - 50) / 10.0;   // Simulate accelerometer roll (-5 to 5 degrees)
    *accel_pitch = (rand() % 100 - 50) / 10.0;  // Simulate accelerometer pitch (-5 to 5 degrees)
    *gyro_roll = (rand() % 100 - 50) / 50.0;    // Simulate gyroscope roll rate (-1 to 1 degrees/s)
    *gyro_pitch = (rand() % 100 - 50) / 50.0;   // Simulate gyroscope pitch rate (-1 to 1 degrees/s)
}

void readSensors(double *roll, double *pitch) 
{
    // busy timer
    Timer timer;

    const double alpha = 0.98; // Filter weight for gyroscope influence

    double accel_roll, accel_pitch, gyro_roll_rate, gyro_pitch_rate;
        
    // Simulate raw sensor data
    readRawSensorValues(&accel_roll, &accel_pitch, &gyro_roll_rate, &gyro_pitch_rate);    
    
    static double estimated_roll = 0.0, estimated_pitch = 0.0; // Initial estimated orientation

    // Simulate some processing time
    while (!timer.hasElapsedMilliseconds(TASK_BUSY_TIME))
    {
        // Apply complementary filter
        estimated_roll = alpha * (estimated_roll + gyro_roll_rate * 0.01) + (1 - alpha) * accel_roll;
        estimated_pitch = alpha * (estimated_pitch + gyro_pitch_rate * 0.01) + (1 - alpha) * accel_pitch;

        *roll = estimated_roll;
        *pitch = estimated_pitch;
    }

#ifdef DEBUG
    printf("Task 1: Read Sensors - Roll: %lf, Pitch: %lf\n", estimated_roll, estimated_pitch);
#endif

    return;
}

void calculateStabilization(double roll, double pitch, double yaw, double *stabilized_output) 
{
    // busy timer
    Timer timer;

    double rollRad = roll * M_PI / 180.0;
    double pitchRad = pitch * M_PI / 180.0;
    double yawRad = yaw * M_PI / 180.0;

    double rotationMatrix[3][3];

    while (!timer.hasElapsedMilliseconds(TASK_BUSY_TIME))
    {
        rotationMatrix[0][0] = cos(rollRad);
        rotationMatrix[1][1] = cos(pitchRad);
        rotationMatrix[2][2] = cos(yawRad);
    }

    stabilized_output[0] = -roll * rotationMatrix[0][0];
    stabilized_output[1] = -pitch * rotationMatrix[1][1];
    stabilized_output[2] = -yaw * rotationMatrix[2][2];

#ifdef DEBUG
    printf("Task 2: Stabilization Calculation\n");
    printf("Correction Outputs: Roll Correction = %lf, Pitch Correction = %lf, Yaw Correction = %lf\n",
           stabilized_output[0], stabilized_output[1], stabilized_output[2]);
#endif

    return;
}

void controlMotors(double roll_correction, double pitch_correction, double yaw_correction) 
{
    // busy timer
    Timer timer;

    while (!timer.hasElapsedMilliseconds(TASK_BUSY_TIME))
    {
        double Kp = 0.8, Ki = 0.1, Kd = 0.2;

        static double prevRollError = 0.0, prevPitchError = 0.0, prevYawError = 0.0;
        static double integralRoll = 0.0, integralPitch = 0.0, integralYaw = 0.0;

        double rollError = roll_correction;
        double pitchError = pitch_correction;
        double yawError = yaw_correction;

        integralRoll += rollError;
        integralPitch += pitchError;
        integralYaw += yawError;

        double derivativeRoll = rollError - prevRollError;
        double derivativePitch = pitchError - prevPitchError;
        double derivativeYaw = yawError - prevYawError;

        double rollOutput = Kp * rollError + Ki * integralRoll + Kd * derivativeRoll;
        double pitchOutput = Kp * pitchError + Ki * integralPitch + Kd * derivativePitch;
        double yawOutput = Kp * yawError + Ki * integralYaw + Kd * derivativeYaw;

        prevRollError = rollError;
        prevPitchError = pitchError;
        prevYawError = yawError;
    }

#ifdef DEBUG
    printf("Task 3: Adjusted motor speeds based on PID corrections.\n");
#endif

    return;
}


/* Version 2 */

// Function to simulate receiving static sensor values
void getStaticSensorValues(double &accelRoll, double &accelPitch, double &gyroRollRate, double &gyroPitchRate) {
    accelRoll = 12.5;   // Static roll value (degrees)
    accelPitch = -8.0;  // Static pitch value (degrees)
    gyroRollRate = 0.2; // Static roll rate (degrees/sec)
    gyroPitchRate = -0.1; // Static pitch rate (degrees/sec)
}

Command readSensors() 
{
    // busy timer
    Timer timer;

    double estimatedRoll;
    double estimatedPitch;

    // Simulate some processing time
    while (!timer.hasElapsedMilliseconds(TASK_BUSY_TIME))
    {
        // Simulated (static) sensor values
        double accelRoll, accelPitch, gyroRollRate, gyroPitchRate;
        getStaticSensorValues(accelRoll, accelPitch, gyroRollRate, gyroPitchRate);

        // Apply complementary filter
        const double alpha = 0.98; // Gyroscope weight
        estimatedRoll = alpha * (gyroRollRate * 0.01) + (1 - alpha) * accelRoll;
        estimatedPitch = alpha * (gyroPitchRate * 0.01) + (1 - alpha) * accelPitch;

    }

    // Generate a command based on thresholds
    if (fabs(estimatedRoll) > 10.0) {        
        return STABILIZE_ROLL;
    } else if (fabs(estimatedPitch) > 10.0) {        
        return STABILIZE_PITCH;
    }
    
    return NO_ACTION;
}

Command calculateStabilization(const Command &command) 
{
    // busy timer
    Timer timer;

    if (command == NO_ACTION) {        
        return NO_ACTION;
    }

    // Simulated (static) stabilization values
    double fictiveRoll = 12.5;  // Hardcoded roll value
    double fictivePitch = -8.0; // Hardcoded pitch value
    double fictiveYaw = 0.0;    // Hardcoded yaw value

    // Simulate some processing time
    while (!timer.hasElapsedMilliseconds(TASK_BUSY_TIME))
    {

        // Heavy computation: Matrix operations
        const int matrixSize = 3;
        std::array<std::array<double, matrixSize>, matrixSize> rotationMatrix = {{
            {1.0, 0.0, 0.0},
            {0.0, 1.0, 0.0},
            {0.0, 0.0, 1.0}
        }};
        std::array<std::array<double, matrixSize>, matrixSize> correctionMatrix = {{
            {0.1, 0.2, 0.3},
            {0.4, 0.5, 0.6},
            {0.7, 0.8, 0.9}
        }};
        std::array<std::array<double, matrixSize>, matrixSize> resultMatrix = {0};

        // Perform matrix multiplication
        for (int i = 0; i < matrixSize; ++i) {
            for (int j = 0; j < matrixSize; ++j) {
                resultMatrix[i][j] = 0.0;
                for (int k = 0; k < matrixSize; ++k) {
                    resultMatrix[i][j] += rotationMatrix[i][k] * correctionMatrix[k][j];
                }
            }
        }
    }

    // Log corrections based on the command
    if (command == STABILIZE_ROLL) {
        //std::cout << "Task 2: Calculating roll correction: " << -0.8 * fictiveRoll << "\n";
    } else if (command == STABILIZE_PITCH) {
        //std::cout << "Task 2: Calculating pitch correction: " << -0.8 * fictivePitch << "\n";
    } else if (command == STABILIZE_YAW) {
        //std::cout << "Task 2: Calculating yaw correction: " << -0.8 * fictiveYaw << "\n";
    }

    return command;
}

void controlMotors(const Command &command) {
    // busy timer
    Timer timer;

    if (command == NO_ACTION) {
        //std::cout << "Task 3: No motor adjustments needed.\n";
        return;
    }

    // PID controller constants
    double Kp = 0.8, Ki = 0.1, Kd = 0.2;
    static double prevErrorRoll = 0.0, prevErrorPitch = 0.0, prevErrorYaw = 0.0;
    static double integralRoll = 0.0, integralPitch = 0.0, integralYaw = 0.0;

    // Predefined (fictive) error values
    double errorRoll = 12.5;  // Simulated roll error
    double errorPitch = -8.0; // Simulated pitch error
    double errorYaw = 0.0;    // Simulated yaw error

    // Variables for PID output
    double rollOutput = 0.0, pitchOutput = 0.0, yawOutput = 0.0;

    // Simulate some processing time
    while (!timer.hasElapsedMilliseconds(TASK_BUSY_TIME))
    {
        // Compute PID correction based on the command
        if (command == STABILIZE_ROLL) {
            integralRoll += errorRoll;
            double derivativeRoll = errorRoll - prevErrorRoll;
            rollOutput = Kp * errorRoll + Ki * integralRoll + Kd * derivativeRoll;
            prevErrorRoll = errorRoll;
            //std::cout << "Task 3: Adjusting motors for roll stabilization: " << rollOutput << " degrees.\n";
        } else if (command == STABILIZE_PITCH) {
            integralPitch += errorPitch;
            double derivativePitch = errorPitch - prevErrorPitch;
            pitchOutput = Kp * errorPitch + Ki * integralPitch + Kd * derivativePitch;
            prevErrorPitch = errorPitch;
            //std::cout << "Task 3: Adjusting motors for pitch stabilization: " << pitchOutput << " degrees.\n";
        } else if (command == STABILIZE_YAW) {
            integralYaw += errorYaw;
            double derivativeYaw = errorYaw - prevErrorYaw;
            yawOutput = Kp * errorYaw + Ki * integralYaw + Kd * derivativeYaw;
            prevErrorYaw = errorYaw;
            //std::cout << "Task 3: Adjusting motors for yaw stabilization: " << yawOutput << " degrees.\n";
        }
    }
}
