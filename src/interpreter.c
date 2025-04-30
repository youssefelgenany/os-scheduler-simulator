#include "interpreter.h"
#include "pcb.h"
#include "memory.h"
#include "Mutex.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char linesArr[MAX_INSTRUCTIONS][MAX_LINE_LENGTH];
static char* linesPtr[MAX_INSTRUCTIONS];

// Global window pointer for dialogs
static GtkWidget *main_window = NULL;

void set_main_window(GtkWidget *window) {
    main_window = window;
}
GtkWidget* get_main_window(void) {
    return main_window;
}

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
    else if (strcmp(command, "writeFile") == 0 && numArgs >= 3) {
        handleWriteFile(pcb, arg1, arg2);
    }
    else if (strcmp(command, "readFile") == 0 && numArgs >= 2) {
        handleReadFile(pcb, arg1);
    }
    else if (strcmp(command, "printFromTo") == 0 && numArgs >= 3) {
        handlePrintFromTo(pcb, arg1, arg2);
    }
    else if (strcmp(command, "semWait") == 0 && numArgs >= 2) {
        ResourceType resource = getResourceType(arg1);
        if (resource != RESOURCE_INVALID) {
            sem_wait(resource, pcb);
            if (pcb->state == BLOCKED) {
                pcb->programCounter++;
                return;
            }
        }
    }
    else if (strcmp(command, "semSignal") == 0 && numArgs >= 2) {
        ResourceType resource = getResourceType(arg1);
        if (resource != RESOURCE_INVALID) {
            sem_signal(resource, pcb);
        }
    }
    else {
        char error[128];
        snprintf(error, sizeof(error), "PID %d: Unknown instruction - %s", pcb->pid, instruction);
        if (main_window) {
            GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                                                      GTK_DIALOG_MODAL,
                                                      GTK_MESSAGE_ERROR,
                                                      GTK_BUTTONS_OK,
                                                      "%s", error);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
        }
    }
    if (pcb->state == RUNNING) {
        pcb->programCounter++;
    }
}

void handlePrint(PCB* pcb, const char* arg) {
    if (!pcb || !arg) return;
    
    const char* value = readFromMemory(pcb->pid, arg);
    char output[128];
    snprintf(output, sizeof(output), "PID %d: %s", pcb->pid, value ? value : arg);
    if (main_window) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_INFO,
                                                  GTK_BUTTONS_OK,
                                                  "%s", output);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}

void handleAssign(PCB* pcb, const char* varName, const char* value) {
    if (!pcb || !varName || !value) return;
    
    if (strcmp(value, "input") == 0) {
        char input[MAX_ARG_LENGTH] = "";
        if (main_window) {
            GtkWidget *dialog = gtk_dialog_new_with_buttons("Input Value",
                                                           GTK_WINDOW(main_window),
                                                           GTK_DIALOG_MODAL,
                                                           "_OK", GTK_RESPONSE_OK,
                                                           "_Cancel", GTK_RESPONSE_CANCEL,
                                                           NULL);
            GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
            // Add label with PID and variable name
            GtkWidget *label = gtk_label_new(NULL);
            char label_text[128];
            snprintf(label_text, sizeof(label_text), "Enter value for PID %d, variable %s:", pcb->pid, varName);
            gtk_label_set_text(GTK_LABEL(label), label_text);
            gtk_box_pack_start(GTK_BOX(content_area), label, FALSE, FALSE, 5);
            GtkWidget *entry = gtk_entry_new();
            gtk_box_pack_start(GTK_BOX(content_area), entry, TRUE, TRUE, 5);
            gtk_widget_show_all(content_area);

            // Log waiting for input
            char log[128];
            snprintf(log, sizeof(log), "PID %d: Waiting for input to %s", pcb->pid, varName);
            update_log_view(log);

            if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
                strncpy(input, gtk_entry_get_text(GTK_ENTRY(entry)), MAX_ARG_LENGTH - 1);
                input[MAX_ARG_LENGTH - 1] = '\0';
                writeToMemory(pcb->pid, varName, input);
                // Log successful input
                snprintf(log, sizeof(log), "PID %d: Assigned input %s to %s", pcb->pid, input, varName);
                update_log_view(log);
            } else {
                // Log cancelled input
                snprintf(log, sizeof(log), "PID %d: Input cancelled for %s", pcb->pid, varName);
                update_log_view(log);
            }
            gtk_widget_destroy(dialog);
        }
    } else {
        writeToMemory(pcb->pid, varName, value);
        // Log direct assignment
        char log[128];
        snprintf(log, sizeof(log), "PID %d: Assigned %s to %s", pcb->pid, value, varName);
        update_log_view(log);
    }
}
void handleWriteFile(PCB* pcb, const char* filename, const char* data) {
    if (!pcb || !filename || !data) return;
    
    const char* actualData = readFromMemory(pcb->pid, data);
    if (!actualData) actualData = data;
    
    FILE* fp = fopen(filename, "w");
    if (fp) {
        fprintf(fp, "%s", actualData);
        fclose(fp);
    } else {
        char error[128];
        snprintf(error, sizeof(error), "PID %d: Failed to write to file %s", pcb->pid, filename);
        if (main_window) {
            GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                                                      GTK_DIALOG_MODAL,
                                                      GTK_MESSAGE_ERROR,
                                                      GTK_BUTTONS_OK,
                                                      "%s", error);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
        }
    }

}

