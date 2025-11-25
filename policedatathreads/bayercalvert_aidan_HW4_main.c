/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Aidan Bayer-Calvert
* Student ID:: 922119403
* GitHub-Name:: abccodes
* Project:: Assignment 4 – Processing FLR Data with Threads
*
* File:: bayercalvert_aidan_HW4_main.c
*
* Description:: Serves as the entry point for processing fix length record data (FLR) with
* threads. Processes FLR dataset using multithreading. Reads and parses a header file and
* displays computed time and area related statistics.
*
**************************************************************/

// Standard input-output functions
#include <stdio.h>
// Standard library functions like memory management
#include <stdlib.h>
// Provides multithreading support
#include <pthread.h>
// Provides time-related functions and structures
#include <time.h>
// String manipulation functions
#include <string.h>

// Including all other files to access all written functions, variables, and data structures
#include "bayercalvert_aidan_HW4_processing.h"
#include "bayercalvert_aidan_HW4_threading.h"
#include "bayercalvert_aidan_HW4_stats.h"
#include "bayercalvert_aidan_HW4_helper.h"

int main (int argc, char *argv[])
    {
    //Read arguments, initialize application:
    // Retrieve subfield and area names from command-line arguments
    char *subfield = argv[4];
    char *areaName1 = argv[5];
    char *areaName2 = argv[6];

    // Parse the header file (specified as argv[2]) to initialize field definitions
    parseHeaderFile(argv[2]);

    // Initialize subfieldIndex to -1; this will store the index of the desired subfield
    int subfieldIndex = -1;

    // Loop through all defined fields to find the index matching the provided subfield name
    for (int i = 0; i < numFields; i++) {
        if (strcmp(fields[i].name, subfield) == 0) {
            // Found the matching subfield index set to variable
            subfieldIndex = i;
            // Once the subfield has been found exit
            break;
        }
    }
    
    // Copies the area names into the respective selectedAreas structure elements. This sets
    // up the areas for which detailed statistics will be computed
    strncpy(selectedAreas[0].name, areaName1, MAX_FIELD_NAME);
    strncpy(selectedAreas[1].name, areaName2, MAX_FIELD_NAME);

    calculateTotalRecords(argv[1]);
    allocateMemory();

    //**************************************************************
    //Time stamp start
    struct timespec startTime;
    struct timespec endTime;

    clock_gettime(CLOCK_REALTIME, &startTime);
    //**************************************************************
    // Threading:
    // Convert the command line argument to an integer to determine the number of threads to
    // spawn
    int numThreads = atoi(argv[3]);

    // Declare an array to hold thread identifiers (one per thread).
    pthread_t threads[numThreads];

    // Declare an array of ThreadArgs structures to pass the necessary data to each thread
    ThreadArgs threadArgs[numThreads];

    // Calculate how many records each thread will process by dividing the total number of
    // records evenly.
    int recordsPerThread = totalRecords / numThreads;

    // Loop over each thread index to initialize its argument structure and create the thread.
    for (int i = 0; i < numThreads; i++) {
        // For each thread, set up its arguments as defined:
        // dataFile: the data file name from the command line (argv[1])
        // startRecord: starting record index for this thread (i multiplied by recordsPerThread)
        // endRecord: ending record index ((i + 1) multiplied by recordsPerThread)
        // subFieldIndex: index of the subfield to filter or process, determined earlier
        threadArgs[i] = (ThreadArgs){argv[1], i * recordsPerThread, (i + 1) * recordsPerThread,
        subfieldIndex};

        // Create the thread that executes 'threadFunction' with its corresponding arguments
        pthread_create(&threads[i], NULL, threadFunction, &threadArgs[i]);
    }

    // After launching all threads, wait for each one to complete by joining them
    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Once all threads have finished processing, display the data pages
    printStatistics();

    //**************************************************************
    //Clock output
    clock_gettime(CLOCK_REALTIME, &endTime);
    time_t sec = endTime.tv_sec - startTime.tv_sec;
    long n_sec = endTime.tv_nsec - startTime.tv_nsec;
    if (endTime.tv_nsec < startTime.tv_nsec)
        {
        --sec;
        n_sec = n_sec + 1000000000L;
        }

    printf("Total Time was %ld.%09ld seconds\n", sec, n_sec);
    //**************************************************************


    // Free all declared memory to avoid memory issues or unexpected behavior
    freeMemory();
    return 0;
    }
