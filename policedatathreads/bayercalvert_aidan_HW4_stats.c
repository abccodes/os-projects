/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Aidan Bayer-Calvert
* Student ID:: 922119403
* GitHub-Name:: abccodes
* Project:: Assignment 4 – Processing FLR Data with Threads
*
* File:: bayercalvert_aidan_HW4_stats.c
*
* Description:: Implements stats computations, including calculating metrics such as, mean,
* median, quartiles, IQR, and SD for event response times. Structures and formats the outputs.
*
**************************************************************/

#include "bayercalvert_aidan_HW4_stats.h"

// Standard input-output functions
#include <stdio.h>
// Standard library functions like memory management
#include <stdlib.h>
// String manipulation functions
#include <string.h>
// Mathematical functions like pow() and sqrt()
#include <math.h>

// Compare two ints for stats calculations
int compare(const void *a, const void *b) {
    // Derefernce the void pointers and convert them to int pointers. Then access the vals.
    int firstPointerValue = *(int *)a;
    int secondPointerValue = *(int *)b;

    // Calculate different between two ints. if firstval>secondval result is - , firsVal comes 
    // first. If opposite secondVal is first. If equal result is zero, no change in order.
    int result = firstPointerValue - secondPointerValue;

    // Returning the result to use in other methods
    return result;
}

// Computes stats such as min, max, quartiles, IQR, mean, and standard deviation
void computeStats(int *values, int count, StatResults *result) {
    // Allocate memory for a copy of the values array to avoid modifying original
    int *sorted = malloc(count * sizeof(int));

    // Check if memory allocation was sucessful, if not print error and exit
    if (!sorted) {
        perror("computeStats: failed to malloc sorted array");
        exit(EXIT_FAILURE);
    }

    // Copy input array to sorted values array and sort values to help with calculations.
    for (int i = 0; i < count; i++) {
        sorted[i] = values[i];
    }
    qsort(sorted, count, sizeof(int), compare);

    // Calculate min val by accessing first element
    int minVal = sorted[0];
    // Calculate max val by accessing last element
    int maxVal = sorted[count - 1];

    // Calculate sum by iterating over all values and adding to sum variable
    double sum = 0.0;
    for (int i = 0; i < count; i++) {
        sum += sorted[i];
    }

    // Compute media, middle point in sorted array
    int median = sorted[count / 2];

    // compute first quartile or 25th percent
    int q1 = sorted[count / 4];

    // Compute third quartile of 75th percent
    int q3 = sorted[(count * 3) / 4];

    // Compute inter quartile range or 75 percent - 25 percent
    int iqr = q3 - q1;

    // Compute lower bound to identify outliers using IQR 1.5 outlier rule. Make sure the the
    // lower bound does not go below dataets min value.
    int outlierThresholdLower = q1 - (int)(1.5 * iqr);
    int lowerBound = (outlierThresholdLower > minVal) ? outlierThresholdLower : minVal;

    // Computer upper bound. Make sure doesnt go above datasets max val.
    int outlierThresholdUpper = q3 + (int)(1.5 * iqr);
    int upperBound = (outlierThresholdUpper < maxVal) ? outlierThresholdUpper : maxVal;

    // Compute average or mean
    double mean = sum / count;

    // Variance. How spread out the values are.
    double variance = 0.0;
    for (int i = 0; i < count; i++) {
        variance += pow((double)sorted[i] - mean, 2);
    }

    // Compute standard deviation
    double stddev = sqrt(variance / count);

    // Store computed results in results struct
    result->minVal = minVal;
    result->maxVal = maxVal;
    result->median = median;
    result->q1 = q1;
    result->q3 = q3;
    result->iqr = iqr;
    result->lowerBound = lowerBound;
    result->upperBound = upperBound;
    result->mean = mean;
    result->stddev = stddev;

    // Free allocated memory
    free(sorted);
}

