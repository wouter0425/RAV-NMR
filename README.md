# CoreGuard-NMR

CoreGuard-NMR is created as a way to create and validate redundancy techniques in schedulers for my thesis. It supports N-modular redundancy.
The current main file implements 2 schedules, one using TMR and one running normally. The scheduling technique used is load balancing.
The scheduler tries to distribute the tasks over the available cores.

Additionally, this framework is able to log various parameters while the scheduler runs:
- **results/cores:** The utility of the CPU cores
- **results/tasks:** The number of successfull runs of the tasks
- **results/weights:** The reliability of the cores

## Usage
- you can modify the parameters in defines.h which explains itself
- run make
- sudo ./bin/main