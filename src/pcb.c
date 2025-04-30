#include "pcb.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "memory.h"

#define PCB_BASE_WORDS 7
#define VARIABLE_WORDS 3
#define INSTRUCTION_WORD_SIZE 1

static PCB pcb_pool[MAX_ACTIVE_PROCESSES];
static int pcb_in_use[MAX_ACTIVE_PROCESSES];  // 0 = free, 1 = used

void initPCBs() {
    for (int i = 0; i < MAX_ACTIVE_PROCESSES; i++) {
        pcb_in_use[i] = 0;
    }
}
PCB* createPCB( int arrivalTime) {
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

// In pcb.c
int calculateProcessSize(PCB* pcb) {
    if (!pcb) return 0;
    return WORDS_PER_PROCESS; // All processes use the same fixed size
}

void setBoundaries(PCB* pcb, int* nextAvailableBlock) {
    if (!pcb || !nextAvailableBlock) return;
    
    int processSize = calculateProcessSize(pcb);
    int requiredBlocks = 1; // msh hatzeed 3n 1
    pcb->memoryLowerBound = *nextAvailableBlock * WORDS_PER_PROCESS;
    pcb->memoryUpperBound = pcb->memoryLowerBound + (requiredBlocks * WORDS_PER_PROCESS) - 1;
    if (pcb->memoryUpperBound >= TOTAL_MEMORY_WORDS) {
        pcb->memoryUpperBound = TOTAL_MEMORY_WORDS - 1;
    }
    
    *nextAvailableBlock += requiredBlocks;
}

// void cleanupPCB(PCB* pcb) { // * Added cleanup function
//     if (!pcb) return;
//     destroyPCB(pcb);
// }

void cleanupPCB(PCB* pcb) {
    if (!pcb) return;
    // Free any allocated instruction strings
    for (int i = 0; i < pcb->instructionCount; i++) {
        if (pcb->instructions[i]) {
            free(pcb->instructions[i]);
            pcb->instructions[i] = NULL;
        }
    }
    // Mark slot as free
    int index = pcb - pcb_pool;
    if (index >= 0 && index < MAX_ACTIVE_PROCESSES) {
        pcb_in_use[index] = 0;
    }
}

void updatePCBState(PCB* pcb, ProcessState newState) {
    if (pcb) pcb->state = newState;
}

void displayPCB(PCB* pcb) {
    if (!pcb) {
        printf("PCB: NULL\n");
        return;
    }

    const char* stateStr;
    switch (pcb->state) {
        case READY: stateStr = "READY"; break;
        case RUNNING: stateStr = "RUNNING"; break;
        case BLOCKED: stateStr = "BLOCKED"; break;
        case TERMINATED: stateStr = "TERMINATED"; break;
        default: stateStr = "UNKNOWN"; break;
    }

    printf("PCB %d:\n", pcb->pid);
    printf("  State: %s\n", stateStr);
    printf("  Priority: %d\n", pcb->priority);
    printf("  PC: %d/%d\n", pcb->programCounter, pcb->instructionCount);
    printf("  Memory: [%d-%d] (%d words)\n", 
           pcb->memoryLowerBound, 
           pcb->memoryUpperBound,
           pcb->memoryUpperBound - pcb->memoryLowerBound + 1);
    
    printf("  Variables:\n");
    for (int i = 0; i < MAX_VARIABLES; i++) {
        if (pcb->variables[i][0] != '\0') {
            printf("    %d: %s\n", i, pcb->variables[i]);
        }
    }

    printf("  Instructions (%d):\n", pcb->instructionCount);
    for (int i = 0; i < pcb->instructionCount; i++) {
        printf("    %d: %s\n", i, pcb->instructions[i] ? pcb->instructions[i] : "NULL");
    }
}
// printf("PCB %d: State=%s, Prio=%d, PC=%d/%d, Mem=[%d-%d]\n",
//     pcb->pid, stateStr, pcb->priority,
//     pcb->programCounter, pcb->instructionCount,
//     pcb->memoryLowerBound, pcb->memoryUpperBound);
// }

int addInstruction(PCB* pcb, const char* instruction) {
    if (!pcb || !instruction || pcb->instructionCount >= MAX_INSTRUCTIONS) {
        return 0;
    }

    pcb->instructions[pcb->instructionCount] = strdup(instruction);
    if (!pcb->instructions[pcb->instructionCount]) {
        return 0;
    }

    pcb->instructionCount++;
    return 1;
}

int setVariable(PCB* pcb, int index, const char* value) {
    if (!pcb || index < 0 || index >= MAX_VARIABLES || !value) {
        return 0;
    }

    strncpy(pcb->variables[index], value, sizeof(pcb->variables[index]) - 1);
    pcb->variables[index][sizeof(pcb->variables[index]) - 1] = '\0';
    return 1;
}

const char* getVariable(PCB* pcb, int index) {
    if (!pcb || index < 0 || index >= MAX_VARIABLES) {
        return NULL;
    }
    return pcb->variables[index];
}