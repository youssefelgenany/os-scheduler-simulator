#include "scheduler.h"
#include "interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>

Scheduler systemScheduler;

void initScheduler(SchedulingAlgorithm algo, int initialQuantum) {
    systemScheduler.algorithm = algo;
    systemScheduler.quantum = (initialQuantum > 0) ? initialQuantum : 1; // * Ensure positive quantum
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

void cleanupScheduler() { // * Added cleanup function
    // Clean up all process references
    for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
        for (int j = 0; j < MAX_PROCESSES; j++) {
            systemScheduler.readyQueues[i][j] = NULL;
        }
        systemScheduler.readyCounts[i] = 0;
    }
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        systemScheduler.blockedQueue[i] = NULL;
    }
    systemScheduler.blockedCount = 0;
    systemScheduler.runningProcess = NULL;
}

void addProcess(PCB* pcb) {
    if (!pcb) return;
    int priority = (pcb->priority - 1) % MAX_PRIORITY_LEVELS; // * Ensure valid priority
//     // In scheduler.c when adding processes:
// int priority = pcb->priority % MAX_PRIORITY_LEVELS; // Now handles 0-3
    
    if (systemScheduler.readyCounts[priority] < MAX_PROCESSES) {
        systemScheduler.readyQueues[priority][systemScheduler.readyCounts[priority]++] = pcb;
        pcb->state = READY;
    }
}

void scheduleAndExecute(int currentCycle) {
    // Check current process
        if (systemScheduler.runningProcess){
            
        executeInstruction(systemScheduler.runningProcess);
        systemScheduler.currentQuantum++;
        
        
        if (systemScheduler.runningProcess->state == TERMINATED) {
            systemScheduler.runningProcess = NULL;
            systemScheduler.currentQuantum = 0;
        } else if (systemScheduler.runningProcess->state == BLOCKED){
            blockCurrentProcess();
        }
        
        if(systemScheduler.algorithm == FCFS){
            systemScheduler.currentQuantum = 0;
        }

        if (systemScheduler.runningProcess) {
        int quantumLimit = systemScheduler.quantum;
        if (systemScheduler.algorithm == MULTILEVEL_FEEDBACK) {
            int level = systemScheduler.runningProcess->priority - 1;
            quantumLimit = (level < 3) ? (1 << level) : quantumLimit;
        }
        
        if (systemScheduler.currentQuantum < quantumLimit) {
            return;
        } 
        
        systemScheduler.currentQuantum = 0;
        PCB* expiredProcess = systemScheduler.runningProcess;
        expiredProcess->state = READY;
        switch (systemScheduler.algorithm) {
            case ROUND_ROBIN:
                addProcess(expiredProcess);
                break;
            case MULTILEVEL_FEEDBACK:
                if (expiredProcess->priority < MAX_PRIORITY_LEVELS) {
                    expiredProcess->priority++;
                }
                addProcess(expiredProcess);
                break;
            case FCFS:
                return;
        }
        
        systemScheduler.runningProcess = NULL;
    }
    }

    // Select new process --- add en ehna nshayek the priority levels in EACH ready queue
 if(!systemScheduler.runningProcess){
    for (int prio = 0; prio < MAX_PRIORITY_LEVELS; prio++) {
        if (systemScheduler.readyCounts[prio] > 0) {
            for (int i = 0; i < systemScheduler.readyCounts[prio]; i++) {
                PCB* p = systemScheduler.readyQueues[prio][i];
                if (p && p->arrivalTime <= currentCycle) {
                    // Remove from queue 
                    for (int j = i; j < systemScheduler.readyCounts[prio]-1; j++) {
                        systemScheduler.readyQueues[prio][j] = systemScheduler.readyQueues[prio][j+1];
                    }
                    systemScheduler.readyCounts[prio]--;
                    
                    systemScheduler.runningProcess = p;
                    p->state = RUNNING;
                    systemScheduler.currentQuantum = 0;
                    return;
                }
            }
        }
        }
}
}





