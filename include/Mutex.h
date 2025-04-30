#ifndef MUTEX_H
#define MUTEX_H

#include "pcb.h"
#include "memory.h"
#include <stdbool.h>

typedef enum {
    RESOURCE_INVALID,
    userInput,
    userOutput,
    file
} ResourceType;

// In Mutex.h
typedef struct {

    ResourceType type;
    int holder_pid;
    PCB* blocked_queue[MAX_ACTIVE_PROCESSES]; // Fixed-size array
    bool isLocked;
    int queue_size;
} Mutex;

// Global mutex declarations
extern Mutex mutex_user_input;
extern Mutex mutex_user_output;
extern Mutex mutex_file;
void update_log_view(const char *message);
// Function prototypes
void init_mutexes();
void sem_wait(ResourceType resource, PCB* process); // * Added PCB parameter
void sem_signal(ResourceType resource, PCB* process); // * Added PCB parameter
Mutex* get_mutex(ResourceType resource_name);


#endif