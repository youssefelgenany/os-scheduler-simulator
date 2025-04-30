#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"
#include "interpreter.h"
#include "memory.h"
#include "Mutex.h"
#include "pcb.h"

// Forward declaration
static gboolean simulation_step(gpointer data);

typedef struct {
    GtkWidget *window;
    GtkWidget *notebook;
    GtkWidget *start_button;
    GtkWidget *step_button;
    GtkWidget *reset_button;
    GtkWidget *add_process_button;
    GtkWidget *algo_combo;
    GtkWidget *quantum_entry;
    GtkWidget *quantum_label;
    GtkWidget *status_label;
    GtkWidget *process_view;
    GtkWidget *queue_view;
    GtkWidget *mutex_view;
    GtkWidget *memory_view;
    GtkWidget *log_view;
    int clock_cycle;
    gboolean running;
    PCB *processes[MAX_PROCESSES];
    int process_count;
    guint timeout_id;
} AppData;

static AppData *app = NULL;

static void update_process_view(void) {
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(app->process_view)));
    gtk_list_store_clear(store);
    GtkTreeIter iter;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (app->processes[i]) {
            const char *state_str = app->processes[i]->state == READY ? "Ready" :
                                    app->processes[i]->state == RUNNING ? "Running" :
                                    app->processes[i]->state == BLOCKED ? "Blocked" :
                                    app->processes[i]->state == TERMINATED ? "Terminated" : "Unknown";
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                               0, app->processes[i]->pid,
                               1, state_str,
                               2, app->processes[i]->priority,
                               3, app->processes[i]->programCounter,
                               4, app->processes[i]->memoryLowerBound,
                               5, app->processes[i]->memoryUpperBound,
                               -1);
        }
    }
}

static void update_queue_view(void) {
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(app->queue_view)));
    gtk_list_store_clear(store);
    GtkTreeIter iter;
    // Ready Queues
    for (int level = 0; level < MAX_PRIORITY_LEVELS; level++) {
        for (int i = 0; i < systemScheduler.readyCounts[level]; i++) {
            PCB *pcb = systemScheduler.readyQueues[level][i];
            if (pcb) {
                char instruction[64] = "";
                if (pcb->programCounter < pcb->instructionCount && pcb->instructions[pcb->programCounter]) {
                    strncpy(instruction, pcb->instructions[pcb->programCounter], sizeof(instruction)-1);
                    instruction[sizeof(instruction)-1] = '\0';
                }
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter,
                                   0, pcb->pid,
                                   1, "Ready",
                                   2, level + 1,
                                   3, instruction,
                                   -1);
            }
        }
    }
    // Running Process
    PCB *running = getRunningProcess();
    if (running) {
        char instruction[64] = "";
        if (running->programCounter < running->instructionCount && running->instructions[running->programCounter]) {
            strncpy(instruction, running->instructions[running->programCounter], sizeof(instruction)-1);
            instruction[sizeof(instruction)-1] = '\0';
        }
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, running->pid,
                           1, "Running",
                           2, running->priority,
                           3, instruction,
                           -1);
    }
    // Blocked Queue
    for (int i = 0; i < systemScheduler.blockedCount; i++) {
        PCB *pcb = systemScheduler.blockedQueue[i];
        if (pcb) {
            char instruction[64] = "";
            if (pcb->programCounter < pcb->instructionCount && pcb->instructions[pcb->programCounter]) {
                strncpy(instruction, pcb->instructions[pcb->programCounter], sizeof(instruction)-1);
                instruction[sizeof(instruction)-1] = '\0';
            }
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                               0, pcb->pid,
                               1, "Blocked",
                               2, pcb->priority,
                               3, instruction,
                                   -1);
        }
    }
}

