// # /**************************************************************
// # * Class::  CSC-415-03 Fall 2023
// # * Name:: Aidan James Bayer-Calvert
// # * Student ID:: 922119403
// # * GitHub-Name:: abccodes
// # * Project:: Assignment 1 – Command Line Arguments
// # *
// # * File:: bayer-calvert_aidan_HW1_main
// # *
// # * Description:: This C file demonstrates the use of 
// # * command-line arguments by printing the total number of 
// # * arguments and listing each one.
// # *
// # **************************************************************/

// printf library used to print amount of arguments and all arguments by user
#include  <stdio.h>

// argc(argument count) stores the total number of command-line arguments
// includes name of executable
// argv (argument vector) is an array of strings where 
// argv[0] = name of executable, argv[1] = first command line argument by user
int main(int argc, char *argv[]) {
    // prints to command line how many arguments are passed into file
    printf("There are %d arguments on the command line.\n", argc);

    // iterates over every argument to display them in command line
    for (int i = 0; i < argc; i++) {
        printf("Arg %d:\t%s\n", i, argv[i]);
    }

    // indicates sucessful program execution
    return 0;
}