void handleReadFile(PCB* pcb, const char* filename) {
    if (!pcb || !filename) return;
    

    FILE* fp = fopen(filename, "r");
    if (fp) {
        char content[MAX_ARG_LENGTH];
        if (fgets(content, sizeof(content), fp)) {
            content[strcspn(content, "\n")] = '\0';
            writeToMemory(pcb->pid, filename, content);
        }
        fclose(fp);
    } else {
        char error[128];
        snprintf(error, sizeof(error), "PID %d: Failed to read file %s", pcb->pid, filename);
        if (main_window) {
            GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                                                      GTK_DIALOG_MODAL,
                                                      GTK_MESSAGE_ERROR,
                                                      GTK_BUTTONS_OK,
                                                      "%s", error);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
        }
    }

}

void handlePrintFromTo(PCB* pcb, const char* start, const char* end) {
    if (!pcb || !start || !end) return;
    

    int x = atoi(start);
    int y = atoi(end);
    
    const char* x_val = readFromMemory(pcb->pid, start);
    const char* y_val = readFromMemory(pcb->pid, end);
    if (x_val) x = atoi(x_val);
    if (y_val) y = atoi(y_val);
    
    if (x > y) {
        char error[128];
        snprintf(error, sizeof(error), "PID %d: Invalid range %d to %d", pcb->pid, x, y);
        if (main_window) {
            GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                                                      GTK_DIALOG_MODAL,
                                                      GTK_MESSAGE_ERROR,
                                                      GTK_BUTTONS_OK,
                                                      "%s", error);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
        }
    } else {
        GString *output = g_string_new(NULL);
        g_string_append_printf(output, "PID %d: ", pcb->pid);
        for (int i = x; i <= y; i++) {
            g_string_append_printf(output, "%d ", i);
        }
        if (main_window) {
            GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                                                      GTK_DIALOG_MODAL,
                                                      GTK_MESSAGE_INFO,
                                                      GTK_BUTTONS_OK,
                                                      "%s", output->str);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
        }
        g_string_free(output, TRUE);
    }
}

ResourceType getResourceType(const char* resourceName) {
    if (!resourceName) return RESOURCE_INVALID;
    
    if (strcmp(resourceName, "userInput") == 0) return userInput;
    if (strcmp(resourceName, "userOutput") == 0) return userOutput;
    if (strcmp(resourceName, "file") == 0) return file;
    return RESOURCE_INVALID;
}

char** loadProgram(const char* filename, int* lineCount) {
    if (!filename || !lineCount) return NULL;
    FILE* fp = fopen(filename, "r");
    if (!fp) return NULL;

    *lineCount = 0;
    char buffer[MAX_LINE_LENGTH];
    while (*lineCount < MAX_INSTRUCTIONS && fgets(buffer, sizeof(buffer), fp)) {
        buffer[strcspn(buffer, "\n")] = '\0';
        strncpy(linesArr[*lineCount], buffer, MAX_LINE_LENGTH-1);
        linesArr[*lineCount][MAX_LINE_LENGTH-1] = '\0';
        linesPtr[*lineCount] = linesArr[*lineCount];
        (*lineCount)++;
    }
    fclose(fp);
    return linesPtr;
}