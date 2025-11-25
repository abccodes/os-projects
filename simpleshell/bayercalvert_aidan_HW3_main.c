/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Aidan Bayer-Calvert
* Student ID:: 922119403
* GitHub-Name:: abccodes
* Project:: Assignment 3 – Simple Shell with Pipes
*
* File:: bayercalvert_aidan_HW3_main.c
*
* Description:: This file contains code that creats a shell which allows users to input
* commands, execute them, and supports chaining multiple commands by using pipes.
*
**************************************************************/

#include "bayercalvert_aidan_HW3_main.h"

int main() {

    // Defining a fixed size of 103 bytes for input storage to ensure consistant memory
    // allocation.
    const int bufferSize = 103;
    
    // Defining a fixed arbitrary size for array that stores each argument. Since this is a
    // simple shell a fixed size for arguments will do just fine. If wanting to improve in the
    // future can edit to handle dynamic memory allocation. Same goes for pipeSegmentsSize.
    const int argumentSize = bufferSize;

    // Defining a fixed arbitrary size for array that stores each pipe segment.
    const int pipeSegmentsSize = bufferSize / 2;

    // A character array to store user input from the command line.
    char input[bufferSize];

    // While loop that keeps shell running indefinitely until exit is issued.
    while(1) {

        // Retrieves result of user input retrieved from command line.
        int userInputResult = getUserInput(input, bufferSize);

        // If status is -1 means exit condition is met so break/exit shell loop.
        if (userInputResult == -1) {
            break;
        }

        // If status is 2 means user inputted nothing and to re-prompt
        if (userInputResult == -2) {
            continue;
        }

        // Stores amount of pipes in user input to determine how many commands will need to be
        // executed.
        int numPipes = 0;

        // Array that stores individual command segment pointers by splitting through pipes.
        // Setting size of array to bufferSize / 2 because we set to bufferSize it is a waste
        // of memory. We need a smaller amount of storage because of pointers so this storage
        //value is an estimated arbitrary integer.
        char *pipeSegments[pipeSegmentsSize];

        // Splits the input into command segments based on presence of pipes.
        splitCommands(input, pipeSegments, &numPipes);
        
        // Executes the commands with appropriate redirection based on pip positioning.
        executePipedCommands(pipeSegments, numPipes, argumentSize);
    }
    return 0;
}

// Handles retrieving user input and processing edge cases.
int getUserInput(char *input, int bufferSize) {

    // Displays "prompt$"" to let user know shell is running and can be prompted.
    if (isatty(STDIN_FILENO)) {
        printf("prompt$ ");
    }

    // Reads user input and stores it in input buffer.
    char *userInput = fgets(input, bufferSize, stdin);

    // Checks wether input retrieval has failed. Ensures the shell can determine when to exit
    // and what exit condition has been met such as EOF or error.
    if (userInput == NULL) {

        // Differentiates between intentional input termination (EOF) and unexpected failures
        // and tells user in console that the shell is exiting. Returns 1 to indicate for
        // shell to exit.
        if(feof(stdin)) {
            return -1;
        } 

        // Reports failure in reading input and terminates execution as saftey precaution to
        // prevent undefined behavior.
        else {
            perror("Error reading input");
            exit(EXIT_FAILURE);
        }
    }

    // Removes newline character at the end of user input to avoid incorrect parsing.
    input[strcspn(input, "\n")] = '\0';

    // Checks if user entered "exit" if so, exit the shell gracefully.
    if (strcmp(input, "exit") == 0) {

        // Signal to main to exit the shell
        return -1;
    }

    // If the string length is 0, means user has not entered command, print error message and
    // re-prompt.
    if (strlen(userInput) == 0) {
        printf("Error, no command entered\n");
        return -2;
    }

    // Returns 0 to indicate successful input retrieval.
    return 0;
}

