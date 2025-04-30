#ifndef MEMORY_H
#define MEMORY_H

#include "pcb.h"

#define TOTAL_MEMORY_WORDS 60
#define WORDS_PER_PROCESS 20
#define MAX_ACTIVE_PROCESSES 3

typedef struct {
    char name[30];              // Name of the memory item
    char value[100];            // Stored data
    int owner_pid;              // Process ID that owns this word (-1 for free)
} MemoryWord;

// Memory management functions
void initMemory();
int allocateProcessMemory(PCB* pcb);
void freeProcessMemory(int pid);
int writeToMemory(int pid, const char* name, const char* value);
const char* readFromMemory(int pid, const char* name);
void displayMemory();
int getAvailableMemory();

extern MemoryWord memory[TOTAL_MEMORY_WORDS];

#endif