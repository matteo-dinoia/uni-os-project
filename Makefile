CC=gcc
CFLAGS=-g -O0 -std=c89 -pedantic
CCOMPILE=$(CC) $(CFLAGS)
#
REQUIRED=shared_mem.o message.o semaphore.o utils.o
PROCESSES=ship port meteo
TARGET=master

all: $(PROCESSES) $(TARGET)

#COMPILING SPECIFIC FILES
%.o: %.c
	$(CCOMPILE) -c $< -o $@
%: %.c $(REQUIRED)
	$(CCOMPILE) $(REQUIRED) $< -o $@ -lm

#GENERAL USE
recompile: clear all
run: all
	./$(TARGET)
clear:
	$(RM) *.o $(TARGET) $(PROCESSES) *~

crun: tool-clear recompile
	echo -e "\nPress any key to launch"; read -n 1
	./$(TARGET); rm -f *.o $(TARGET) $(PROCESSES) *~
crunf: tool-clear recompile
	echo -e "\nPress any key to launch"; read -n 1
	./$(TARGET) > output.log; rm -f *.o $(TARGET) $(PROCESSES) *~

#TOOLS
tool-alive:
	ps -aux | grep -E "/master|/ship|/port" | grep -v -E "grep|rm" | cat;
tool-killall:
	killall -s INT master port ship | cat
tool-clear:
	clear
tool-count:
	printf "\nNumber of lines in project: "; cat *.c *.h Makefile | wc -l
