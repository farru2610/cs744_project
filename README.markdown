# Distributed Task Scheduling System

This project implements a distributed task scheduling system for computing tasks (e.g: factorials for now), designed as part of the "Design and Engineering of Computing Services" course. The system uses a client-server architecture with TCP sockets, `epoll` for scalable I/O, and multithreading for concurrent task processing.

## Table of Contents
- [Overview](#overview)
- [System Components](#system-components)
- [Requirements](#requirements)
- [Setup Instructions](#setup-instructions)
- [Usage](#usage)
- [Performance Monitoring](#performance-monitoring)
- [Contributing](#contributing)
- [License](#license)

## Overview
The system consists of a scheduler that coordinates tasks between clients and workers. Clients submit factorial computation tasks (a single integer), the scheduler distributes tasks to available workers, and workers compute the factorial using a thread pool. The system supports multiple concurrent clients and workers, with a task queue to handle load spikes.

## System Components
- **client.c**: Sends a factorial task to the scheduler and receives the result.
- **scheduler.c**: Manages client and worker connections, queues tasks, and assigns them to workers using `epoll` for event handling.
- **worker.c**: Computes factorials using a thread pool (4 threads) and returns results to the scheduler.
- **task.h**: Defines the `Task` structure (an integer for factorial input).
- **loadgen.sh**: Bash script to simulate multiple clients with random inputs.
- **performance.sh**: Bash script to measure system throughput and task completion time.

## Requirements
- **Operating System**: Linux (tested on Ubuntu)
- **Compiler**: GCC
- **Libraries**: Standard C libraries (`pthread`, `arpa/inet`, `sys/epoll`, etc.)
- **Tools**: Bash shell for running scripts
- **Hardware**: Multi-core CPU recommended for worker thread pool

## Setup Instructions
1. **Clone the Repository**:
   ```bash
   git clone https://github.com/your-username/your-repo-name.git
   cd your-repo-name
   ```

2. **Compile the Code**:
   ```bash
   gcc -o client client.c -pthread
   gcc -o scheduler scheduler.c -pthread
   gcc -o worker worker.c -pthread
   ```

3. **Ensure Scripts are Executable**:
   ```bash
   chmod +x loadgen.sh performance.sh
   ```

## Usage
1. **Start the Scheduler**:
   ```bash
   ./scheduler
   ```
   The scheduler listens on `localhost:8080`.

2. **Start Workers** (in separate terminals):
   ```bash
   ./worker
   ```
   Start multiple workers (up to 10) to handle tasks.

3. **Run a Client**:
   ```bash
   ./client <number>
   ```
   Example: `./client 5` computes the factorial of 5.

4. **Simulate Multiple Clients**:
   ```bash
   ./loadgen.sh <number_of_clients>
   ```
   Example: `./loadgen.sh 10` spawns 10 clients with random inputs (1–10).

5. **Measure Performance**:
   ```bash
   ./performance.sh <number_of_clients>
   ```
   Example: `./performance.sh 10` runs 10 clients and logs metrics to `server_performance.log`.

## Performance Monitoring
The `performance.sh` script measures:
- **Elapsed Time**: Total time to process all client tasks.
- **Throughput**: Tasks completed per second.
- **Completed Tasks**: Number of successful tasks.

Results are logged to `server_performance.log`, and client outputs are saved in `client_output.log`.

## Contributing
Contributions are welcome! Please follow these steps:
1. Fork the repository.
2. Create a feature branch (`git checkout -b feature-name`).
3. Commit your changes (`git commit -m "Add feature"`).
4. Push to the branch (`git push origin feature-name`).
5. Open a pull request.

Please include tests and documentation for new features.

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
