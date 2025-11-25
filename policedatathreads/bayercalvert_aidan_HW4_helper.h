/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Aidan Bayer-Calvert
* Student ID:: 922119403
* GitHub-Name:: abccodes
* Project:: Assignment 4 – Processing FLR Data with Threads
*
* File:: bayercalvert_aidan_HW4_helper.h
*
* Description:: Defines utility functions that support timestamp conversions, memory management,
* and cleanup operations.
*
**************************************************************/

#ifndef HELPER_H
#define HELPER_H

// Include time.h to access time_t type
#include <time.h>

// Converts timestamp string to epoch time
time_t convertToEpochTime(const char *timestamp);

// Allocate memory for event and area data
void allocateMemory();

// Free allocated memory after processing
void freeMemory();

#endif