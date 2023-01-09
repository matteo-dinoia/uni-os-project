CC=gcc
CFLAGS=-g -O0 -std=c89 -pedantic
CCOMPILE=$(CC) $(CFLAGS)
.PRECIOUS: bin/%.o
#
_REQUIRED=shared_mem message semaphore utils
REQUIRED_O=$(patsubst %, bin/%.o,$(_REQUIRED))
TARGET=master
_PROCESSES=ship port weather $(TARGET)
PROCESSES_OUT=$(addprefix bin/,$(_PROCESSES))

all: $(PROCESSES_OUT)

#COMPILING SPECIFIC FILES
bin:
	mkdir -p $@
bin/%.o: src/%.c src/header/%.h Makefile| bin
	$(CCOMPILE) -c $< -o $@
bin/ipc_manager.o: src/ipc_manager.c src/header/ipc_manager.h Makefile| bin
	$(CCOMPILE) -c $< -o $@
bin/%: src/%.c $(REQUIRED_O) bin/ipc_manager.o Makefile
	$(CCOMPILE) $(REQUIRED_O) bin/ipc_manager.o $< -o $@ -lm


#GENERAL USE
recompile: clear all
clear:
	$(RM) -r bin
debug: all
	cd bin && gdb $(TARGET)
run: all
	cd bin && ./$(TARGET)
#truncate output when ^c is pressed (bug)
run-and-log: all
	cd bin && ./$(TARGET) | tee ../output.log; echo -e "\n\nPossible cut output because of make limations (use run instead)"

#TOOLS
alives:
	ps -aux | grep -E "/master|/ship|/port|/weather" | grep -v -E "grep|rm" | cat;
killall-int:
	killall -s INT $(_PROCESSES) | cat
killall-kill:
	killall -s KILL $(_PROCESSES) | cat
count:
	printf "\nNumber of lines in project: "; wc -l src/*.c src/header/*.h Makefile