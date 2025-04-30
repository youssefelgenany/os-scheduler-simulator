#ifndef PCB_H
#define PCB_H

#define MAX_INSTRUCTIONS 10     
#define MAX_PROCESS_SIZE 20     // * Actually used in memory allocation
#define MAX_VARIABLES 3         
#define MAX_VAR_NAME_LENGTH 30  // * Added for consistency 
#define MAX_VAR_VALUE_LENGTH 100 // * Added for consistency

typedef enum {
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
} ProcessState;

typedef struct {
    int pid;
    ProcessState state;
    int priority;               // 1-4 talking about levels
    int programCounter;
    int memoryLowerBound;
    int memoryUpperBound;
    char* instructions[MAX_INSTRUCTIONS];
    int instructionCount;
    char variables[MAX_VARIABLES][MAX_VAR_VALUE_LENGTH]; // * Used consistent size
    int arrivalTime;
} PCB;
void initPCBs();
PCB* createPCB(int arrivalTime); // make sure of pid whether its automatic, and the max instructions is 10
void cleanupPCB(PCB* pcb);
void updatePCBState(PCB* pcb, ProcessState newState);
int calculateProcessSize(PCB* pcb);
void setBoundaries(PCB* pcb, int* nextAvailableBlock);
void displayPCB(PCB* pcb);
int addInstruction(PCB* pcb, const char* instruction);
int setVariable(PCB* pcb, int index, const char* value);
const char* getVariable(PCB* pcb, int index);

#endif