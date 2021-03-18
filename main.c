#include "InterruptHandlers.h"
#include "CommandParser.h"
#include "CommandDelegator.h"

extern volatile sig_atomic_t toggleFgMode;

void startShell();
void initParentProc(pid_t pid);

int main() {
    // perform initialisation required for parent process
    initParentProc(getpid());

    // start the shell
    startShell();

    return 0;
}

/************************************************************************************
 * Function to present and interpret the main ui for the smallsh program.
 ***********************************************************************************/
void startShell() {
    int isForeOnlyMode = 0;
    int run = 1;  // var to use to generate infinite loop

    // variables to store command line raw input and arguments
    char *input;
    char *args[512];

    // create the linked list to hold outstanding child processes
    struct processLinkedList *procList = calloc(1, sizeof(struct processLinkedList));

    printPrompt();

    while (run) {
        // check if FG-only mode should be toggled and toggle it if so
        if (toggleFgMode) {
            applyFgOnlyToggle(&isForeOnlyMode);
            printPrompt();
        }

        // allocate memory for input and initialize args to null pointers
        input = calloc(2048, sizeof(char));
        memset(args, 0, 512 * sizeof(char *));



        // get and parse inputs into a command structure and free the now unneded input
        struct command *cmd = parseInput(input, args, isForeOnlyMode);
        free(input);

        // if there's a command execute it
        if (cmd != NULL) {
            // check if the command is built into the shell and execute the appropriate
            // command
            int builtInRes = isBuiltIn(cmd->args[0]);
            switch (builtInRes) {
                case CD_FLAG: cd(cmd->args, cmd->numArgs);
                    printPrompt();
                    break;
                case STATUS_FLAG:
                    showStatus();
                    printPrompt();
                    break;
                case EXIT_FLAG:
                    exitProgram(procList, cmd);
                    printPrompt();
                    run = 0;
                    break;
                default:

                    // if command is not build in process it using fork
                    if (cmd->isBgProcess) { // if it's a background process
                        // perform fork and save result
                        struct forkResult *res = forkBackground(cmd, procList);

                        // if result pid is 0 there was an error so free memory and exit
                        if (res->pid == 0){
                            if (cmd != NULL) {
                                freeCommand(cmd);
                            }
                            freeProcessList(procList);
                            free(res);
                            exit(1);
                        }
                        free(res);

                    } else {
                        // perform fork and save result
                        struct forkResult *res = forkForeground(cmd, procList, isForeOnlyMode);

                        // if result pid is 0 there was an error so free memory and exit
                        if (res->pid == 0){
                            if (cmd != NULL) {
                                freeCommand(cmd);
                            }
                            freeProcessList(procList);
                            free(res);
                            exit(1);
                        }

                        // save result's foreground only in case it was updated
                        isForeOnlyMode = res->isForeOnly;
                        free(res);
                    }
            }

            freeCommand(cmd);
        } else {
            // clear any finished background processes before presenting the next
            // command line prompt
            nonBlockClearFinished(procList);

            printPrompt();
        }

    }
}
/************************************************************************************
 * Function to perform initial setup for the program
 *
 * @param pid: proces id of the parent process
 ***********************************************************************************/
void initParentProc(pid_t pid) {
    // create vars more than large enough to hold the data
    char *temp = calloc(15, sizeof(char ));
    int length = 0;

    // fill the string with the pid and save it as an environment variable
    sprintf(temp, "%d", pid);
    setenv(PID, temp, 1);

    // get the length of the PID and save that as an environment variable
    length = strlen(temp);
    memset(temp, 0, 15*sizeof(char ));
    sprintf(temp, "%d", length);
    setenv(PID_LEN, temp, 1);

    free(temp);

    // load the handlers and set the initial status and FG toggle to 0
    loadHandlers(PARENT);
    setenv(STATUS, "exit code 0", 1);
    setenv("TOGGLE_FG_MODE", "0", 1);
}