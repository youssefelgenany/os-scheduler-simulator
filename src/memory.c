#include "interpreter.h"
#include "pcb.h"
#include "memory.h"
#include "Mutex.h"
#include <string.h>
#include <stdio.h>



MemoryWord memory[TOTAL_MEMORY_WORDS];

void initMemory() {
    for (int i = 0; i < TOTAL_MEMORY_WORDS; i++) {
        memory[i].name[0] = '\0';
        memory[i].value[0] = '\0';
        memory[i].owner_pid = -1;
    }
}

int allocateProcessMemory(PCB* pcb) {
    if (!pcb) return 0; // * Added null check
    
    for (int block_start = 0; block_start <= TOTAL_MEMORY_WORDS - WORDS_PER_PROCESS; block_start += WORDS_PER_PROCESS) {
        int block_free = 1;
        
        for (int i = block_start; i < block_start + WORDS_PER_PROCESS; i++) {
            if (memory[i].owner_pid != -1) {
                block_free = 0;
                break;
            }
        }
        
        if (block_free) {
            for (int i = block_start; i < block_start + WORDS_PER_PROCESS; i++) {
                memory[i].owner_pid = pcb->pid;
            }
            
            pcb->memoryLowerBound = block_start;
            pcb->memoryUpperBound = block_start + WORDS_PER_PROCESS - 1;
            return 1;
        }
    }
    return 0;
}

void freeProcessMemory(int pid) {
    for (int i = 0; i < TOTAL_MEMORY_WORDS; i++) {
        if (memory[i].owner_pid == pid) {
            memory[i].owner_pid = -1;
            memory[i].name[0] = '\0';
            memory[i].value[0] = '\0';
        }
    }
}

int writeToMemory(int pid, const char* name, const char* value) {
    if (!name || !value) return 0; // * Added null checks
    
    for (int i = 0; i < TOTAL_MEMORY_WORDS; i++) {
        if (memory[i].owner_pid == pid && memory[i].name[0] == '\0') {
            strncpy(memory[i].name, name, sizeof(memory[i].name) - 1);
            strncpy(memory[i].value, value, sizeof(memory[i].value) - 1);
            memory[i].name[sizeof(memory[i].name) - 1] = '\0';
            memory[i].value[sizeof(memory[i].value) - 1] = '\0';
            return 1;
        }
    }
    return 0;
}

const char* readFromMemory(int pid, const char* name) {
    if (!name) return NULL; // * Added null check
    
    for (int i = 0; i < TOTAL_MEMORY_WORDS; i++) {
        if (memory[i].owner_pid == pid && strcmp(memory[i].name, name) == 0) {
            return memory[i].value;
        }
    }
    return NULL;
}

void displayMemory() {
    printf("Memory State:\n");
    printf("-------------------------------------------------\n");
    for (int i = 0; i < TOTAL_MEMORY_WORDS; i++) {
        printf("Word %2d: ", i);
        if (memory[i].owner_pid == -1) {
            printf("[FREE]\n");
        } else {
            printf("PID %d: %s = %s\n", 
                   memory[i].owner_pid, 
                   memory[i].name, 
                   memory[i].value);
        }
    }
    printf("-------------------------------------------------\n");
}

int getAvailableMemory() {
    int count = 0;
    for (int i = 0; i < TOTAL_MEMORY_WORDS; i++) {
        if (memory[i].owner_pid == -1) {
            count++;
        }
    }
    return count;
}