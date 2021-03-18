/*************************************************************************************
 * This file defines functions related to installing custom interrupt handlers and
 * associated utilities
 ************************************************************************************/
#ifndef CS344_INTERRUPTHANDLERS_H
#define CS344_INTERRUPTHANDLERS_H

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>  // debug purposes for now
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "CommandParser.h"

#define CHILD 1
#define PARENT 2
#define FOREGROUND 4
#define BACKGROUND 8
#define FOREGROUND_ONLY 16

#define BASIC_STATUS "exit value 0"
#define FILICIDE SIG_DFL
#define OBLIVIOUS SIG_IGN

extern volatile sig_atomic_t toggleFgMode;

void printForeGroundMsg(int isFgOnly);
void applyFgOnlyToggle(int *isForeOnly);
void toggleFgOnlyMode(int signum);
void endForegroundOnlyMode(int signum);

void loadHandlers(int processMask);

void printHandlers(int processMask);

#endif //CS344_INTERRUPTHANDLERS_H
