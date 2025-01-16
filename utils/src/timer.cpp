#include "timer.h"

// Constructor
Timer::Timer() : start_time(std::chrono::high_resolution_clock::now()) 
{    
}

Timer::~Timer()
{    
}

// Reset the timer
void Timer::reset() 
{
    start_time = std::chrono::high_resolution_clock::now();
}

// Get the elapsed time in seconds
double Timer::elapsedSeconds() const 
{
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = now - start_time;
    return elapsed.count();
}

// Get the elapsed time in milliseconds
double Timer::elapsedMilliseconds() const 
{
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = now - start_time;
    return elapsed.count();
}

// Check if the given number of milliseconds has elapsed
bool Timer::hasElapsedMilliseconds(double milliseconds) const 
{
    return elapsedMilliseconds() >= milliseconds;
}