static void update_mutex_view(void) {
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(app->mutex_view)));
    gtk_list_store_clear(store);
    GtkTreeIter iter;
    Mutex *mutexes[] = {&mutex_user_input, &mutex_user_output, &mutex_file};
    const char *names[] = {"userInput", "userOutput", "file"};
    for (int i = 0; i < 3; i++) {
        char blocked[128] = "";
        for (int j = 0; j < mutexes[i]->queue_size; j++) {
            if (mutexes[i]->blocked_queue[j]) {
                char pid[16];
                snprintf(pid, sizeof(pid), "%d ", mutexes[i]->blocked_queue[j]->pid);
                strncat(blocked, pid, sizeof(blocked) - strlen(blocked) - 1);
            }
        }
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, names[i],
                           1, mutexes[i]->isLocked ? "Locked" : "Unlocked",
                           2, mutexes[i]->holder_pid,
                           3, blocked,
                           -1);
    }
}

static void update_memory_view(void) {
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(app->memory_view)));
    gtk_list_store_clear(store);
    GtkTreeIter iter;
    for (int i = 0; i < TOTAL_MEMORY_WORDS; i++) {
        if (memory[i].owner_pid != -1) {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                               0, i,
                               1, memory[i].owner_pid,
                               2, memory[i].name,
                               3, memory[i].value,
                               -1);
        }
    }
}

void update_log_view(const char *message) {
    if (!app || !app->log_view) {
        printf("Error: Log view not initialized (%s)\n", message);
        return;
    }
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_view));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, message, -1);
    gtk_text_buffer_insert(buffer, &end, "\n", -1);
    // // Auto-scroll to bottom
    // GtkTextMark *mark = gtk_text_buffer_create_mark(buffer, NULL, &end, FALSE);
    // gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(app->log_view), mark, 0.0, FALSE, 0.0, 0.0);
    // gtk_text_buffer_delete_mark(buffer, mark);
    // gtk_widget_queue_draw(app->log_view);
}

static void update_status_label(void) {
    char status[128];
    snprintf(status, sizeof(status), "Cycle: %d | Processes: %d | Algorithm: %s | Quantum: %d",
             app->clock_cycle, app->process_count,
             systemScheduler.algorithm == FCFS ? "FCFS" :
             systemScheduler.algorithm == ROUND_ROBIN ? "Round Robin" :
             "Multilevel Feedback",
             systemScheduler.quantum);
    gtk_label_set_text(GTK_LABEL(app->status_label), status);
}

// In main.c (~line 190)
static void on_algo_changed(GtkComboBox *combo, gpointer data) {
    gint active = gtk_combo_box_get_active(combo);
    SchedulingAlgorithm algo = FCFS;
    switch (active) {
        case 0: algo = FCFS; break;
        case 1: algo = ROUND_ROBIN; break;
        case 2: algo = MULTILEVEL_FEEDBACK; break;
    }

    // Show quantum entry only for Round Robin
    gtk_widget_set_visible(app->quantum_entry, algo == ROUND_ROBIN);
    gtk_widget_set_visible(app->quantum_label, algo == ROUND_ROBIN); // Use stored label

    int quantum = 1; // Default quantum
    if (algo == ROUND_ROBIN) {
        const char *quantum_text = gtk_entry_get_text(GTK_ENTRY(app->quantum_entry));
        quantum = atoi(quantum_text) > 0 ? atoi(quantum_text) : 1;
    }
    initScheduler(algo, quantum);
    char log[64];
    snprintf(log, sizeof(log), "Algorithm set to %s, Quantum: %d", 
             algo == FCFS ? "FCFS" : algo == ROUND_ROBIN ? "Round Robin" : "Multilevel Feedback", 
             quantum);
    update_log_view(log);
    update_status_label();
}

