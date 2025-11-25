/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Aidan Bayer-Calvert
* Student ID:: 922119403
* GitHub-Name:: abccodes
* Project:: Assignment 4 – Processing FLR Data with Threads
*
* File:: bayercalvert_aidan_HW4_processing.c
*
* Description:: Responsible for parsing dataset, extracting field values, computing time
* differences, and storing structured event and area stats.
*
**************************************************************/

#include "bayercalvert_aidan_HW4_processing.h"

// Standard input-output functions
#include <stdio.h>
// Standard library functions like memory management
#include <stdlib.h>
// String manipulation functions
#include <string.h>
// Allows file control operations
#include <fcntl.h>
// including file and thread management functions
#include <unistd.h>
// Provides file metadata structures and functions
#include <sys/stat.h>

// Variables for tracking parsed data:
// Tracks the number of unique event types recorded
int eventCount = 0;
// Stores the number of fields defined in the header file
int numFields = 0;
// Stores the total size of a single record, computed from field widths
int recordSize = 0;
// Stores the total number of records in the data file (computed dynamically)
int totalRecords = 0;

// Indices of key timestamp fields (initialized to -1 to indicate "not found")
// These values are assigned dynamically when parsing the header file.
// Index for "received_datetime" (when a call was first received)
int receivedIndex = -1;
// Index for "dispatch_datetime" (when a unit was dispatched)
int dispatchIndex = -1;
// Index for "onscene_datetime" (when a unit arrived on the scene)
int onsceneIndex = -1;
// Index for "enroute_datetime" (when a unit began traveling to the scene)
int enrouteIndex = -1;
// Index for "call_type_final_desc" (the final categorized event type)
int eventTypeIndex = -1;

// A mutex to prevent race conditions when threads update shared statistics
pthread_mutex_t statsMutex = PTHREAD_MUTEX_INITIALIZER;

// Gloval array to store statistics for different event types
EventData *eventStats = NULL;

// Global array to store statistics for two selected areas specified in CL arguments
AreaData selectedAreas[2];

// Global array to store field definitions extracted from the header file
FieldInfo fields[MAX_FIELDS];

// parse the header file and extract field names and widths
void parseHeaderFile(const char *headerFile) {
    // Open the header file in read-only mode
    int fd = open(headerFile, O_RDONLY);

    // Checks if file opened correctly, if not, printed error and exits
    if (fd == -1) {
        perror("Error opening header file");
        exit(EXIT_FAILURE);
    }

    // Buffer to read the file and store each line
    char buffer[BUFFER_SIZE];

    // How many bytes read from file
    ssize_t bytesRead;
    
    // Index for tracking cur position in the buffer
    int currBufferIndex = 0;

    // Read the header file one byte at a time
    while ((bytesRead = read(fd, &buffer[currBufferIndex], 1)) > 0) {

        // If a newline is reached or buffer is full, process the current line
        if (buffer[currBufferIndex] == '\n' || currBufferIndex == BUFFER_SIZE - 1) {

            // Null-terminate the string, end of string
            buffer[currBufferIndex] = '\0';

            // Ensure we don't exceed field storage limit.Set to an arbitrary large amount
            // for memory efficiency.
            if (numFields < MAX_FIELDS) {

                // Split line by colon
                char *fieldWidthAndName = strtok(buffer, ":");

                // Error checking to avoid unexpected behavior. If it exists continue.
                if (fieldWidthAndName) {

                    // Extract field width
                    fields[numFields].width = atoi(fieldWidthAndName);

                    // Get field name
                    fieldWidthAndName = strtok(NULL, ":");

                    // Error checking to avoid unexpected behavior. If it exists continue.
                    if (fieldWidthAndName) {

                        // Extract field name
                        strncpy(fields[numFields].name, fieldWidthAndName + 1, MAX_FIELD_NAME);

                        // Add to total record size
                        recordSize += fields[numFields].width;

                        // Identify and store index of important fields for later lookup
                        if (strcmp(fields[numFields].name, "received_datetime") == 0) {
                            receivedIndex = numFields;
                        }
                        else if (strcmp(fields[numFields].name, "dispatch_datetime") == 0) {
                            dispatchIndex = numFields;
                        }
                        else if (strcmp(fields[numFields].name, "onscene_datetime") == 0) {
                            onsceneIndex = numFields;
                        }
                        else if (strcmp(fields[numFields].name, "enroute_datetime") == 0) {
                            enrouteIndex = numFields;
                        }
                        else if (strcmp(fields[numFields].name, "call_type_final_desc") == 0) {
                            eventTypeIndex = numFields;
                        }
                        else if 
                        (
                        strcmp(
                            fields[numFields].name, "call_type_original_desc"
                            ) == 0 && eventTypeIndex == -1
                        )
                            eventTypeIndex = numFields;

                        // Increment field count since a field that was valid was parsed.
                        numFields++;
                    }
                }
            }

            // Reset buffer index for the next line reading
            currBufferIndex = 0;
        } else {

            // Move to the next character in the buffer
            currBufferIndex++;
        }
    }

    // Close the header file after processing
    close(fd);
}

// Calculate the total number of records in the data file
void calculateTotalRecords(const char *dataFile) {

    // Struct to store file metadata
    struct stat fileMetaData;

    // Open the data file in read-only mode
    int fd = open(dataFile, O_RDONLY);

    // Check if it fails to open, if so print error and exit
    if (fd == -1) {
        perror("Error opening data file");
        exit(EXIT_FAILURE);
    }

    // Retrieve file size using fstat() and store value
    int result = fstat(fd, &fileMetaData);

    // Calculate total records using file size
    totalRecords = fileMetaData.st_size / recordSize;

    // Close the file
    close(fd);
}

// // Displays parsed header information for testing
// void printHeaderInfo() {
//     printf("Parsed header info\n");
//     printf("Total record size: %d bytes \n", recordSize);
//     for (int i = 0; i < numFields; i++){
//         printf(
//         "Indexes: receivedIndex=%d, dispatchIndex=%d, onsceneIndex=%d, enrouteIndex=%d\n", 
//         receivedIndex, dispatchIndex, onsceneIndex, enrouteIndex
//         );
//     }
// }
