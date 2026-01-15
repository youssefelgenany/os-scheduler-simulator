# OS Scheduler Simulator

## Motivation

The OS Scheduler Simulator is an educational desktop application designed to demonstrate core operating system concepts through interactive simulation. It provides a visual and hands-on approach to understanding process scheduling algorithms, memory management, process synchronization, and process state transitions. This project serves as a practical learning tool for students and developers interested in understanding how operating systems manage processes, allocate memory, and handle resource synchronization in a controlled environment.

## Build Status

**Current Status:** Functional with known limitations

**Known Issues:**
- Maximum of 3 processes can be loaded simultaneously
- Memory allocation uses fixed-size blocks per process
- GUI may become unresponsive during long-running simulations
- File I/O operations are simulated and do not interact with the actual filesystem
- Error handling for invalid instruction formats could be improved
- Memory fragmentation is not handled (fixed allocation strategy)

**Future Improvements:**
- Support for more concurrent processes
- Dynamic memory allocation strategies
- Additional scheduling algorithms (Shortest Job First, Priority Scheduling)
- Enhanced error recovery mechanisms
- Performance optimizations for large-scale simulations

## Code Style

This project follows the following coding conventions:

- **Naming Convention:** 
  - Functions: `camelCase` (e.g., `executeInstruction`, `handlePrint`)
  - Variables: `camelCase` (e.g., `programCounter`, `arrivalTime`)
  - Constants: `UPPER_SNAKE_CASE` (e.g., `MAX_PROCESSES`, `MAX_INSTRUCTIONS`)
  - Types: `PascalCase` (e.g., `PCB`, `Scheduler`, `ProcessState`)
  - File names: `camelCase.c` and `camelCase.h` (e.g., `interpreter.c`, `scheduler.h`)

- **Code Organization:**
  - Header files in `include/` directory
  - Source files in `src/` directory
  - One header file per source file with corresponding `.h` file
  - Function declarations in header files, implementations in source files

- **Indentation:** 4 spaces (no tabs)
- **Line Length:** Maximum 100 characters per line
- **Comments:** 
  - Function-level comments for complex logic
  - Inline comments for non-obvious operations
  - Header guards using `#ifndef` / `#define` / `#endif`

- **Memory Management:**
  - Explicit memory allocation and deallocation
  - Cleanup functions for all data structures
  - Null pointer checks before dereferencing

## Tech/Framework Used

This project is built using:

- **Programming Language:** C (C99 standard)
- **GUI Framework:** GTK+ 3.0 (GTK3) for the graphical user interface
- **Build System:** GNU Make
- **Compiler:** GCC (GNU Compiler Collection)
- **Development Tools:**
  - `pkg-config` for managing GTK3 dependencies
  - Standard C libraries: `stdio.h`, `stdlib.h`, `string.h`, `ctype.h`

**Dependencies:**
- GTK+ 3.0 development libraries
- GCC compiler with C support
- Make utility
- pkg-config

## Features

The OS Scheduler Simulator includes the following features:

1. **Multiple Scheduling Algorithms:**
   - First-Come-First-Served (FCFS)
   - Round Robin with configurable quantum
   - Multilevel Feedback Queue

2. **Process Management:**
   - Create and manage up to 3 processes simultaneously
   - Track process states (Ready, Running, Blocked, Terminated)
   - Process Control Block (PCB) with priority, program counter, and memory bounds
   - Arrival time specification for each process

3. **Instruction Set:**
   - `print` - Display variable values
   - `assign` - Assign values to variables
   - `writeFile` - Write data to files
   - `readFile` - Read data from files
   - `printFromTo` - Print range of values
   - `semWait` - Wait on semaphore/mutex
   - `semSignal` - Signal semaphore/mutex

4. **Memory Management:**
   - Fixed-size memory allocation per process
   - Memory visualization showing allocated blocks
   - Variable storage within process memory space
   - Memory bounds tracking (lower and upper bounds)

5. **Synchronization:**
   - Mutex support for three resources: `userInput`, `userOutput`, and `file`
   - Process blocking and unblocking based on resource availability
   - Blocked queue management

6. **Graphical User Interface:**
   - Real-time process list view
   - Ready and blocked queue visualization
   - Mutex status display
   - Memory allocation view
   - Execution log
   - Control buttons (Start, Step, Reset, Add Process)
   - Scheduling algorithm selection
   - Quantum configuration for Round Robin

