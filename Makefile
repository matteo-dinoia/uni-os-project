CC = gcc
CFLAGS = -g -O0 -std=c89 -pedantic
#
REQUIRED =  shared_mem.o message.o semaphore.o
PROCESSES = ship port
TARGET = master

default: crun

#COMPILING SPECIFIC FILES
ship: $(REQUIRED) ship.c
	$(CC) $(CFLAGS) $(REQUIRED) ship.c -o ship -lm

port: $(REQUIRED) port.c
	$(CC) $(CFLAGS) $(REQUIRED) port.c -o port

$(TARGET): $(PROCESSES) $(REQUIRED) $(TARGET).c
	$(CC) $(CFLAGS) $(REQUIRED) $(TARGET).c -o $(TARGET)

#GENERAL USE
recompile: clear all
all: $(TARGET)
crun: tool-clear recompile
	echo -e "\nPress any key to launch"; read  -n 1
	./$(TARGET); rm -f *.o $(TARGET) $(PROCESSES) *~
run: all
	./$(TARGET)
clear:
	rm -f *.o $(TARGET) $(PROCESSES) *~

#TOOLS
tool-alive:
	ps -aux | grep -E "/master|/ship|/port" | grep -v -E "grep|rm" | cat;
tool-killall:
	killall -s INT master port ship | cat
tool-clear:
	clear
tool-count:
	printf "\nNumber of lines in project: "; cat *.c *.h Makefile | wc -l