// Aggregates time difference values across all events and computes overall statistics
void computeAggregatedStatistics(const char *timeCategory) {
    // Initialize counters for to keep track of total event occurrences and valid time entries
    int totalCount = 0;
    int totalEntries = 0;

    // Allocate memory to store time differences
    int *fullTimesArray = (int *)malloc(MAX_TIME_ENTRIES * MAX_EVENTS * sizeof(int));

    // Check if memory allocation was a success, if noto print error and return
    if (!fullTimesArray) {
        perror("Memory allocation failed for aggregated times");
        return;
    }
    
    // Iterate through all recorded even types to collect time values
    for (int i = 0; i < eventCount; i++) {

        // Calc total event occurences
        totalCount += eventStats[i].count;
        
        // Iterate over all time index
        for (int j = 0; j < eventStats[i].timeIndex; j++) {

            // Default val to not do any time difference if none exist
            int value = -1;

            // Find out which time difference array to extract vals based on category in
            // func arguments
            if (strcmp(timeCategory, "dispatch_received") == 0)
                value = eventStats[i].dispatch_received[j];
            else if (strcmp(timeCategory, "onscene_enroute") == 0)
                value = eventStats[i].onscene_enroute[j];
            else if (strcmp(timeCategory, "onscene_received") == 0)
                value = eventStats[i].onscene_received[j];

            // Store valid time difference in array
            if (value != -1) {
                fullTimesArray[totalEntries++] = value;
            }
        }
    }
    
    // Compute results for time values
    StatResults sr;
    computeStats(fullTimesArray, totalEntries, &sr);
    
    // Print overall aggregated stats and label as the total
    printf("%-25s | %6d | %6d | %6d | %6d | %6d | %8.2f | %6d | %6d | %6d | %6d | %8.2f\n",
           "TOTAL", totalCount,
           sr.minVal, sr.lowerBound, sr.q1, sr.median, sr.mean,
           sr.q3, sr.upperBound, sr.maxVal, sr.iqr, sr.stddev);
    
    // Free all malloced memory
    free(fullTimesArray);
}

// Computes and prints statistics for each unique event type in the dataset
void computeTotalStatisticsByType(const char *timeCategory) {
    // Print the column headers to be able to nicely display in console
    printf(
    "\n%-25s | %-6s | %-6s | %-6s | %-6s | %-6s | %-8s | %-6s | %-6s | %-6s | %-6s | %-6s\n",
     "Call Type", "Count", "Min", "LB", "Q1", "Med", "Mean", "Q3", "UB", "Max", "IQR", "Std Dev"
    );

    // Compute overall stats for all events
    computeAggregatedStatistics(timeCategory);

    // Iterate over each event type and compute its stats
    for (int i = 0; i < eventCount; i++) {

        // Skip curr event if it has no time vals
        if (eventStats[i].timeIndex == 0)
            continue;

        // Determine which array to use based on provided args category given
        int *timeValues = NULL;
        if (strcmp(timeCategory, "dispatch_received") == 0)
            timeValues = eventStats[i].dispatch_received;
        else if (strcmp(timeCategory, "onscene_enroute") == 0)
            timeValues = eventStats[i].onscene_enroute;
        else if (strcmp(timeCategory, "onscene_received") == 0)
            timeValues = eventStats[i].onscene_received;
        else {
            // If invalid just skip return
            return;
        }

        // Compute stats for current event type
        StatResults sr;
        computeStats(timeValues, eventStats[i].timeIndex, &sr);
        
        // Print computed stats for specific event type
        printf("%-25s | %6d | %6d | %6d | %6d | %6d | %8.2f | %6d | %6d | %6d | %6d | %8.2f\n",
               eventStats[i].eventType,
               eventStats[i].count,
               sr.minVal, sr.lowerBound, sr.q1, sr.median, sr.mean,
               sr.q3, sr.upperBound, sr.maxVal, sr.iqr, sr.stddev);
    }
}