static void on_quantum_changed(GtkEditable *editable, gpointer data) {
    if (systemScheduler.algorithm != ROUND_ROBIN) return;
    int quantum = atoi(gtk_entry_get_text(GTK_ENTRY(editable)));
    if (quantum > 0) {
        setQuantum(quantum);
        char log[64];
        snprintf(log, sizeof(log), "Quantum set to %d", quantum);
        update_log_view(log);
        update_status_label();
    }
}
// static void on_start_clicked(GtkButton *button, gpointer data) {
//     if (!app->running) {
//         app->running = TRUE;
//         gtk_button_set_label(button, "Pause");
//         app->timeout_id = g_timeout_add(1000, (GSourceFunc)simulation_step, NULL);
//     } else {
//         app->running = FALSE;
//         gtk_button_set_label(button, "Resume");
//         g_source_remove(app->timeout_id);
//     }
// }

static void on_start_clicked(GtkButton *button, gpointer data) {
    if (!app->running) {
        app->running = TRUE;
        gtk_button_set_label(button, "Pause");
        // Pass non-NULL data to indicate continuous running
        app->timeout_id = g_timeout_add(1000, (GSourceFunc)simulation_step, (gpointer)1);
    } else {
        app->running = FALSE;
        gtk_button_set_label(button, "Resume");
        if (app->timeout_id) {
            g_source_remove(app->timeout_id);
            app->timeout_id = 0;
        }
    }
}

// In main.c
static void on_step_clicked(GtkButton *button, gpointer data) {
    if (app->process_count == 0) {
        update_log_view("No processes to execute");
        return;
    }

    // Stop continuous running if active
    if (app->running) {
        app->running = FALSE;
        gtk_button_set_label(GTK_BUTTON(app->start_button), "Start");
        if (app->timeout_id) {
            g_source_remove(app->timeout_id);
            app->timeout_id = 0;
        }
    }

    // Disable button during execution
    gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);

    // Execute one cycle
    simulation_step(NULL);

    // Update GUI
    update_process_view();
    update_queue_view();
    update_mutex_view();
    update_memory_view();
    update_status_label();
    while (gtk_events_pending()) {
        gtk_main_iteration(); // Ensure GUI updates
    }
    // Re-enable button
    gtk_widget_set_sensitive(GTK_WIDGET(button), TRUE);
}

static void on_reset_clicked(GtkButton *button, gpointer data) {
    app->running = FALSE;
    app->clock_cycle = 0;
    app->process_count = 0;
    gtk_button_set_label(GTK_BUTTON(app->start_button), "Start");
    if (app->timeout_id) g_source_remove(app->timeout_id);
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (app->processes[i]) {
            freeProcessMemory(app->processes[i]->pid);
            cleanupPCB(app->processes[i]);
            app->processes[i] = NULL;
        }
    }
    cleanupScheduler();
    initMemory();
    init_mutexes();
    initPCBs();
    initScheduler(FCFS, 1);
    gtk_combo_box_set_active(GTK_COMBO_BOX(app->algo_combo), 0);
    gtk_entry_set_text(GTK_ENTRY(app->quantum_entry), "1");
    update_process_view();
    update_queue_view();
    update_mutex_view();
    update_memory_view();
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_view)), "", -1);
    update_status_label();
    update_log_view("System reset.");
}

