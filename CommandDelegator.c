#include "CommandParser.h"
#include "CommandDelegator.h"

/************************************************************************************
 * Blocking function to wait for a specified process to finish and preform related
 * clean up operations
 *
 * @param targetProcess: pid of the process to wait for
 ***********************************************************************************/
int clearFinished(pid_t targetProcess, int hideStatus) {
    int statusCode = 0, result;
    char *stat = calloc(100, sizeof(char));

    // wait for the indicated process and load its status into statusCode
    result = waitpid(targetProcess, &statusCode, 0);

    if (!hideStatus) {
        // if statuscode is 0 the process terminated normally so load stat with "exit value 0"
        if (statusCode == 0) {
            sprintf(stat, "exit value 0\n");

            // is the process exited set stat to be "exit value #" where # is the value used
            // in the exit call
        } else if (WIFEXITED(statusCode) == 1) {
            statusCode = WEXITSTATUS(statusCode);
            sprintf(stat, "exit value %d\n", statusCode);

            // if neither of the previous if's evaluate to true then the process was terminated
            // by a signal so set stat to indicate as such with the signal number (and display
            // that it was terminated)
        } else {
            sprintf(stat, "terminated by signal %d\n", WTERMSIG(statusCode));
            printf("%s", stat);
        }
    }

    // set the status environment variable to stat for use with the status command
    setenv(STATUS, stat, 1);
    free(stat);

    // since this function is the one most likely to be active during a switch
    // to foreground only mode this return value indicates that the clearing finished
    return result;
}

/************************************************************************************
 * A non-blocking function to wait for any non finished child process and preform
 * related clean up operations
 *
 * @param processList: a linked list of processes that have yet to be collected by
 *                     the parent
 ***********************************************************************************/
void nonBlockClearFinished(struct processLinkedList *processList){
    int statusCode = 0;
    pid_t finishedProcess = 0;

    // while there are still processes waiting to be collected collect them
    while((finishedProcess = waitpid(-1, &statusCode, WNOHANG)) > 0){
        // if we can successfully remove the process from the list
        if (removeProcess(processList, finishedProcess)) {
            // print the process id that was collected and how it terminated
            printf("background pid %d is done:", finishedProcess);
            if (statusCode == 0) {
                printf(" exit value 0\n");
            } else if (WIFEXITED(statusCode) == 1) {
                printf(" exit value %d\n", WEXITSTATUS(statusCode));
            } else {
                printf(" terminated by signal %d\n", WTERMSIG(statusCode));
            }
            fflush(stdout);
        }
    }
}

/************************************************************************************
 * Function to implement behavior to change the current working directory
 *
 * @param args: list of arguments
 * @param numArgs: number of arguments
 ***********************************************************************************/
void cd(char **args, int numArgs) {
    // only 1 argument change directory to home, otherwise change to directory
    // indicated in arg[1]
    if (numArgs < 2) {
        chdir(getenv("HOME"));
    } else {
        chdir(args[1]);
    }
}

/************************************************************************************
 * Function to show the most recent status code from a foreground process
 ***********************************************************************************/
void showStatus() {
    // get the status from the environment variables and print it
    printf(getenv(STATUS));
    fflush(stdout);
}

/************************************************************************************
 * Function to exit the shell. It kills all outstanding child processes and frees
 * the memory still in use at time of function call.
 *
 * @param processList: linked list of outstanding processes
 * @param cmd: current command structure to be freed
 ***********************************************************************************/
void exitProgram(struct processLinkedList *processList, struct command *cmd) {
    // while there are still outstanding child processes
    while (processList->head){
        // kill them and remove them from the proc list and process linked list
        kill(processList->head->pid, SIGTERM);
        clearFinished(processList->head->pid, 1);
        removeProcess(processList, processList->head->pid);
    }
    // free the linked list and set it to NULL
    freeProcessList(processList);

    // free the memory for cmd
    freeCommand(cmd);

    // unset environment variables
    unsetenv(PID);
    unsetenv(PID_LEN);
    unsetenv(STATUS);
    printf("\033[0m\n");

    // exit the program
    exit(0);
}

/************************************************************************************
 * Function to fork a foreground child process.
 *
 * @param cmd: command for the child process to perform
 * @param isForeOnlyMode: is the shell in foreground-only mode
 * @return: pid of the current process (used in calling function in case of errors)
 ***********************************************************************************/