// Computes and prints statistical data for a specific area, such as a police district.
void computeAreaAggregatedStatistics(AreaData *area, const char *timeCategory) {  

    // Retrieve total time entries for area
    int overallCount = area->timeIndex;
    
    // Allocate memory to store all time vals, needed for stats calculations
    int *overallValues = (int*)malloc(overallCount * sizeof(int));

    // Check if memory allocation was a success, if not print err and return
    if (!overallValues) {
        perror("Failed to allocate memory for overallValues in computeAreaAggregatedStatistics");
        return;
    }
    
    // Determine correct time category using category from args
    int *areaTimes = NULL;
    if (strcmp(timeCategory, "dispatch_received") == 0)
        areaTimes = area->dispatch_received;
    else if (strcmp(timeCategory, "onscene_enroute") == 0)
        areaTimes = area->onscene_enroute;
    else if (strcmp(timeCategory, "onscene_received") == 0)
        areaTimes = area->onscene_received;
    else {
        fprintf(stderr, "Invalid time category '%s' for area statistics.\n", timeCategory);
        free(overallValues);
        return;
    }
    
    // Copy time values from the selected area’s dataset into the allocated memory.
    memcpy(overallValues, areaTimes, overallCount * sizeof(int));
    
    // Compute overall stats for single selected area
    StatResults srOverall;
    computeStats(overallValues, overallCount, &srOverall);
    
    // Print the column headers to display nicely on console
    printf(
    "\n%-25s | %-6s | %-6s | %-6s | %-6s | %-6s | %-8s | %-6s | %-6s | %-6s | %-6s | %-6s\n",
     "Call Type", "Count", "Min", "LB", "Q1", "Med", "Mean", "Q3", "UB", "Max", "IQR", "Std Dev"
    );

    // Prrint calculated stats to be able to see in console
    printf(
        "%-25s | %6d | %6d | %6d | %6d | %6d | %8.2f | %6d | %6d | %6d | %6d | %8.2f\n",
           "TOTAL", overallCount,
           srOverall.minVal, srOverall.lowerBound, srOverall.q1, srOverall.median,
           srOverall.mean, srOverall.q3, srOverall.upperBound, srOverall.maxVal,
           srOverall.iqr, srOverall.stddev
        );

    // Free the allocated memory to prevent leaks or unexpected behavior
    free(overallValues);

    // Iterate over each recorded event type in this area and compute its stats.
    for (int i = 0; i < area->totalEvents; i++) {
        EventData *ev = &area->areaEvents[i];

        // Skip event type if no recorded time vals
        if (ev->timeIndex == 0)
            continue;

        // Determine which time category for event
        int *timeValues = NULL;
        if (strcmp(timeCategory, "dispatch_received") == 0)
            timeValues = ev->dispatch_received;
        else if (strcmp(timeCategory, "onscene_enroute") == 0)
            timeValues = ev->onscene_enroute;
        else
            timeValues = ev->onscene_received;

        // Compute stats for current event type
        StatResults sr;
        computeStats(timeValues, ev->timeIndex, &sr);
        
        // Print the row for this event type.
        printf("%-25s | %6d | %6d | %6d | %6d | %6d | %8.2f | %6d | %6d | %6d | %6d | %8.2f\n",
               ev->eventType,
               ev->count,
               sr.minVal, sr.lowerBound, sr.q1, sr.median, sr.mean,
               sr.q3, sr.upperBound, sr.maxVal, sr.iqr, sr.stddev);
    }
}

//Organizes and prints statistical summaries, including event-level and area-level statistics
void printStatistics() {
    // Print total statistics for all events
    printf("\n---------------------------\n");
    printf("Total - Dispatch Time - Received Time\n");
    printf("---------------------------\n");
    computeTotalStatisticsByType("dispatch_received");

    printf("\n---------------------------\n");
    printf("Total - OnScene Time - Enroute Time\n");
    printf("---------------------------\n");
    computeTotalStatisticsByType("onscene_enroute");

    printf("\n---------------------------\n");
    printf("Total - OnScene Time - Received Time\n");
    printf("---------------------------\n");
    computeTotalStatisticsByType("onscene_received");

    // Print area-specific statistics for the first selected area
    printf("\n---------------------------\n");
    printf("%s - Dispatch Time - Received Time\n", selectedAreas[0].name);
    printf("---------------------------\n");
    computeAreaAggregatedStatistics(&selectedAreas[0], "dispatch_received");

    printf("\n---------------------------\n");
    printf("%s - OnScene Time - Enroute Time\n", selectedAreas[0].name);
    printf("---------------------------\n");
    computeAreaAggregatedStatistics(&selectedAreas[0], "onscene_enroute");

    printf("\n---------------------------\n");
    printf("%s - OnScene Time - Received Time\n", selectedAreas[0].name);
    printf("---------------------------\n");
    computeAreaAggregatedStatistics(&selectedAreas[0], "onscene_received");

    // Print area-specific statistics for the second selected area
    printf("\n---------------------------\n");
    printf("%s - Dispatch Time - Received Time\n", selectedAreas[1].name);
    printf("---------------------------\n");
    computeAreaAggregatedStatistics(&selectedAreas[1], "dispatch_received");

    printf("\n---------------------------\n");
    printf("%s - OnScene Time - Enroute Time\n", selectedAreas[1].name);
    printf("---------------------------\n");
    computeAreaAggregatedStatistics(&selectedAreas[1], "onscene_enroute");

    printf("\n---------------------------\n");
    printf("%s - OnScene Time - Received Time\n", selectedAreas[1].name);
    printf("---------------------------\n");
    computeAreaAggregatedStatistics(&selectedAreas[1], "onscene_received");
}