static void on_add_process_clicked(GtkButton *button, gpointer data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Select Program File",
                                                    GTK_WINDOW(app->window),
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Open", GTK_RESPONSE_ACCEPT,
                                                    NULL);
    
    // Set the default folder to the programs directory
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), "./programs");

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (!filename) {
            update_log_view("Invalid filename selected.");
            gtk_widget_destroy(dialog);
            return;
        }

        GtkWidget *arrival_dialog = gtk_dialog_new_with_buttons("Set Arrival Time",
                                                               GTK_WINDOW(app->window),
                                                               GTK_DIALOG_MODAL,
                                                               "_OK", GTK_RESPONSE_OK,
                                                               "_Cancel", GTK_RESPONSE_CANCEL,
                                                               NULL);
        GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(arrival_dialog));
        GtkWidget *entry = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(entry), "0");
        gtk_box_pack_start(GTK_BOX(content_area), entry, TRUE, TRUE, 5);
        gtk_widget_show_all(arrival_dialog);

        response = gtk_dialog_run(GTK_DIALOG(arrival_dialog));
        if (response == GTK_RESPONSE_OK) {
            const char *arrival_text = gtk_entry_get_text(GTK_ENTRY(entry));
            int arrival_time = arrival_text ? atoi(arrival_text) : 0;

            if (app->process_count < MAX_PROCESSES) {
                int lineCount = 0;
                char **lines = loadProgram(filename, &lineCount);
                if (lines && lineCount > 0) {
                    PCB *pcb = createPCB(arrival_time);
                    if (pcb) {
                        for (int i = 0; i < lineCount && i < MAX_INSTRUCTIONS; i++) {
                            if (lines[i]) {
                                addInstruction(pcb, lines[i]);
                            }
                        }

                        if (allocateProcessMemory(pcb)) {
                            app->processes[app->process_count++] = pcb;
                            addProcess(pcb);
                            char log[128];
                            snprintf(log, sizeof(log), "Added process PID %d from %s (Arrival: %d)", 
                                     pcb->pid, filename, arrival_time);
                            update_log_view(log);
                        } else {
                            cleanupPCB(pcb);
                            update_log_view("Failed to allocate memory for process.");
                        }
                    } else {
                        update_log_view("Failed to create PCB.");
                    }
                } else {
                    update_log_view("Failed to load program file or file is empty.");
                }
            } else {
                update_log_view("Maximum process limit reached.");
            }
        }
        gtk_widget_destroy(arrival_dialog);
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
    update_process_view();
    update_queue_view();
}

// static gboolean simulation_step(gpointer data) {
//     if (!app->running) return FALSE;

//     // Execute one cycle
//     scheduleAndExecute(app->clock_cycle);
//     app->clock_cycle++;

//     // Update GUI
//     update_process_view();
//     update_queue_view();
//     update_mutex_view();
//     update_memory_view();
//     update_status_label();

//     // Log running process instruction
//     PCB *running = getRunningProcess();
//     if (running && running->programCounter < running->instructionCount && running->instructions[running->programCounter]) {
//         char log[128];
//         snprintf(log, sizeof(log), "PID %d executing: %s", running->pid, running->instructions[running->programCounter]);
//         update_log_view(log);
//     }

//     // Check if simulation should continue
//     int all_done = 1;
//     int has_blocked = systemScheduler.blockedCount > 0;
//     int has_future_arrivals = 0;

//     for (int i = 0; i < MAX_PROCESSES; i++) {
//         if (app->processes[i]) {
//             if (app->processes[i]->state != TERMINATED) {
//                 all_done = 0;
//                 if (app->processes[i]->arrivalTime > app->clock_cycle) {
//                     has_future_arrivals = 1;
//                 }
//             }
//         }
//     }

//     if (all_done && app->process_count > 0) {
//         app->running = FALSE;
//         gtk_button_set_label(GTK_BUTTON(app->start_button), "Start");
//         if (app->timeout_id) g_source_remove(app->timeout_id);
//         update_log_view("All processes completed.");
//         return FALSE;
//     } else if (!getRunningProcess() && !has_blocked && !has_future_arrivals && app->process_count > 0) {
//         app->running = FALSE;
//         gtk_button_set_label(GTK_BUTTON(app->start_button), "Start");
//         if (app->timeout_id) g_source_remove(app->timeout_id);
//         update_log_view("No runnable processes and no blocked or future arrivals.");
//         return FALSE;
//     }

//     return TRUE;
// }

