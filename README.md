# SistemiOperativi2024 - Project Report

[![C](https://img.shields.io/badge/C-Language-00599C?style=flat&logo=c&logoColor=white)]()
[![POSIX](https://img.shields.io/badge/POSIX-API-0078D4?style=flat)]()
[![Threads](https://img.shields.io/badge/Threads-Concurrency-red?style=flat)]()
[![Processes](https://img.shields.io/badge/Processes-Fork-green?style=flat)]()
[![Shared Memory](https://img.shields.io/badge/SharedMemory-Memory-yellow?style=flat)]()
[![Semaphores](https://img.shields.io/badge/Semaphores-Synchronization-blue?style=flat)]()
[![Message Queues](https://img.shields.io/badge/Message%20Queues-Communication-purple?style=flat)]()
[![Signal Handling](https://img.shields.io/badge/Signal%20Handling-Signals-pink?style=flat)]()
[![IPC](https://img.shields.io/badge/IPC-Interprocess-Comm-00bfff?style=flat)]()
[![System Programming](https://img.shields.io/badge/System%20Programming-Systems-8a2be2?style=flat)]()

## Master

The `master.c` module acts as the main controller for the simulation system, coordinating processes responsible for energy management and chemical reactions. It uses shared memory and semaphores to synchronize with processes such as `atom`, `activator`, `alimentation`, and `inhibitor`.

The simulation involves creating atoms, managing their splitting, and generating energy, while also handling energy consumption and waste management.

The `stats` structure includes shared pointers to data for managing general configuration parameters, controlling the atom, activator, alimentation and inhibitor processes, and tracking simulation metrics. It contains:

- **Configuration parameters** (e.g., initial energy demand, initial atom count, maximum and minimum atomic number, simulation duration)
- **varibles for periodic prints** (e.g., activation counts, energy consumption)
- **Various IDs** for shared memory, message queues, and semaphores, along with a PID for managing the inhibitor process

The `main()` function initializes signal handling, shared memory, and loads configuration parameters from a file into shared memory. It sets up the message queue and semaphores, then forks and launches processes for atoms, the activator, and alimentation using `execve`. It also prompts the user to enable the inhibitor via the terminal, activating it if requested.

Once setup is complete, the simulation begins, synchronizing all processes with semaphores and performing periodic updates. During the simulation, energy is deducted at regular intervals, and the execution halts if an `EXPLODE` or `BLACKOUT` event occurs. If the specified simulation time elapses, execution concludes as a `TIMEOUT`. In all exit scenarios, the function waits for all processes to terminate, releases resources (semaphore, message queue, and shared memory), and terminates the master process.


### Process Initialization

The functions `init_atoms()`, `init_alimentation()`, `init_activator()`, and `init_inhibitor()` create and manage processes for atoms, the alimentation, and the activator. These functions use the helper function `run_process()` to initiate child processes. If too many atoms are initialized, the fork() function might fail, leading to a meltdown.


### Signal Handlers

The `sigint_handler()` function sets up handlers for key signal like `SIGINT`, used for a controlled shutdown.


### Reporting Functions

The `periodic_print()` function generates reports every second, providing updated information on atoms, energy levels, waste and more. If activated, it also logs the actions of the inhibitor process.


### Terminating the Simulation

At the end of the simulation, regardless of the exit scenario, child processes are terminated, shared memory segments are detached, and IPC resources are cleaned up. The `close_and_exit()` function manages the shutdown process. To confirm that all atoms have terminated, the semaphore acts as a process counter. If the inhibitor was activated but is currently paused, it is reactivated by sending a signal, allowing it to recognize that it should terminate. The function then waits up to 5 seconds to monitor for any changes in the number of active processes. If the process count remains unchanged, it sends a `SIGINT` signal to all processes that need to be shut down. If `SIGINT` fails, it waits 3 more seconds, then sends a `SIGKILL` signal to any remaining processes.


## Shared Memory

The `lib/shm.h` library assists with creating, attaching, detaching, and destroying shared memory segments.


## Semaphores

The `lib/semaphore.h` library facilitates the creation and management of semaphore arrays. These are used to synchronize the start and end of the simulation, manage energy sharing across processes, and ensure mutual exclusion when accessing shared memory variables.


## Messaging

Communication between the activator and atoms is handled through message structures defined in `src/msg_communication.h` and implemented in `msg_comunication.c`. These components facilitate the creation and removal of message queues, as well as the creation, sending, and receiving of messages.

---

## Atom

The atom is the most important part of the simulation, it manages its atomic number and energy production, splits upon receiving a message from the activator, and becomes waste if its atomic number drops too low.

**Core Functions**:
- **`main()`**: Serves as the starting point for the atom process. Its key responsibilities include:
1. Setting up signal handlers using `signal_handler_init()` to allow the process to respond to termination.
2. Attaching to shared memory via `shm_info_attach()`, to gain access to semaphores and shared data.
3. Main loop where the atom decides whether to split, and updates status counters before eventually terminating or generating new child processes.
- **`split()`**: Simulates the "splitting" or division of an atom into child processes and distributes atomic number between the parent and new child atom using a pipe and creates energy calculated by the `energy()` function.
- **`adaptive_probability()`**: Used when the inhibitor process is active, it calculates an adaptive probability value, determining if a process can split based on the most restrictive resource limit chosen between free memory using `get_free_memory()` , user process limit using `get_max_user_processes()`, and cgroup process limit using `read_pids_max()`.
- **`update_energy()`**: Manages energy production and updates for atomic processes. Computes energy levels and compares them to a critical threshold. If energy exceeds this threshold, the process signals an explosion, halts execution, and calls `close_and_exit`.
It updates shared memory with the total energy produced and adjusts energy levels based on inhibition factors when the inhibitor process is active.

## Activator

The activator process manages atom splits by sending split messages at specified intervals. It runs in a loop, checking the semaphore for control signals and sending a split message when appropriate.

**Core Functions**:
- **`main()`**: Sets up the shared memory and retrieves the time interval (`step_nsec`) from the shared configuration to control the timing between split actions. The main loop monitors the semaphore for split availability, and when conditions are met, it sends a split message. On shutdown, it calls `close_and_exit()` to release resources.
- **`send_split_msg()`**: Constructs and sends a message to indicate an atom split, sent via the message queue.
- **`nsleep()`**: Implements a precise sleep mechanism using `nanosleep()` to pause execution for `step_nsec` nanoseconds, controlling the periodic split activity.
- **`close_and_exit()`**: Detaches from shared memory, ensuring a clean shutdown for the activator process.


## Alimentation

The alimentation process periodically creates new atoms based on simulation parameters.

**Core Functions**:
- **`main()`**: Attaches to shared memory, retrieves parameters like the number of new atoms and step duration, and waits for synchronization via semaphores before starting the main loop. It then creates new atoms periodically and exits once the simulation concludes, cleaning up resources.
- **`nsleep()`**: Pauses the process for a specified nanosecond duration (`step_nsec`), allowing for periodic updates.
- **`close_and_exit()`**: Manages the cleanup of resources and exits the process. It detaches shared memory and exits gracefully.

## Inhibitor 

The inhibitor process alternates between active and paused states in response to signals, interacting with shared resources and semaphores. It also communicates its current status and ensures proper shutdown when required.

**Core Functions**:
- **`main()`**: Sets up signal handlers for `SIGUSR1` and `SIGUSR2` to pause and resume execution. It attaches to shared memory and checks the semaphore to confirm readiness before activating the inhibitor. The process continuously checks whether it should remain active or paused, and if the simulation ends, it releases resources and exits.
- **`handle_sigusr1()`**: Pauses the inhibitor by setting the appropriate semaphore value, halting the inhibitor’s main loop.
- **`handle_sigusr2()`**: Resumes the inhibitor by resetting the semaphore value, allowing the process to continue.
- **`close_and_exit()`**: Releases semaphore and shared memory resources before terminating the process.
