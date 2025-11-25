/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Aidan Bayer-Calvert
* Student ID:: 922119403
* GitHub-Name:: abccodes
* Project:: Assignment 4 – Processing FLR Data with Threads
*
* File:: bayercalvert_aidan_HW4_threading.c
*
* Description:: Contains declarations for threading related functions.
*
**************************************************************/

#ifndef THREADING_H
#define THREADING_H

#include "bayercalvert_aidan_HW4_processing.h"

// Function declarations for thread processing and memory management
// Processes a portion of the dataset based on assigned records.
void *threadFunction(void *arg);
// Processes a subset of event records within a given range, extracting relevant time
// differences.
void processEventTypeRange(
    const char *dataFile, int subfieldIndex, int startRecord, int endRecord
);
// Searches for an event type in a specific area, inserting it if not found.
int findOrInsertAreaEvent(AreaData *area, const char *eventType);
// Updates statistical data for a given event type in a specific area by adding time
// differences
void updateAreaEvent(
    AreaData *area, const char *eventType, int dispRec, int onscEnr, int onscRec
);
// Searches for an event type in the global event list, inserting it if not found, and returns
// its index.
int findOrInsertEventType(const char *eventType);

#endif