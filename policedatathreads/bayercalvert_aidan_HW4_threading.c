/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Aidan Bayer-Calvert
* Student ID:: 922119403
* GitHub-Name:: abccodes
* Project:: Assignment 4 – Processing FLR Data with Threads
*
* File:: bayercalvert_aidan_HW4_threading.c
*
* Description:: Contains multithreading logic that divides data processing among multiple threads
* to improve performance. Includes functionality for reading fix length records, managing
* event/area, and updating stats based on the parsed data.
*
**************************************************************/

#include "bayercalvert_aidan_HW4_threading.h"
#include "bayercalvert_aidan_HW4_helper.h"

// Standard input-output functions
#include <stdio.h>

// Standard library functions like memory management
#include <stdlib.h>

// String manipulation functions
#include <string.h>

// Allows file control operations
#include <fcntl.h>

// POSIX API functions, including file and thread management
#include <unistd.h>

// Provides multithreading support
#include <pthread.h>

// Provides file metadata structures and functions
#include <sys/stat.h>


// Searches for an event type in a specific area, inserting it if not found.
int findOrInsertAreaEvent(AreaData *area, const char *eventType) {
    // Check if the event already exists to ensure no unexpected behavior
    for (int i = 0; i < area->totalEvents; i++) {
        if (strcmp(area->areaEvents[i].eventType, eventType) == 0) {
            // Return index if event type is found
            return i;
        }
    }

    // If event type is not found, add a new entry if there is space
    if (area->totalEvents < MAX_EVENTS) {
        EventData *newEvent = &area->areaEvents[area->totalEvents];
         // Store event type name
        strncpy(newEvent->eventType, eventType, MAX_FIELD_NAME);
        // Initialize count to 0
        newEvent->count = 0;
        // Initialize time index to 0
        newEvent->timeIndex = 0;
        // Return the new event index and increment total events
        return area->totalEvents++;
    }
    // Returning -1 if max event limit is reached
    return -1;
}


// Updates statistical data for a given event type in a specific area by adding time
// differences
void updateAreaEvent (
    AreaData *area, const char *eventType, int dispRec, int onscEnr, int onscRec
    ) {
    // Get index of the event type in area
    int idx = findOrInsertAreaEvent(area, eventType);

    // If the event couldn't be inserted, exit early
    if (idx == -1) return;

    EventData *event = &area->areaEvents[idx];

    // Increment event occurrence count
    event->count++;

    // Store the time differences in the respective arrays if space allows
    if (event->timeIndex < MAX_TIME_ENTRIES) {
        int i = event->timeIndex;
        event->dispatch_received[i] = dispRec;
        event->onscene_enroute[i] = onscEnr;
        event->onscene_received[i] = onscRec;
        event->timeIndex++;
    }
}

// Function to find an event type in eventStats or add a new one
int findOrInsertEventType(const char *eventType) {
    // Check if event type exists in eventStats
    for (int i = 0; i < eventCount; i++) {
        if (!strcmp(eventStats[i].eventType, eventType)) {
            // Return the index if found to be used elsewhere
            return i;
        }
    }

    // Add new event type if space is available
    if (eventCount >= MAX_EVENTS) {
        fprintf(stderr, "Warning: Maximum event types reached!\n");
        return -1;
    }

    // If not found insert a new event type
    EventData *newEvent = &eventStats[eventCount++];
    strncpy(newEvent->eventType, eventType, MAX_FIELD_NAME);
    newEvent->count = 0;
    newEvent->timeIndex = 0;

    // Returning the new event index to be accessed later
    return eventCount - 1;
}