// Handles splitting user input into seperate commands based on pipe character.
void splitCommands(char *input, char **pipeSegments, int *numPipes) {

    // Counts occurences of the pipe character in input string and increments pipe count to
    // reflect total number of pipes.
    for (int i = 0; i < strlen(input); i++) {
        if (input[i] == '|') {
            (*numPipes)++; 
        }
    }

    // Initializing an index to keep track of the position in pipeSegments array.
    int pipeIndex = 0;

    // Splitting the input string into segments using pipes as delimiters.
    char *segment = strtok(input, "|");

    // Trimming leading spaces and trailing spaces to ensure no issues when executing commands.
    while (segment != NULL) {

        // Trim leading spaces by advancing pointer until non-space character is found.
        while (*segment == ' ') {
            segment++;
        }

        // Trim trailing spaces by moving backwards and replacing spaces with null terminators.
        char *end = segment + strlen(segment) - 1;
        while (end > segment && *end == ' ') {
            *end = '\0';
            end--;
        } 
            
        // Store the cleaned command segment in the pipSegments array.
        pipeSegments[pipeIndex++] = segment;

        // Move to the next psegment split by pipe.
        segment = strtok(NULL, "|");
        }

    // Marks the end of the pipeSegments array with NULL.
    pipeSegments[pipeIndex] = NULL;
}

// Splits individual command into its arguments.
void parseArguments(char *command, char **args) {

        // Index for tracking individual arguments in args array.
        int argIndex = 0;

        // Split the command segment into individual words by using spaces as delimiters.
        char *arg = strtok(command, " ");

        // Loops until all arguments are extracted.
        while (arg != NULL) {

            // Store the argument in the args array and continues extracting the next argument.
            args[argIndex++] = arg;
            arg = strtok(NULL, " "); 
        } 

    // Terminates the args array with NULL for proper future handling.   
    args[argIndex] = NULL;
}

// Executes commands, handling pipes for input/output. 
void executePipedCommands(char **pipeSegments, int numPipes, int argumentSize) {

    // Stores the read end of the previous pipe.
    int lastPipeReadingFileDescriptor = -1;

    // File descriptors for the current pipe.
    int pipeFileDescriptor[2];
    
    // Storage for all child process IDS
    pid_t pids[numPipes + 1];

    // Iterates over each command seperated by pipes.
    for (int i = 0; i < numPipes + 1; i++) {
        
        // Creates a pipe unless its the last command. If its the last command, there is no
        // need for redirection.
        if (i != numPipes) {
            if (pipe(pipeFileDescriptor) == -1) {
                perror("Pipe failed");
                exit(EXIT_FAILURE);
            }
        } 
        
        // Created a new child process to execute the current command.
        pid_t pid = fork();
        
        // Checks if fork has failed and prints error if it has.
        if (pid == -1) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        } 

        // Child execution block if fork has not failed.
        if (pid == 0) {
            
            // Allocates array that holds arguments. Arbitrary size of bufferSize/2 for memory
            // purposes and consistancy.
            char *args[argumentSize];

            // Parsing arguments.
            parseArguments(pipeSegments[i], args);

            // Redirects input from previous pipe if not the first command.
            if (i != 0) {

                // Redirects standard output.
                dup2(lastPipeReadingFileDescriptor, STDIN_FILENO);

                // Closes file descriptor after redirection.
                close(lastPipeReadingFileDescriptor);
            }
                
            // Redirects output to the write end of the current pipe if not the last command  .  
            if (i != numPipes) {

                // Redirects standard output.
                dup2(pipeFileDescriptor[1], STDOUT_FILENO);

                // Closes write end after redirection.
                close(pipeFileDescriptor[1]);
            } 
            
            // Closes read end of the pipe.
            close(pipeFileDescriptor[0]);
            
            // Replaces the child process with the command execution.
            execvp(args[0], args);

            // Prints error if replacing child process with command execution fails.
            perror("Execution failed");

            // Terminates if execution fails.
            exit(EXIT_FAILURE); 
            
        }
        
        // Store the PID of the child process
        pids[i] = pid;

        // Closes read end of previous pipe in parent process.
        if (i != 0) {
            close(lastPipeReadingFileDescriptor);
        } 
        
        // Stores the read end for the next iteration and closes the write end for the current
        // pipe.
        if (i != numPipes) {
            lastPipeReadingFileDescriptor = pipeFileDescriptor[0];
            close(pipeFileDescriptor[1]);
        }
    } 
    
    // Waits for all child processes and prints the PID + exit status
    for (int i = 0; i < numPipes + 1; i++) {
        int status;
        pid_t finishedPidResult = waitpid(pids[i], &status, 0);
        printf("Child %d exited with status %d\n", finishedPidResult, WEXITSTATUS(status));
    }
}