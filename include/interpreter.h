#ifndef INTERPRETER_H
#define INTERPRETER_H
#include <gtk/gtk.h>
#include "pcb.h"
#include "Mutex.h"

#define MAX_LINE_LENGTH 100
#define MAX_ARG_LENGTH 50 // * Added for consistency

void set_main_window(GtkWidget *window);
GtkWidget* get_main_window(void);
void executeInstruction(PCB* pcb);
char** loadProgram(const char* filename, int* lineCount);
void handlePrint(PCB* pcb, const char* arg);
void handleAssign(PCB* pcb, const char* varName, const char* value);
void handleWriteFile(PCB* pcb, const char* filename, const char* data);
void handleReadFile(PCB* pcb, const char* filename);
void handlePrintFromTo(PCB* pcb, const char* start, const char* end);


ResourceType getResourceType(const char* resourceName);

#endif