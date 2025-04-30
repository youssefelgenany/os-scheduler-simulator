#include "Mutex.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h> // * Added for printf
#include <memory.h>
#include <scheduler.h>
#include <gtk/gtk.h> // Add for GtkMessageDialog
#include "interpreter.h"

// Define global mutexes

Mutex mutex_user_input = {userInput, -1, {0}, false, 0};
Mutex mutex_user_output = {userOutput, -1, {0}, false, 0};
Mutex mutex_file = {file, -1, {0}, false, 0};

void init_mutexes() {
    mutex_user_input.holder_pid = -1;
    mutex_user_input.isLocked = false;
    mutex_user_input.queue_size = 0;
    
    mutex_user_output.holder_pid = -1;
    mutex_user_output.isLocked = false;
    mutex_user_output.queue_size = 0;
    
    mutex_file.holder_pid = -1;
    mutex_file.isLocked = false;
    mutex_file.queue_size = 0;
}


Mutex* get_mutex(ResourceType resource_name) {
    switch(resource_name) { // * Added switch for clarity
        case userInput: return &mutex_user_input;
        case userOutput: return &mutex_user_output;
        case file: return &mutex_file;
        default: return NULL;
    }
}

void sem_wait(ResourceType resource, PCB* process) {
    if (!process) return;

    Mutex* m = get_mutex(resource);
    if (!m) return;
    char log[128];
    snprintf(log, sizeof(log), "PID %d: Requesting mutex %d", process->pid, resource);
    update_log_view(log);

    if (!m->isLocked) {
        m->holder_pid = process->pid;
        m->isLocked = true;
        snprintf(log, sizeof(log), "PID %d: Acquired mutex %d", process->pid, resource);
        update_log_view(log);
    } else {
        updatePCBState(process, BLOCKED); // Use PCB function for consistency
        if (m->queue_size < MAX_ACTIVE_PROCESSES) {
            m->blocked_queue[m->queue_size++] = process;
            snprintf(log, sizeof(log), "PID %d: Blocked on mutex %d", process->pid, resource);
            update_log_view(log);

            GtkWidget *main_window = get_main_window();
            if (main_window) {
                GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                                                          GTK_DIALOG_MODAL,
                                                          GTK_MESSAGE_INFO,
                                                          GTK_BUTTONS_OK,
                                                          "PID %d: Blocked on mutex %d", process->pid, resource);
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
            }
        }
        // If this is the running process, block it in the scheduler
        // if (getRunningProcess() == process) {
        //     blockCurrentProcess();
        // }
    }
}

void sem_signal(ResourceType resource, PCB* process) {
    if (!process) return;

    Mutex* m = get_mutex(resource);
    if (!m || m->holder_pid != process->pid) return;

    char log[128];
    snprintf(log, sizeof(log), "PID %d: Releasing mutex %d", process->pid, resource);
    update_log_view(log);

    m->isLocked = false;
    m->holder_pid = -1;

    if (m->queue_size > 0) {
        int highest_pri = -1;
        int selected_idx = -1;

        for (int i = 0; i < m->queue_size; i++) {
            if (m->blocked_queue[i] && m->blocked_queue[i]->priority > highest_pri) {
                highest_pri = m->blocked_queue[i]->priority;
                selected_idx = i;
            }
        }

        if (selected_idx != -1) {
            PCB* next = m->blocked_queue[selected_idx];
            updatePCBState(next, READY); // Use PCB function MIGHT BE RUNNING
            m->holder_pid = next->pid;
            m->isLocked = true;
            snprintf(log, sizeof(log), "PID %d: Unblocked for mutex %d", next->pid, resource);
            update_log_view(log);


            GtkWidget *main_window = get_main_window();
            if (main_window) {
                GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                                                          GTK_DIALOG_MODAL,
                                                          GTK_MESSAGE_INFO,
                                                          GTK_BUTTONS_OK,
                                                          "PID %d: Unblocked for mutex %d", next->pid, resource);
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
            }
            unblockProcess(next);


            for (int i = selected_idx; i < m->queue_size - 1; i++) {
                m->blocked_queue[i] = m->blocked_queue[i + 1];
            }
            m->queue_size--;
        }
    }
}

// void sem_wait(ResourceType resource, PCB* process) {
//     if (!process) {
//         update_log_view("sem_wait: Null process");
//         return;
//     }

//     char log[128];
//     snprintf(log, sizeof(log), "PID %d: sem_wait %d", process->pid, resource);
//     update_log_view(log);
    
//     Mutex* m = get_mutex(resource);
//     if (!m) {
//         snprintf(log, sizeof(log), "PID %d: Invalid mutex for resource %d", process->pid, resource);
//         update_log_view(log);
//         return;
//     }
    
//     if (!m->isLocked) {
//         m->holder_pid = process->pid;
//         m->isLocked = true;
//         snprintf(log, sizeof(log), "PID %d: Acquired mutex %d", process->pid, resource);
//         update_log_view(log);
//     } else {
//         updatePCBState(process, BLOCKED);
//         if (m->queue_size < MAX_ACTIVE_PROCESSES) {
//             m->blocked_queue[m->queue_size++] = process;
//             snprintf(log, sizeof(log), "PID %d: Blocked on mutex %d, queue_size=%d", 
//                      process->pid, resource, m->queue_size);
//             update_log_view(log);
//         } else {
//             snprintf(log, sizeof(log), "PID %d: Mutex %d queue full", process->pid, resource);
//             update_log_view(log);
//         }
//         if (getRunningProcess() == process) {
//             snprintf(log, sizeof(log), "PID %d: Blocking current process", process->pid);
//             update_log_view(log);
//             blockCurrentProcess();
//         }
//     }
// }

// void sem_signal(ResourceType resource, PCB* process) {
//     if (!process) {
//         update_log_view("sem_signal: Null process");
//         return;
//     }

//     char log[128];
//     snprintf(log, sizeof(log), "PID %d: sem_signal %d", process->pid, resource);
//     update_log_view(log);
    
//     Mutex* m = get_mutex(resource);
//     if (!m || m->holder_pid != process->pid) {
//         snprintf(log, sizeof(log), "PID %d: Invalid mutex or not holder for resource %d", 
//                  process->pid, resource);
//         update_log_view(log);
//         return;
//     }
    
//     m->isLocked = false;
//     m->holder_pid = -1;
//     snprintf(log, sizeof(log), "PID %d: Released mutex %d", process->pid, resource);
//     update_log_view(log);
    
//     if (m->queue_size > 0) {
//         int highest_pri = -1;
//         int selected_idx = -1;
        
//         for (int i = 0; i < m->queue_size; i++) {
//             if (m->blocked_queue[i] && m->blocked_queue[i]->priority > highest_pri) {
//                 highest_pri = m->blocked_queue[i]->priority;
//                 selected_idx = i;
//             }
//         }
        
//         if (selected_idx != -1) {
//             PCB* next = m->blocked_queue[selected_idx];
//             updatePCBState(next, READY);
//             m->holder_pid = next->pid;
//             m->isLocked = true;
//             snprintf(log, sizeof(log), "PID %d: Unblocked PID %d for mutex %d", 
//                      process->pid, next->pid, resource);
//             update_log_view(log);
//             unblockProcess(next);
            
//             for (int i = selected_idx; i < m->queue_size - 1; i++) {
//                 m->blocked_queue[i] = m->blocked_queue[i + 1];
//             }
//             m->queue_size--;
//         }
//     }
// }