static gboolean simulation_step(gpointer data) {
    // Execute one cycle
    scheduleAndExecute(app->clock_cycle);
    app->clock_cycle++;

    // Update GUI
    update_process_view();
    update_queue_view();
    update_mutex_view();
    update_memory_view();
    update_status_label();

    // Log running process instruction
    PCB *running = getRunningProcess();
    if (running && running->programCounter < running->instructionCount && 
        running->instructions[running->programCounter]) {
        char log[128];
        snprintf(log, sizeof(log), "PID %d executing: %s", 
                running->pid, running->instructions[running->programCounter]);
        update_log_view(log);
    }
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }
    // For step mode (button click), return FALSE to run just once
    if (data == NULL) {
        return FALSE;
    }
    
    // For continuous running (start button), check if simulation should continue
    int all_done = 1;
    int has_blocked = systemScheduler.blockedCount > 0;
    int has_future_arrivals = 0;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (app->processes[i]) {
            if (app->processes[i]->state != TERMINATED) {
                all_done = 0;
                if (app->processes[i]->arrivalTime > app->clock_cycle) {
                    has_future_arrivals = 1;
                }
            }
        }
    }

    if (all_done && app->process_count > 0) {
        app->running = FALSE;
        gtk_button_set_label(GTK_BUTTON(app->start_button), "Start");
        if (app->timeout_id) g_source_remove(app->timeout_id);
        update_log_view("All processes completed.");
        return FALSE;
    } else if (!getRunningProcess() && !has_blocked && !has_future_arrivals && app->process_count > 0) {
        app->running = FALSE;
        gtk_button_set_label(GTK_BUTTON(app->start_button), "Start");
        if (app->timeout_id) g_source_remove(app->timeout_id);
        update_log_view("No runnable processes and no blocked or future arrivals.");
        return FALSE;
    }

    return TRUE;
}

