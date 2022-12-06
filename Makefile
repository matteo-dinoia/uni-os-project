CC = gcc
CFLAGS = -g -O0 -std=c89 -pedantic
#
REQUIRED =  shared_mem.o message.o semaphore.o
PROCESSES = ship port
TARGET = master


ship: $(REQUIRED)
	$(CC) $(CFLAGS) $(REQUIRED) ship.c -o ship -lm

port: $(REQUIRED)
	$(CC) $(CFLAGS) $(REQUIRED) port.c -o port

$(TARGET): $(PROCESSES) $(REQUIRED)
	$(CC) $(CFLAGS) $(REQUIRED) $(TARGET).c -o $(TARGET)

#GENERAL USE
recompile: clear all
all: $(TARGET)
crun: clear run
run: all
	./$(TARGET)
clear:
	rm -f *.o $(TARGET) $(PROCESSES) *~