void blockCurrentProcess() {
    if (systemScheduler.runningProcess && systemScheduler.blockedCount < MAX_PROCESSES) {
        systemScheduler.runningProcess->state = BLOCKED;
        systemScheduler.blockedQueue[systemScheduler.blockedCount++] = systemScheduler.runningProcess;
        char log[128];
        snprintf(log, sizeof(log), "PID %d: Blocked", systemScheduler.runningProcess->pid);
        update_log_view(log);
        GtkWidget *main_window = get_main_window();
        if (main_window) {
            GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                                                      GTK_DIALOG_MODAL,
                                                      GTK_MESSAGE_INFO,
                                                      GTK_BUTTONS_OK,
                                                      "PID %d: Blocked", systemScheduler.runningProcess->pid);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
        }
        systemScheduler.runningProcess = NULL;
        systemScheduler.currentQuantum = 0;
    }
}


void unblockProcess(PCB* pcb) {
    if (!pcb) return;
    
    for (int i = 0; i < systemScheduler.blockedCount; i++) {
        if (systemScheduler.blockedQueue[i] == pcb) {
            for (int j = i; j < systemScheduler.blockedCount-1; j++) {
                systemScheduler.blockedQueue[j] = systemScheduler.blockedQueue[j+1];
            }
            systemScheduler.blockedCount--;
            addProcess(pcb);
            char log[128];
            snprintf(log, sizeof(log), "PID %d: Unblocked", pcb->pid);
            update_log_view(log);
            GtkWidget *main_window = get_main_window();
            if (main_window) {
                GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                                                          GTK_DIALOG_MODAL,
                                                          GTK_MESSAGE_INFO,
                                                          GTK_BUTTONS_OK,
                                                          "PID %d: Unblocked", pcb->pid);
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
            }
             break;
        }
    }
}

void terminateCurrentProcess() {
    if (systemScheduler.runningProcess) {
        systemScheduler.runningProcess->state = TERMINATED;
        systemScheduler.runningProcess = NULL;
        systemScheduler.currentQuantum = 0;
    }
}

PCB* getRunningProcess() {
    return systemScheduler.runningProcess;
}

void setQuantum(int newQuantum) {
    if (newQuantum > 0) {
        systemScheduler.quantum = newQuantum;
    }
}

// ... (all previous code remains exactly the same until displaySchedulerState)

void displaySchedulerState() {
    printf("\n=== Scheduler State ===\n");
    printf("Algorithm: ");
    switch (systemScheduler.algorithm) {
        case FCFS: printf("FCFS"); break;
        case ROUND_ROBIN: printf("Round Robin (q=%d)", systemScheduler.quantum); break;
        case MULTILEVEL_FEEDBACK: printf("MLFQ"); break;
    }
    
    printf("\nRunning: ");
    if (systemScheduler.runningProcess) {
        printf("PID %d (Prio %d, Q: %d/%d)", 
               systemScheduler.runningProcess->pid,
               systemScheduler.runningProcess->priority,
               systemScheduler.currentQuantum,
               systemScheduler.quantum); // Simplified display
    } else {
        printf("None");
    }
    
    printf("\nReady Queues:\n");
    for (int prio = 0; prio < MAX_PRIORITY_LEVELS; prio++) {
        printf("  Level %d (%d): ", prio+1, systemScheduler.readyCounts[prio]);
        for (int i = 0; i < systemScheduler.readyCounts[prio]; i++) {
            PCB* p = systemScheduler.readyQueues[prio][i];
            if (p) printf("%d (arrival: %d) ", p->pid, p->arrivalTime);
        }
        printf("\n");
    }
    
    printf("Blocked (%d): ", systemScheduler.blockedCount);
    for (int i = 0; i < systemScheduler.blockedCount; i++) {
        if (systemScheduler.blockedQueue[i]) printf("%d ", systemScheduler.blockedQueue[i]->pid);
    }
    printf("\n======================\n");
}