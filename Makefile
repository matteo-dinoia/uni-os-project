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
bin/%.o: src/%.c | bin
	$(CCOMPILE) -c $< -o $@
bin/%: src/%.c $(REQUIRED_O)
	$(CCOMPILE) $(REQUIRED_O) $< -o $@ -lm


#GENERAL USE
recompile: clear all
clear:
	$(RM) -r bin
debug: all
	cd bin && gdb $(TARGET)
run: all
	cd bin && ./$(TARGET)
runf: all
	cd bin && ./$(TARGET) | tee ../output.log
crun: _clear-screen recompile _wait-input
	cd bin && ./$(TARGET); $(RM) -r bin
crunf: _clear-screen recompile _wait-input
	cd bin && ./$(TARGET) > ../output.log; $(RM) -r bin

#TOOLS
alives:
	ps -aux | grep -E "/master|/ship|/port|/weather" | grep -v -E "grep|rm" | cat;
killall-int:
	killall -s INT $(_PROCESSES) | cat
killall-kill:
	killall -s KILL $(_PROCESSES) | cat
count:
	printf "\nNumber of lines in project: "; cat src/*.c src/header/*.h Makefile | wc -l

#DEPENDENCIES
_clear-screen:
	clear
_wait-input:
	echo -e "\nPress any key to launch"; read -n 1
