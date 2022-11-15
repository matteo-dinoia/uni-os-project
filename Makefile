CC = gcc
CFLAGS= -std=c89 -pedantic
#
TARGET= master


#GENERAL USE
all: $(TARGET)
clear:
	rm -f *.o $(TARGET) *~