struct forkResult *forkForeground(struct command *cmd, struct processLinkedList *procList, int isForeOnlyMode) {
    // fork the process
    int pid = fork();

    // create and initialize variable to save the results of this forked proces
    struct forkResult *res = calloc(1, sizeof(struct forkResult));
    res->pid = pid;
    res->isForeOnly = isForeOnlyMode;

    if (pid < 0) { // error
        printf("Error forking process");
        fflush(stdout);

    } else if (pid > 0) { // parent
        // loop until clear finish completes successfully
        while (clearFinished(pid, 0) == -1);
        // check for toggle flag and toggle mode if so
        if (toggleFgMode){
            applyFgOnlyToggle(&isForeOnlyMode);
            res->isForeOnly = isForeOnlyMode;
        }

        // clear any finished background processes before presenting the next
        // command line prompt
        nonBlockClearFinished(procList);

        printPrompt();

    } else { // child
        // load child handlers open io redirect files if necessary and execute the command
        loadHandlers(FOREGROUND | CHILD);

        // if files could be opened execute command
        if (openRedirFiles(cmd)){
            execvp(cmd->args[0], cmd->args);
            printf("%s: no such file or directory\n", cmd->args[0]);
            fflush(stdout);
        }
    }

    // return result of fork for parent always, and child if there was an error
    return res;
}

/************************************************************************************
 * Function to fork off a background child process to perform the command indicated
 * by the command struct parameter
 *
 * @param cmd: command for the child process to perform
 * @param processList: linked list of outstanding processes
 * @return: pid of the current process (used in calling function in case of errors)return
 ***********************************************************************************/
struct forkResult * forkBackground(struct command *cmd, struct processLinkedList *processList) {
    // fork process
    int pid = fork();

    // create and initialize variable to save the results of this forked proces
    struct forkResult *res = calloc(1, sizeof(struct forkResult));
    res->pid = pid;
    res->isForeOnly = 0;

    if (pid < 0) {
        printf("Error forking process");
        fflush(stdout);

    } else if (pid > 0) {  // parent
        // add process to process linked list and clear any finished processes
        addProcess(processList, pid);
        nonBlockClearFinished(processList);
        int i = 0;
        while (i < 200)
            i++;
    } else { // child
        // load handlers, display pid of child process, open any files necessary
        // for io redirection, and perform the command
        loadHandlers(BACKGROUND | CHILD);
        printf("backround pid is %d\n", getpid());
        fflush(stdout);

        // clear any finished background processes before presenting the next
        // command line prompt
        nonBlockClearFinished(processList);

        printPrompt();
        // if files could be opened execute command
        if (openRedirFiles(cmd)) {
            execvp(cmd->args[0], cmd->args);
            printf("%s: no such file or directory\n", cmd->args[0]);
            fflush(stdout);
        }
    }

    // return result of fork for parent always, and child if there was an error
    return res;
}

/************************************************************************************
 * Function to add a process to the outstanding child process list (to ensure that
 * all child processes are killed when the exit command is called)
 *
 * @param procList: linked list of processes
 * @param pid: process id of the process to add to the list
 ***********************************************************************************/
void addProcess(struct processLinkedList *procList, int pid) {
    // save reference to end of linked list
    struct processNode *tail = procList->tail;

    // if tail is not null then add a new node to the end of the list
    if (tail != NULL) {
        tail->next = calloc(1, sizeof(struct processNode));
        procList->tail = tail->next;

    // if tail is null then list was empty so add new node to start/end of list
    } else {
        tail = calloc(1, sizeof(struct processNode));
        procList->head = tail;
        procList->tail = tail;
    }

    // set the new node's pid
    procList->tail->pid = pid;
}

/************************************************************************************
 * Function to remove a process from the linked list (called when clearing background
 * processes)
 *
 * @param procList: linked list of outstanding processes
 * @param pid: pid of the process to remove
 * @return: integer value to indicate if the remove was successful
 ***********************************************************************************/
