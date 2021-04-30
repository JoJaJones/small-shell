#include "CommandParser.h"
#include "CommandDelegator.h"

/*************************************************************************************
 * Function to detect if the received command is one of the built in commands
 *
 * @param command: character array containing the command to check
 * @return: an integer representation of the command in the character array (if any
 * matches otherwise 0
 ************************************************************************************/
int isBuiltIn(char *command) {
    int commandVal = 0;
    if (strcmp(command, "cd") == 0){
        commandVal += CD_FLAG;
    }

    if (strcmp(command, "status") == 0) {
        commandVal += STATUS_FLAG;
    }

    if (strcmp(command, "exit") == 0) {
        commandVal += EXIT_FLAG;
    }

    return commandVal;
}

/*************************************************************************************
 * Helper function to detect whitespace in the string passed to the command line
 *
 * @param c: character to check
 * @return: 0 if the character is not whitespace, non-zero if it is
 ************************************************************************************/
int isWhitespace(char c) {
    return !(c >= '!' && c <= '~');
}

/*************************************************************************************
 * Function to parse the command line and load the information into a command struct
 * for use in the main loop of the shell
 *
 * @param input: raw command line input
 * @param args: array of character pointers to hold the arguments
 * @param isForeOnlyMode: int value indicating whether current operation mode is
 *      foreground only mode or not
 * @return: a filled command struct of NULL if command line is empty or a comment
 ************************************************************************************/
struct command *parseInput(char *input, char **args, int isForeOnlyMode) {
    size_t len = 2048;
    gets(input);
    int numArg = stripWhiteSpace(input, args);;

    // if cmd is blank or a comment skip this process
    if (numArg == 0 || args[0][0] == '#'){
        return NULL;
    }

    // create the command structure and set the file indices to invalid settings
    struct command *parsedCommand = calloc(1, sizeof(struct command));
    parsedCommand->hasOutfile = FALSE;
    parsedCommand->hasInfile = FALSE;

    // strip the whitespace from the raw command and set the number of args
    parsedCommand->numArgs = numArg;
    parsedCommand->args = args;

    // parse each arg, performing variable expansion as necessary
    parseAllArgs(args, parsedCommand, isForeOnlyMode);
    // if the command is echo, make it print purple
    if (strcmp(parsedCommand->args[0], "echo") == 0) echoModifier(parsedCommand);

    return parsedCommand;
}

/*************************************************************************************
 * Function to strip the whitespace from the raw input and load pointers to the first
 * characters of each arg into the argument array
 *
 * @param input: raw input
 * @param args: array of character pointers to hold the arguments
 * @return: number of arguments found
 ************************************************************************************/
int stripWhiteSpace(char *input, char **args) {
    int i, ptrIdx = 0, newArg = 1, length = strlen(input);
    // iterate through the input
    for (i = 0; i < length; i++) {
        // if it's not whitespace and the newArg flag is set, set the next pointer for
        // args
        if (!isWhitespace(input[i]) && newArg) {
            args[ptrIdx] = input + i;
            newArg = FALSE;
            ptrIdx++;

        // if it's whitespace set input to a null character and set the newArg flag
        } else if (isWhitespace(input[i])) {
            input[i] = 0;
            newArg = TRUE;
        }
    }

    // return number of arguments detected
    return ptrIdx;
}

/*************************************************************************************
 * Function to parse all the args, allocating new memory, performing variable expansion
 * and updating the command structure as appropriate
 *
 * @param args: array of args to parse
 * @param cmd: the cmd structure to be loaded
 * @param isForeOnlyMode: flag for forground only mode
 ************************************************************************************/
void parseAllArgs(char **args, struct command *cmd, int isForeOnlyMode) {
    int i, ptrIdx = cmd->numArgs;

    // iterate through args
    for (i = 0; i < ptrIdx; i++) {
        args[i] = parseArg(args[i]);  // parse each arg
        // if new the end of the arg list
        if (i >= ptrIdx - 5) {
            // check for input file and update cmd appropriately
            if (strcmp(args[i], "<") == 0) {
                // free the arg
                free(args[i]);
                args[i] = NULL;

                // set index and adjust number of arguments
                cmd->hasInfile = TRUE;
                cmd->infile = parseArg(args[i + 1]);
                args[i + 1] = NULL;
                cmd->numArgs -= 2;
                i++;
                // check for output file and update cmd appropriately
            } else if (strcmp(args[i], ">") == 0) {
                // free the arg
                free(args[i]);
                args[i] = NULL;

                // set index and adjust number of arguments
                cmd->hasOutfile = TRUE;
                cmd->outfile = parseArg(args[i + 1]);
                args[i + 1] = NULL;
                cmd->numArgs -= 2;
                i++;
            }
        }
    }
    // check if command should be run in background
    if (args[ptrIdx - 1] && strcmp(args[ptrIdx - 1], "&") == 0) {
        // if the command is not a built in command and we're not in forground only mode
        // set isBgProcess to true
        cmd->isBgProcess = (!isBuiltIn(args[0]) && !isForeOnlyMode);

        // free the arg and adjust arg count
        free(args[ptrIdx - 1]);
        cmd->numArgs--;

        setNullRedirects(cmd);
        args[ptrIdx - 1] = NULL;
    }
}