7. **Simulation Controls:**
   - Start/Pause continuous execution
   - Step-by-step execution mode
   - Reset simulation to initial state
   - Load custom program files

## Code Examples

### Example 1: Process Creation and Initialization

```c
PCB* createPCB(int arrivalTime) {
    for (int i = 0; i < MAX_ACTIVE_PROCESSES; i++) {
        if (!pcb_in_use[i]) {
            pcb_in_use[i] = 1;
            PCB* pcb = &pcb_pool[i];
            // Initialize fields
            pcb->pid = i + 1;
            pcb->state = READY;
            pcb->priority = 1;
            pcb->programCounter = 0;
            pcb->memoryLowerBound = -1;
            pcb->memoryUpperBound = -1;
            pcb->instructionCount = 0;
            pcb->arrivalTime = arrivalTime;
            for (int v = 0; v < MAX_VARIABLES; v++) {
                pcb->variables[v][0] = '\0';
            }
            for (int ins = 0; ins < MAX_INSTRUCTIONS; ins++) {
                pcb->instructions[ins] = NULL;
            }
            return pcb;
        }
    }
    return NULL; // No free PCB available
}
```

### Example 2: Instruction Execution

```c
void executeInstruction(PCB* pcb) {
    if (!pcb || pcb->state != RUNNING) return;
    
    if (pcb->programCounter >= pcb->instructionCount) {
        pcb->state = TERMINATED;
        return;
    }

    char* instruction = pcb->instructions[pcb->programCounter];
    if (!instruction) {
        pcb->programCounter++;
        return;
    }

    char command[MAX_ARG_LENGTH], arg1[MAX_ARG_LENGTH], arg2[MAX_ARG_LENGTH];
    int numArgs = sscanf(instruction, "%49s %49s %49s", command, arg1, arg2);
    
    if (strcmp(command, "print") == 0 && numArgs >= 2) {
        handlePrint(pcb, arg1);
    }
    else if (strcmp(command, "assign") == 0 && numArgs >= 3) {
        handleAssign(pcb, arg1, arg2);
    }
    // ... additional command handlers
    if (pcb->state == RUNNING) {
        pcb->programCounter++;
    }
}
```

### Example 3: Scheduler Initialization

```c
void initScheduler(SchedulingAlgorithm algo, int initialQuantum) {
    systemScheduler.algorithm = algo;
    systemScheduler.quantum = (initialQuantum > 0) ? initialQuantum : 1;
    systemScheduler.currentQuantum = 0;
    systemScheduler.runningProcess = NULL;
    systemScheduler.blockedCount = 0;

    for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
        systemScheduler.readyCounts[i] = 0;
        for (int j = 0; j < MAX_PROCESSES; j++) {
            systemScheduler.readyQueues[i][j] = NULL;
        }
    }

    for (int i = 0; i < MAX_PROCESSES; i++) {
        systemScheduler.blockedQueue[i] = NULL;
    }
}
```

### Example 4: Memory Allocation

```c
int allocateProcessMemory(PCB* pcb) {
    if (!pcb) return 0;
    
    int processSize = calculateProcessSize(pcb);
    int requiredBlocks = 1;
    int nextBlock = 0;
    
    // Find next available block
    for (int i = 0; i < TOTAL_MEMORY_WORDS; i += WORDS_PER_PROCESS) {
        if (memory[i].owner_pid == -1) {
            nextBlock = i / WORDS_PER_PROCESS;
            break;
        }
    }
    
    setBoundaries(pcb, &nextBlock);
    
    // Allocate memory
    for (int i = pcb->memoryLowerBound; i <= pcb->memoryUpperBound; i++) {
        if (i < TOTAL_MEMORY_WORDS) {
            memory[i].owner_pid = pcb->pid;
            memory[i].name[0] = '\0';
            memory[i].value[0] = '\0';
        }
    }
    return 1;
}
```

### Example 5: Semaphore Wait Operation

```c
void sem_wait(ResourceType resource, PCB* process) {
    if (resource == RESOURCE_INVALID || !process) return;
    
    Mutex* mutex = getMutex(resource);
    if (!mutex) return;
    
    if (mutex->isLocked) {
        // Add to blocked queue
        if (mutex->queue_size < MAX_PROCESSES) {
            mutex->blocked_queue[mutex->queue_size++] = process;
            process->state = BLOCKED;
            blockCurrentProcess();
        }
    } else {
        // Acquire lock
        mutex->isLocked = 1;
        mutex->holder_pid = process->pid;
    }
}
```

