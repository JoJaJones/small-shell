/************************************************************************************
 * This file defines functions related to parsing the command line input into a
 * command, its args, and options
 ***********************************************************************************/

#ifndef CS344_COMMANDPARSER_H
#define CS344_COMMANDPARSER_H

// flags for built in functionality
#define EXIT_FLAG 1
#define STATUS_FLAG 2
#define CD_FLAG 4

#define TRUE 1
#define FALSE 0
#define PID "pid"
#define PID_LEN "pidlen"

#define STATUS "SMALLSH_STATUS"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

// struct the hold the relevant command information
struct command{
    char **args;
    int numArgs;
    int isBgProcess;
    int hasOutfile;
    char *outfile;
    int hasInfile;
    char *infile;
};

struct command *parseInput(char *input, char **args, int isForeOnlyMode);
int stripWhiteSpace(char *input, char **args);
void parseAllArgs(char **args, struct command *cmd, int isForeOnlyMode);
char *parseArg(char *rawArg);
int isBuiltIn(char* command);
int isWhitespace(char c);
int countVars(char* rawArg);
char *expandVariables(char *dest, char *source);
void freeCommand(struct command *cmd);
void setNullRedirects(struct command *cmd);
void echoModifier(struct command *cmd);

#endif //CS344_COMMANDPARSER_H
