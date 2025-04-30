#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "pcb.h"

#define MAX_PROCESSES 3
#define MAX_PRIORITY_LEVELS 4

typedef enum {
    FCFS,
    ROUND_ROBIN,
    MULTILEVEL_FEEDBACK
} SchedulingAlgorithm;

typedef struct {
    PCB* readyQueues[MAX_PRIORITY_LEVELS][MAX_PROCESSES];
    int readyCounts[MAX_PRIORITY_LEVELS];
    PCB* blockedQueue[MAX_PROCESSES];
    int blockedCount;
    PCB* runningProcess;
    SchedulingAlgorithm algorithm;
    int quantum;                // For Round Robin (user-defined)
    int currentQuantum;         // Counter for current quantum
} Scheduler;

extern Scheduler systemScheduler;
void update_log_view(const char *message);
// Initialize scheduler with specific algorithm
void initScheduler(SchedulingAlgorithm algo, int initialQuantum);

// Add process to appropriate ready queue
void addProcess(PCB* pcb);

// Schedule and execute one instruction
void scheduleAndExecute(int currentCycle);

// Block current running process
void blockCurrentProcess();

// Unblock a process (move from blocked to ready queue)
void unblockProcess(PCB* pcb);

// Terminate current running process
void terminateCurrentProcess();

// Get currently running process
PCB* getRunningProcess();

// Display scheduler state
void displaySchedulerState();

// Set quantum for Round Robin
void setQuantum(int newQuantum);

void cleanupScheduler(); // * Added cleanup function

#endif 