### Example 6: Scheduling and Execution

```c
void scheduleAndExecute(int currentCycle) {
    // Check current process
    if (systemScheduler.runningProcess) {
        executeInstruction(systemScheduler.runningProcess);
        systemScheduler.currentQuantum++;
        
        if (systemScheduler.runningProcess->state == TERMINATED) {
            systemScheduler.runningProcess = NULL;
            systemScheduler.currentQuantum = 0;
        } else if (systemScheduler.runningProcess->state == BLOCKED) {
            blockCurrentProcess();
        }
        
        // Check quantum expiration
        if (systemScheduler.currentQuantum >= systemScheduler.quantum) {
            // Move to ready queue based on algorithm
            PCB* expiredProcess = systemScheduler.runningProcess;
            expiredProcess->state = READY;
            addProcess(expiredProcess);
            systemScheduler.runningProcess = NULL;
            systemScheduler.currentQuantum = 0;
        }
    }
    
    // Select next process to run
    if (!systemScheduler.runningProcess) {
        // Select from ready queues based on algorithm
        // ... selection logic
    }
}
```

## Installation

### Prerequisites

Before building the project, ensure you have the following installed:

1. **GCC Compiler:**
   - On Linux: `sudo apt-get install gcc` (Ubuntu/Debian) or `sudo yum install gcc` (RHEL/CentOS)
   - On macOS: Install Xcode Command Line Tools: `xcode-select --install`
   - On Windows: Install MinGW-w64 or use WSL (Windows Subsystem for Linux)

2. **GTK+ 3.0 Development Libraries:**
   - On Linux: `sudo apt-get install libgtk-3-dev` (Ubuntu/Debian) or `sudo yum install gtk3-devel` (RHEL/CentOS)
   - On macOS: `brew install gtk+3`
   - On Windows: Install GTK+ 3.0 from [gtk.org](https://www.gtk.org/docs/installations/windows/)

3. **pkg-config:**
   - Usually included with GTK+ installation
   - On Linux: `sudo apt-get install pkg-config`
   - On macOS: Included with Homebrew GTK installation

4. **Make Utility:**
   - On Linux: Usually pre-installed
   - On macOS: Included with Xcode Command Line Tools
   - On Windows: Install via MinGW or use WSL

### Building the Project

1. **Clone or download the repository:**
   ```bash
   git clone <repository-url>
   cd os-scheduler-simulator
   ```

2. **Compile the project:**
   ```bash
   make
   ```

3. **Run the simulator:**
   ```bash
   ./os-project
   ```

4. **Clean build artifacts (optional):**
   ```bash
   make clean
   ```


## Contribute

Contributions to this project are welcome! Here are areas where contributions would be valuable:

1. **Additional Scheduling Algorithms:**
   - Shortest Job First (SJF)
   - Priority Scheduling
   - Shortest Remaining Time First (SRTF)

2. **Enhanced Memory Management:**
   - Dynamic memory allocation strategies
   - Memory fragmentation handling
   - Memory compaction algorithms

3. **Code Improvements:**
   - Better error handling and validation
   - Performance optimizations
   - Code refactoring for maintainability
   - Additional unit tests

4. **Documentation:**
   - Improved code comments
   - User guide documentation
   - Algorithm explanations

5. **Features:**
   - Support for more concurrent processes
   - Additional instruction types
   - Export/import simulation configurations
   - Statistics and performance metrics

**How to Contribute:**
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes and test thoroughly
4. Commit your changes (`git commit -m 'Add amazing feature'`)
5. Push to the branch (`git push origin feature/amazing-feature`)
6. Open a Pull Request

Please ensure your code follows the existing code style and includes appropriate comments.

## Credits

This project was developed as a part of the GUC operating systems course. The following resources were used for reference and learning:

- **GTK+ Documentation:** [GTK+ 3 Reference Manual](https://docs.gtk.org/gtk3/)
- **Operating System Concepts:** Based on standard OS textbooks and academic materials covering:
  - Process scheduling algorithms
  - Memory management techniques
  - Process synchronization (semaphores and mutexes)
  - Process Control Blocks (PCB)
- **C Programming Resources:**
  - Standard C library documentation
  - Memory management best practices
- **Build System:**
  - GNU Make documentation
  - pkg-config usage guides

