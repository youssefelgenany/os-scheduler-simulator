CC = gcc
CFLAGS = -Wall -g -Iinclude `pkg-config --cflags gtk+-3.0`
LIBS = `pkg-config --libs gtk+-3.0`
SRC = src/main.c src/interpreter.c src/scheduler.c src/memory.c src/Mutex.c src/pcb.c
OBJ = $(SRC:.c=.o)
EXEC = os-project

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)

.PHONY: all clean