/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Aidan Bayer-Calvert
* Student ID:: 922119403
* GitHub-Name:: abccodes
* Project:: Assignment 3 – Simple Shell with Pipes
*
* File:: bayercalvert_aidan_HW3_main.h
*
* Description:: Header file containing function prototypes and required libraries for the
* simple shell program.
*
**************************************************************/

#ifndef BAYERCALVERT_AIDAN_HW3_MAIN_H 
#define BAYERCALVERT_AIDAN_HW3_MAIN_H

// Including library providing printing , reading user input, and tokenizing functions.
#include <stdio.h>

// Including library that imports memory related functions for allocation and termination.
#include <string.h>

// String library including functions that measure length of stings and copies strings.
#include <stdlib.h>

// Including library for process creation and execution functions.
#include <unistd.h>

// Including library for process management functions.
#include <sys/wait.h>

// All function prototypes:
// Handles retrieving user input and processing edge cases.
int getUserInput(char *input, int bufferSize);

// Handles splitting user input into seperate commands based on pipe character.
void splitCommands(char *input, char **pipeSegments, int *numPipes);

// Splits individual command into its arguments.
void parseArguments(char *command, char **args);

// Executes commands, handling pipes for input/output. 
void executePipedCommands(char **pipeSegments, int numPipes, int argumentSize);

#endif