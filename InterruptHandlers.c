#include "InterruptHandlers.h"

volatile sig_atomic_t toggleFgMode = 0;

/*************************************************************************************
 * This function prints the handler configuration based on the passed integer mask
 ************************************************************************************/
void printHandlers(int processMask) {
    if (PARENT & processMask){
        printf("Parent ");
    }

    if (CHILD & processMask){
        printf("Child ");
    }

    if (FOREGROUND & processMask) {
        printf("Foreground ");
    }

    if (FOREGROUND_ONLY & processMask) {
        printf("Only ");
    }

    if (BACKGROUND & processMask) {
        printf("Background ");
    }

    printf("Handlers Loading on %d...\n", getpid());
    fflush(stdout);
}

/*************************************************************************************
 * This function will set the signal handlers for SIGINT and SIGTSTP based on the
 * process state as communicated by the processMask integer value
 *
 * @param processMask: integer value containing bits set to indicate which
 * combination of states the current process is in
 ************************************************************************************/
void loadHandlers(int processMask) {
    // create struct for SIGINT handler
    struct sigaction SIGINT_action = {0};
    sigaddset(&SIGINT_action.sa_mask, SIGINT);

    // creat struct for SIGTSTP handler
    struct sigaction SIGTSTP_action = {0};
    sigaddset(&SIGTSTP_action.sa_mask, SIGTSTP);

    // set the appropriate handlers based on combination of states indicated by processMask
    if (processMask & BACKGROUND) {
        SIGINT_action.sa_handler = OBLIVIOUS;
        SIGTSTP_action.sa_handler = OBLIVIOUS;
    } else if (processMask & PARENT){
        SIGINT_action.sa_handler = OBLIVIOUS;
        SIGTSTP_action.sa_handler = toggleFgOnlyMode;
    } else if (processMask & FOREGROUND) {
        SIGINT_action.sa_handler = FILICIDE;
        SIGTSTP_action.sa_handler = OBLIVIOUS;
    }

    // load the custom handlers
    sigaction(SIGINT, &SIGINT_action, NULL);
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
}

/*************************************************************************************
 * The function handles SIGTSTP interrupt signals to set the toggle flag for FG only
 * mode
 *
 * @param signum: an integer indicating the signal that the process received
 ************************************************************************************/
void toggleFgOnlyMode(int signum) {
    toggleFgMode = 1;
}

/*************************************************************************************
 * Function to apply the toggle for foreground only mode the next time the parent
 * process is in control of the ui
 *
 * @param isForeOnly: pointer to data containing foreground only flag
 ************************************************************************************/
void applyFgOnlyToggle(int *isForeOnly) {
    // toggle the flag
    (*isForeOnly) ^= FOREGROUND_ONLY;

    // clear the toggle global mode variable
    toggleFgMode = 0;

    // print the mode change message
    printForeGroundMsg(*isForeOnly);
}

/*************************************************************************************
 * Function to print the message for mode changes
 *
 * @param isFgOnly: flag containing 0 if not foreground only and non-zero if is fg-only
 ************************************************************************************/
void printForeGroundMsg(int isFgOnly) {
    if (isFgOnly){
        printf("\033[93mEntering foreground-only mode (& is now ignored)\033[96m\n");
        fflush(stdout);
    } else {
        printf("\033[93mExiting foreground-only mode\033[96m\n");
        fflush(stdout);
    }
}