// Processes a subset of event records within a given range, extracting relevant time
// differences.
void processEventTypeRange (
    const char *dataFile, int subfieldIndex, int startRecord, int endRecord
    ) {
    // Open the data file in read-only mode, Using linux open and not fopen to follow directions
    // of low level operations.
    int fd = open(dataFile, O_RDONLY);

    // Need file to open correctly, checking this here, if not printing error and exiting.
    if (fd == -1) {
        perror("Error opening data file");
        pthread_exit(NULL);
    }

    // Computing the total number of records that this thread needs to process
    int totalRecordsToProcess = endRecord - startRecord;

    // Total number of records processes so far
    int recordsProcessed = 0;

    // Allocate a buffer for batch reading multiple records at once. Implemented this approach
    // to reduce the number of file calls because was having issues with threading improving
    // time. More threads were not decreasing time originally so I thought it might be because
    // of not batching the file operations.
    char *batchBuffer = (char *)malloc(RECORDS_PER_BATCH * recordSize);

    // Checking if memory fails, if so exit and print error
    if (!batchBuffer) {
        perror("Memory allocation failed for batchBuffer");
        close(fd);
        pthread_exit(NULL);
    }

    // Loop through the records assigned to this thread, reading and processing in batches.
    while (recordsProcessed < totalRecordsToProcess) {
        // Determine how many records to read in this batch. If fewer than records per batch 
        // remain. Tested different records per batch, from 1000-10000, seemed like 2000 offered
        // best performance.
        int recordsToRead = (totalRecordsToProcess - recordsProcessed) < RECORDS_PER_BATCH
                                ? (totalRecordsToProcess - recordsProcessed)
                                : RECORDS_PER_BATCH;

        // Compute the file offset where this batch starts. Each record is of fixed size so we
        // use just add and multiply
        off_t offset = (startRecord + recordsProcessed) * recordSize;

        // Read the batch of records from the file. pread func ensures thread safety by reading
        // from a specific offset without changing the file pointer.
        ssize_t bytesRead = pread(fd, batchBuffer, recordsToRead * recordSize, offset);
        if (bytesRead <= 0) break;

        // Process each record from the batch buffer
        for (int i = 0; i < recordsToRead; i++) {

            // Compute start position of the record in batch buffer
            char *recordBuffer = batchBuffer + (i * recordSize);
            int position = 0;

            // Init buffers for extracted field vals, will hold all data info
            char eventType[MAX_FIELD_NAME] = "";
            char area[MAX_FIELD_NAME] = "";
            char receivedTime[26] = "";
            char dispatchTime[26] = "";
            char onsceneTime[26] = "";
            char enrouteTime[26] = "";

            // Extract each field based on predefined field widths.
            for (int j = 0; j < numFields; j++) {

                // Copy the field value from the record buffer and null terminate string
                char fieldValue[fields[j].width + 1];
                strncpy(fieldValue, recordBuffer + position, fields[j].width);
                fieldValue[fields[j].width] = '\0';

                // move to next field position
                position += fields[j].width;

                // Assigning extracted values to corresponding variables
                if (j == receivedIndex)
                    strncpy(receivedTime, fieldValue, sizeof(receivedTime) - 1);
                if (j == dispatchIndex)
                    strncpy(dispatchTime, fieldValue, sizeof(dispatchTime) - 1);
                if (j == onsceneIndex)
                    strncpy(onsceneTime, fieldValue, sizeof(onsceneTime) - 1);
                if (j == enrouteIndex)
                    strncpy(enrouteTime, fieldValue, sizeof(enrouteTime) - 1);
                if (j == eventTypeIndex)
                    strncpy(eventType, fieldValue, MAX_FIELD_NAME);
                if (j == subfieldIndex)
                    strncpy(area, fieldValue, MAX_FIELD_NAME);
            }

            // Error checking if the record doesnt have an event type we skip to avoid
            // unexpected behavior
            if (strlen(eventType) == 0)
                continue;

            // Convert timestamps to epoch time to be more readable
            time_t received_epoch = convertToEpochTime(receivedTime);
            time_t dispatch_epoch = convertToEpochTime(dispatchTime);
            time_t onscene_epoch = convertToEpochTime(onsceneTime);
            time_t enroute_epoch = convertToEpochTime(enrouteTime);

            // Compute time differences (use -1 for missing data). These calculations determine
            // the response times between different event phases.
            int dispRec = (
                dispatch_epoch > 0 && received_epoch > 0) ? (dispatch_epoch - received_epoch
                ) : -1;
            int onscEnr = (
                onscene_epoch > 0 && enroute_epoch > 0) ? (onscene_epoch - enroute_epoch
                ) : -1;
            int onscRec = (
                onscene_epoch > 0 && received_epoch > 0) ? (onscene_epoch - received_epoch
                ) : -1;

            // Determine if this record belongs to one of the selected areas.
            int selectedAreaIdx = -1;
            if (strcmp(area, selectedAreas[0].name) == 0)
                selectedAreaIdx = 0;
            else if (strcmp(area, selectedAreas[1].name) == 0)
                selectedAreaIdx = 1;

            // Lock the mutex to ensure only one thread updates shared data at a time
            pthread_mutex_lock(&statsMutex);

            // Update the global eventStats array.
            int eventIdx = findOrInsertEventType(eventType);
            if (eventIdx != -1) {
                eventStats[eventIdx].count++;
                if (eventStats[eventIdx].timeIndex < MAX_TIME_ENTRIES) {
                    eventStats[eventIdx].dispatch_received[eventStats[eventIdx].timeIndex] = dispRec;
                    eventStats[eventIdx].onscene_enroute[eventStats[eventIdx].timeIndex] = onscEnr;
                    eventStats[eventIdx].onscene_received[eventStats[eventIdx].timeIndex] = onscRec;
                    eventStats[eventIdx].timeIndex++;
                }
            }

            // Update the selected area’s data if it matches
            if (
                selectedAreaIdx != -1 && selectedAreas[selectedAreaIdx].timeIndex < MAX_TIME_ENTRIES
            ) {
                // Store response time differences for the selected area
                selectedAreas[
                    selectedAreaIdx
                ].dispatch_received[selectedAreas[selectedAreaIdx].timeIndex] = dispRec;
                selectedAreas[
                    selectedAreaIdx
                ].onscene_enroute[selectedAreas[selectedAreaIdx].timeIndex] = onscEnr;
                selectedAreas[
                    selectedAreaIdx
                ].onscene_received[selectedAreas[selectedAreaIdx].timeIndex] = onscRec;
                selectedAreas[
                    selectedAreaIdx
                ].timeIndex++;

                 // Also update area-specific statistics for event types
                updateAreaEvent(
                    &selectedAreas[selectedAreaIdx], eventType, dispRec, onscEnr, onscRec
                );
            }

            // Unlock the mutex after shared data has been updated.
            pthread_mutex_unlock(&statsMutex);
        }

        // Update the counter for processed records
        recordsProcessed += recordsToRead;
    }

    // Free allocated memory and close the file to ensure termination is safe
    free(batchBuffer);
    close(fd);
}

// Processes a portion of the dataset based on assigned records.
void *threadFunction(void *arg) {
    // Convert the generic void star argument back to a `ThreadArgs *` structure because threads
    // (`pthread_create`) require a `void *` argument, so we pass a `ThreadArgs *` when creating
    //  the thread and cast it back inside the function.e
    ThreadArgs *args = (ThreadArgs *)arg;

    // Get the unique identifier for the current thread. pthread_self()` returns the calling
    // thread's ID, which is useful for tracking a thread
    pthread_t thread_id = pthread_self();

    //Debug statements for debugging which threads are running and when the complete
    // printf("[THREAD %lu] Starting | Processing records: %d to %d\n",
        //    thread_id, args->startRecord, args->endRecord);
    
    // Call prcoesseventrange to process the assigned range of records per a thread to increase
    // efficiency.
    processEventTypeRange(args->dataFile, args->subFieldIndex, args->startRecord, args->endRecord);

    //Debug statements for debugging which threads are running and when the complete
    // printf("[THREAD %lu] Finished processing\n", thread_id);

    // Terminate the thread
    pthread_exit(NULL);
}