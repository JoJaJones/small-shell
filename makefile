# variable for output file name
FILENAME = smallsh

# source files
OBJS = main.o InterruptHandlers.o CommandParser.o CommandDelegator.o
SRCS = main.c InterruptHandlers.c CommandParser.c CommandDelegator.c
HEADERS = InterruptHandlers.h CommandParser.h CommandDelegator.h
PLAN = README.txt

# compiler variables
CC = gcc
CFLAGS = -std=gnu99

# c++ compilation configurations
CXX = g++
# CXXFLAGS = -std=c++0x
CXXFLAGS = -std=c++11
CXXFLAGS += -Wall
CXXFLAGS += -pedantic-errors

DEBUG = -g
OPTIMIZE = -03

LDFLAGS =

# leak check variables
LEAK = valgrind
FULL = --leak-check=full

# basic compilation
${FILENAME}: ${OBJS} ${HEADERS}
	${CC} ${LDFLAGS} ${OBJS} -o ${FILENAME}

${OBJS}: ${SRCS}
	${CC} ${CFLAGS} -c ${@:.o=.c}

# leak checks
leak:
	${LEAK} ./${FILENAME}

full:
	${LEAK} ${FULL} ./${FILENAME}

# clean
clean:
	rm *.o ${FILENAME}

# zip
zip:
	zip jonesjo7_program3.zip makefile ${SRCS} ${HEADERS} ${PLAN}

# run
run:
	./${FILENAME}

pull:
	git pull origin assignment-3-dev
