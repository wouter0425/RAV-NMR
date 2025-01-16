#ifndef TIMER_H
#define TIMER_H

#include <chrono>

class Timer {
public:
    Timer(); // Constructor
    ~Timer(); // Destructor 

    // Reset the timer
    void reset();

    // Get the elapsed time in seconds
    double elapsedSeconds() const;

    // Get the elapsed time in milliseconds
    double elapsedMilliseconds() const;

    // Check if the given number of milliseconds has elapsed
    bool hasElapsedMilliseconds(double milliseconds) const;

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
};

#endif // TIMER_H