int removeProcess(struct processLinkedList *procList, int pid) {
    // save a reference to the start of the list
    struct processNode *cur = procList->head;

    // if list is not empty continue otherwise return false
    if (procList->head != NULL) {
        // if the head contains the process remove and free it
        if (procList->head->pid == pid) {
            if (procList->head == procList->tail){
                procList->head = procList->head->next;
                procList->tail = procList->head;
            } else {
                procList->head = procList->head->next;
            }
            free(cur);
            return TRUE;

        // otherwise search the list for the matching node and remove/free it
        } else {
            while (cur != NULL && cur->next != NULL) {
                if (cur->next->pid == pid) {
                    struct processNode *temp = cur->next;

                    // if the tail contains the pid update tail to the node before it
                    if (cur->next != NULL && cur->next->pid == procList->tail->pid){
                        procList->tail = cur;
                    }
                    cur->next = cur->next->next;
                    free(temp);
                    return TRUE;
                } else {
                    cur = cur->next;
                }
            }
        }
    }
    return FALSE;
}

/************************************************************************************
 * Function to open a file necessary for io redirection and redirect io accordingly
 *
 * @param cmd: command struct containing all relevant information about the command
 *             to be executed
 * @param isOutput: indicates whether the file is an output file or not
 * return: flag indicating whether the open operation was successful
 ***********************************************************************************/
int openRedirFile(struct command *cmd, int isOutput) {
    int flags = 0, perms = 0750, idx = 0;

    // set flags based on whether opening for input or output
    if (!isOutput) {
        flags = O_RDONLY;
        idx = cmd->infileIdx;
    } else {
        flags = O_WRONLY | O_TRUNC | O_CREAT;
        idx = cmd->outfileIdx;
    }

    // open the file
    int fd = open(cmd->args[idx], flags, perms);

    if (fd == -1){  //error state
        // print appropriate error message and exit with value 1
        printf("cannot open %s for ", cmd->args[idx]);
        if (isOutput){
            printf("output\n");
        } else {
            printf("input\n");
        }
        fflush(stdout);
        return FALSE;

    } else {   // success state
        // redirect input or output to the opened file based on whether is output
        // is true ( == 1) or false ( == 0), then set it to close when the exec
        // call is finished with it
        dup2(fd, isOutput);
        fcntl(fd, F_SETFD, FD_CLOEXEC);
    }

    return TRUE;
}

/************************************************************************************
 * Function to open files for io redirection if required for the current command
 *
 * @param cmd: struct containing information about current command
 * return: status flag indicating whether the operation was successful
 ***********************************************************************************/
int openRedirFiles(struct command *cmd) {
    int res = TRUE;  // status flag used to indicate if opening files was successful

    // if cmd has an infile index set then it needs an input file open for redirection
    if (cmd->infileIdx != -1) {
        // open file for input redirection
        res &= openRedirFile(cmd, FALSE);
    }

    // if cmd has an outfile index set then it needs an output file open for redirection
    if(cmd->outfileIdx != -1 && res) {
        // open file for output redirection
        res &= openRedirFile(cmd, TRUE);
    }

    return res;
}

/************************************************************************************
 * Debugging function to print the contents of the linked list of outstanding
 * processes
 *
 * @param procList: linked list of outstanding processes
 ***********************************************************************************/
void printProcList(struct processLinkedList *procList) {
    printf("Printing proc list:\n");
    struct processNode *cur = procList->head;
    while (cur){
        printf("%d\n", cur->pid);
        cur = cur->next;
    }
    fflush(stdout);
}

/************************************************************************************
 * Function to free the entirety of the linked list of processes (used to prevent
 * memory leaks on failure during exec of child process)
 *
 * @param procList: linked list of outstanding proceses
 * @return:
 ***********************************************************************************/
void freeProcessList(struct processLinkedList *procList) {
    // if the list exists
    if (procList != NULL) {
        //save reference to head and loop through
        struct processNode *cur = procList->head;
        procList->tail = NULL;

        // while cur free it and move to next
        while (cur != NULL) {
            cur = cur->next;
            free(procList->head);
            procList->head = cur;
        }

        // free the linked list struct
        free(procList);
    }
}

/************************************************************************************
 * Function to print the prompt for the next command line of the shell
 ***********************************************************************************/
void printPrompt() {
    // print command line indicator
    printf("\033[92m: \033[96m");
    fflush(stdout);
}