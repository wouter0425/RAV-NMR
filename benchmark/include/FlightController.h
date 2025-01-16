#ifndef FLIGHTCONTROLLER_H
#define FLIGHTCONTROLLER_H

// Commands for stabilization
enum Command {
    NO_ACTION,
    STABILIZE_ROLL,
    STABILIZE_PITCH,
    STABILIZE_YAW
};

// Random value generator
void readRawSensorValues(double *accel_roll, double *accel_pitch, double *gyro_roll, double *gyro_pitch);

// Task 1
void readSensors(double *roll, double *pitch);

// Task 2
void calculateStabilization(double roll, double pitch, double yaw, double *stabilized_output);

// Task 3
void controlMotors(double roll_correction, double pitch_correction, double yaw_correction);

// Other version
void getStaticSensorValues(double &accelRoll, double &accelPitch, double &gyroRollRate, double &gyroPitchRate);

Command readSensors();

Command calculateStabilization(const Command &command);

void controlMotors(const Command &command);

#endif