static void create_gui(void) {
    app = g_new0(AppData, 1);
    app->clock_cycle = 0;
    app->running = FALSE;
    app->process_count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) app->processes[i] = NULL;

    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), "OS Simulator");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 1000, 600);
    g_signal_connect(app->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(app->window), vbox);

    // Status Label
    app->status_label = gtk_label_new("Cycle: 0 | Processes: 0 | Algorithm: FCFS | Quantum: 1");
    gtk_box_pack_start(GTK_BOX(vbox), app->status_label, FALSE, FALSE, 5);

    // Control Panel
    GtkWidget *control_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), control_box, FALSE, FALSE, 5);

    app->algo_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->algo_combo), "FCFS");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->algo_combo), "Round Robin");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->algo_combo), "Multilevel Feedback");
    gtk_combo_box_set_active(GTK_COMBO_BOX(app->algo_combo), 0);
    g_signal_connect(app->algo_combo, "changed", G_CALLBACK(on_algo_changed), NULL);
    gtk_box_pack_start(GTK_BOX(control_box), app->algo_combo, FALSE, FALSE, 5);

    // Quantum Label and Entry
    app->quantum_label = gtk_label_new("Quantum:");
    gtk_widget_set_name(app->quantum_label, "quantum_label");
    gtk_box_pack_start(GTK_BOX(control_box), app->quantum_label, FALSE, FALSE, 5);
    app->quantum_entry = gtk_entry_new();
    gtk_widget_set_name(app->quantum_entry, "quantum_entry");
    gtk_entry_set_text(GTK_ENTRY(app->quantum_entry), "1");
    gtk_widget_set_visible(app->quantum_entry, FALSE); // Initially hidden
    gtk_widget_set_visible(app->quantum_label, FALSE); // Initially hidden
    g_signal_connect(app->quantum_entry, "changed", G_CALLBACK(on_quantum_changed), NULL);
    gtk_box_pack_start(GTK_BOX(control_box), app->quantum_entry, FALSE, FALSE, 5);

    app->start_button = gtk_button_new_with_label("Start");
    g_signal_connect(app->start_button, "clicked", G_CALLBACK(on_start_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(control_box), app->start_button, FALSE, FALSE, 5);

    app->step_button = gtk_button_new_with_label("Step");
    g_signal_connect(app->step_button, "clicked", G_CALLBACK(on_step_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(control_box), app->step_button, FALSE, FALSE, 5);

    app->reset_button = gtk_button_new_with_label("Reset");
    g_signal_connect(app->reset_button, "clicked", G_CALLBACK(on_reset_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(control_box), app->reset_button, FALSE, FALSE, 5);

    app->add_process_button = gtk_button_new_with_label("Add Process");
    g_signal_connect(app->add_process_button, "clicked", G_CALLBACK(on_add_process_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(control_box), app->add_process_button, FALSE, FALSE, 5);
    // Notebook for Tabs
    app->notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(vbox), app->notebook, TRUE, TRUE, 5);

    // Process View
    app->process_view = gtk_tree_view_new();
    GtkListStore *process_store = gtk_list_store_new(6, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT);
    gtk_tree_view_set_model(GTK_TREE_VIEW(app->process_view), GTK_TREE_MODEL(process_store));
    const char *process_columns[] = {"PID", "State", "Priority", "PC", "Lower Bound", "Upper Bound"};
    for (int i = 0; i < 6; i++) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(process_columns[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(app->process_view), column);
    }
    GtkWidget *process_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(process_scroll), app->process_view);
    gtk_notebook_append_page(GTK_NOTEBOOK(app->notebook), process_scroll, gtk_label_new("Processes"));

    // Queue View
    app->queue_view = gtk_tree_view_new();
    GtkListStore *queue_store = gtk_list_store_new(4, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(app->queue_view), GTK_TREE_MODEL(queue_store));
    const char *queue_columns[] = {"PID", "Queue", "Priority", "Current Instruction"};
    for (int i = 0; i < 4; i++) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(queue_columns[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(app->queue_view), column);
    }
    GtkWidget *queue_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(queue_scroll), app->queue_view);
    gtk_notebook_append_page(GTK_NOTEBOOK(app->notebook), queue_scroll, gtk_label_new("Queues"));

    // Mutex View
    app->mutex_view = gtk_tree_view_new();
    GtkListStore *mutex_store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(app->mutex_view), GTK_TREE_MODEL(mutex_store));
    const char *mutex_columns[] = {"Resource", "Status", "Holder PID", "Blocked PIDs"};
    for (int i = 0; i < 4; i++) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(mutex_columns[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(app->mutex_view), column);
    }
    GtkWidget *mutex_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(mutex_scroll), app->mutex_view);
    gtk_notebook_append_page(GTK_NOTEBOOK(app->notebook), mutex_scroll, gtk_label_new("Mutexes"));

    // Memory View
    app->memory_view = gtk_tree_view_new();
    GtkListStore *memory_store = gtk_list_store_new(4, G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(app->memory_view), GTK_TREE_MODEL(memory_store));
    const char *memory_columns[] = {"Address", "Owner PID", "Name", "Value"};
    for (int i = 0; i < 4; i++) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(memory_columns[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(app->memory_view), column);
    }
    GtkWidget *memory_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(memory_scroll), app->memory_view);
    gtk_notebook_append_page(GTK_NOTEBOOK(app->notebook), memory_scroll, gtk_label_new("Memory"));

    // Log View
    app->log_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app->log_view), FALSE);
    GtkWidget *log_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(log_scroll), app->log_view);
    gtk_notebook_append_page(GTK_NOTEBOOK(app->notebook), log_scroll, gtk_label_new("Log"));

    // Apply CSS
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "style.css", NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                             GTK_STYLE_PROVIDER(provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    // Set main window for interpreter
    set_main_window(app->window);

    gtk_widget_show_all(app->window);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    initMemory();
    init_mutexes();
    initPCBs();
    initScheduler(FCFS, 1);

    create_gui();
    update_log_view("System initialized.");

    gtk_main();

    // Cleanup
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (app->processes[i]) {
            freeProcessMemory(app->processes[i]->pid);
            cleanupPCB(app->processes[i]);
        }
    }
    cleanupScheduler();
    g_free(app);

    return 0;
}