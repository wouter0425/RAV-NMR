#ifndef FLIGHT_CONTROLLER_H
#define FLIGHT_CONTROLLER_H

// Commands for stabilization
enum Command {
    NO_ACTION,
    CALCULATE_VECTOR,
    CORRECT_VECTOR
};

void read_sensors(void);
void process_data(void);
void process_data_1(void);
void process_data_2(void);
void process_data_3(void);
void majority_voter(void);
void control_actuators(void);

#endif