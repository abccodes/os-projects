/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Aidan Bayer-Calvert
* Student ID:: 922119403
* GitHub-Name:: abccodes
* Project:: Assignment 4 – Processing FLR Data with Threads
*
* File:: bayercalvert_aidan_HW4_helper.c
*
* Description:: Provides utility functions for tasks such as timestamp conversion, memory
* allocation and cleanup. Supports core functionalities needed in other modules.
*
**************************************************************/

// Solving error with strptime. Making sure strptime is being seen
#define _XOPEN_SOURCE 700

#include "bayercalvert_aidan_HW4_helper.h"
#include "bayercalvert_aidan_HW4_processing.h"

// Standard library functions like memory management
#include <stdlib.h>
// Standard input-output functions
#include <stdio.h>

// Converts timestamp string to epoch time
time_t convertToEpochTime(const char *timestamp) {
    
    // Inits a struct with time structure type to store the parsed data and time components.
    // This struct will hold values like the year,hour,month,day,etc
    struct tm timeStorageStructure = {0};
    
    // Parsing the timestamp string into the time struct, extracts and fills the fields of
    // timestruct based on format: "%m/%d/%Y %I:%M:%S %p".
    char *parseResult = strptime(timestamp, "%m/%d/%Y %I:%M:%S %p", &timeStorageStructure);

    // Checking to ensure the timestamp is valid and has no errors. If the timestamp is NULL or
    // empty, return an invalid value (-1).
    if (parseResult == NULL) {
        return -1;
    }

    // Parses the timesstamp string into the timestruct on that format. If strptime fails it 
    // will return -1 to indicate failure
    if (strptime(timestamp, "%m/%d/%Y %I:%M:%S %p", &timeStorageStructure) == NULL) {
        return -1;
    }

    // Convert the parsed time to epoch time
    time_t epochTime = mktime(&timeStorageStructure);

    // Return the epoch time
    return epochTime;
}

// Allocate memory for event and area data
void allocateMemory() {

    // Allocate memory for eventStats (array of EventData). Will store stats for up to MAX_EVENTS
    // unique event types.
    eventStats = (EventData *)malloc(MAX_EVENTS * sizeof(EventData));

    // Verifying if memory allocation was a sucess, if not exit and print err message.
    if (!eventStats) {
        perror("Memory allocation failed for eventStats");
        exit(EXIT_FAILURE);
    }

    // Allocate memory for area statistics (selectedAreas)
    for (int i = 0; i < 2; i++) {

        // Allocate memory for event breakdown in each area. Each area will have an array of
        // EventData to store events for that area.
        selectedAreas[i].areaEvents = (EventData *)malloc(MAX_EVENTS * sizeof(EventData));

        // Check if memory allocation was a fail, if so exit and print err message.
        if (!selectedAreas[i].areaEvents) {
            perror("Memory allocation failed for selectedAreas[i].areaEvents");
            exit(EXIT_FAILURE);
        }
    }
}

// Free allocated memory after processing
void freeMemory() {

    // Check if event stats has been allocated before attempting to free
    if (eventStats) {

        // Free memory allocated to eventStats
        free(eventStats);
        // Set the pointer to null to prevent dangling references
        eventStats = NULL;
    }

    // Iterated over selected areas and freeing memory for areas.
    for (int i = 0; i < 2; i++) {
        if (selectedAreas[i].areaEvents) {
            // Free allocated event array for each area
            free(selectedAreas[i].areaEvents);
            // Nullify pointer
            selectedAreas[i].areaEvents = NULL;
        }
    }
}