/*************************************************************************************
 * Function to parse a single argument. It will determine if variable expansion is
 * necessary and expand the variable to contain the process id of smallsh if so
 *
 * @param rawArg: the unprocessed character array containing the arg
 * @return: the processed argument
 ************************************************************************************/
char *parseArg(char *rawArg) {
    char* result = NULL;
    int pidLen = atoi(getenv(PID_LEN));

    // count the number of variables that need expansion in the argument
    int newLength, varToExpand = countVars(rawArg);

    // calculate the new length of the arg after expansion
    newLength = (pidLen - 2) * varToExpand + strlen(rawArg) + 1;

    // allocate memory and load the arg with the expanded variables (if necessary
    result = calloc(newLength, sizeof(char));
    expandVariables(result, rawArg);

    return result;
}

/*************************************************************************************
 * Function to count the number of variables in need of expansion in the argument
 *
 * @param rawArg: the raw argument
 * @return: number of $$ found in the argument text
 ************************************************************************************/
int countVars(char *rawArg) {
    int i = 0, len = strlen(rawArg);
    int count = 0;

    // iterate through the string and count the number of occurences of $$
    while(i < len - 1) {
        if (rawArg[i] == '$' && rawArg[i + 1] == '$'){
            count += 1;
            i++; // skip ahead if $$ found so $$$ doesn't result in 2 counted
        }
        i++;
    }

    return count;
}

/*************************************************************************************
 * Function to expand variables to be replaced with process id
 *
 * @param dest: character array to load with expanded variables
 * @param source: character array containing raw argument
 * @return: filled resulting character array
 ************************************************************************************/
char *expandVariables(char *dest, char *source) {
    int i, curIdx = 0;

    // iterate through raw data
    for (i = 0; i < strlen(source); i++){
        //if more than 1 character from end check if $$ is next
        if(i < strlen(source) - 1 && source[i] == '$' && source[i + 1] == '$'){
            // load dest with contents of dest + process id
            sprintf(dest, "%s%s", dest, getenv(PID));

            // adjust current index for dest by length of process id
            curIdx += atoi(getenv(PID_LEN));

            i++;  // skip a character in source
        } else {
            // load current character of source to target index of dest
            dest[curIdx] = source[i];
            curIdx++;
        }
    }

    return dest;
}

/*************************************************************************************
 * Function to release the dynamic memory allocated to the command struct
 *
 * @param cmd: command struct to be released
 ************************************************************************************/
void freeCommand(struct command *cmd) {
    int i;
    // free each arg in the arg array
    for (i = 0; i < cmd->numArgs; i++){
        free(cmd->args[i]);
        cmd->args[i] = NULL;
    }

    // if there's an outfile character array free it
    if (cmd->hasOutfile == TRUE){
        free(cmd->outfile);
        cmd->outfile = NULL;
    }

    // if there's an infile character array free it
    if (cmd->hasInfile == TRUE){
        free(cmd->infile);
        cmd->infile = NULL;
    }

    // free the memory for the base structure
    free(cmd);
}

/*************************************************************************************
 * Function to set redirects to /dev/null if no redirects are otherwise specified
 *
 * @param cmd: command to have it's redirect set to null
 ************************************************************************************/
void setNullRedirects(struct command *cmd) {
    if (!cmd->hasInfile) {
        cmd->hasInfile = TRUE;
        cmd->infile = calloc(strlen("/dev/null") + 1, sizeof(char));
        sprintf(cmd->infile, "/dev/null");
    }

    if (!cmd->hasOutfile) {
        cmd->hasOutfile = TRUE;
        cmd->outfile = calloc(strlen("/dev/null") + 1, sizeof(char));
        sprintf(cmd->outfile, "/dev/null");
    }
}

/*************************************************************************************
 * Function to add color codes to echo commands
 *
 * @param cmd: command to be modified
 ************************************************************************************/
void echoModifier(struct command *cmd) {
    // if we're not echoing a blank line add the purple text escape character and the
    // character to turn off purple text
    if (cmd->numArgs > 1) {
        char *arg = cmd->args[1];
        cmd->args[1] = calloc(strlen(arg) + 6, sizeof(char));
        sprintf(cmd->args[1], "\033[95m%s", arg);
        free(arg);

        arg = cmd->args[cmd->numArgs - 1];
        cmd->args[cmd->numArgs - 1] = calloc(strlen(arg) + 6, sizeof(char));
        sprintf(cmd->args[cmd->numArgs - 1], "%s\033[0m", arg);
        free(arg);
    }
}
