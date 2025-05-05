# RAV-NMR

RAV-NMR is created as a way to create and validate NMR techniques in schedulers for my thesis. It supports both traditional NMR and my proposed RAV-NMR

Additionally, this framework is able to log various parameters while the scheduler runs:
- **results/cores:** The utility of the CPU cores
- **results/tasks:** The number of successfull runs of the tasks
- **results/weights:** The reliability of the cores

## Usage
- you can modify the parameters in defines.h which explains itself
- run make
- sudo ./bin/main
