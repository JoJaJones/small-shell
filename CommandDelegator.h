/************************************************************************************
 * The file defines functions related to delegating tasks necessary to the operation
 * of the shell
 ***********************************************************************************/
#ifndef CS344_COMMANDDELGATOR_H
#define CS344_COMMANDDELGATOR_H

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "CommandParser.h"
#include "InterruptHandlers.h"  // circular dependency issue

// flags for processes in the shell
#define CHILD 1
#define PARENT 2
#define FOREGROUND 4
#define BACKGROUND 8
#define FOREGROUND_ONLY 16

extern volatile sig_atomic_t toggleFgMode;

// structure to create a node for a linked list of still active child processes
struct processNode {
    pid_t pid;
    struct processNode *next;
};

// structure to create a linked list of still active child processes
struct processLinkedList {
    struct processNode *head;
    struct processNode *tail;
};

struct forkResult {
    int pid;
    int isForeOnly;
};

int clearFinished(pid_t targetProcess, int hideStatus);
void nonBlockClearFinished(struct processLinkedList *processList);
void addProcess(struct processLinkedList *procList, int pid);
int removeProcess(struct processLinkedList *procList, int pid);
int openRedirFiles(struct command *cmd);
int openRedirFile(struct command *cmd, int isOutput);
void cd(char **args, int numArgs);
void showStatus();
void exitProgram(struct processLinkedList *processList, struct command *cmd);
struct forkResult *forkForeground(struct command *cmd, struct processLinkedList *procList, int isForeOnlyMode);
struct forkResult * forkBackground(struct command *cmd, struct processLinkedList *processList); // todo add status, change to processLinkedList, remove cur

void printProcList(struct processLinkedList* procList);
void freeProcessList(struct processLinkedList *procList);

void printPrompt();

#endif //CS344_COMMANDDELGATOR_H
