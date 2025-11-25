/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Aidan Bayer-Calvert
* Student ID:: 922119403
* GitHub-Name:: abccodes
* Project:: Assignment 4 – Processing FLR Data with Threads
*
* File:: bayercalvert_aidan_HW4_processing.h
*
* Description:: Declares the area/event/header functions, global variables, and data structures
* Ensures other files have access to necessary global data and variable definitions.
*
**************************************************************/

#ifndef PROCESSING_H
#define PROCESSING_H

// Include pthread for mutex
#include <pthread.h>

// Constants defining system limits:
// These definitions are large arbitrary values to ensure memory saftey/efficiency.
// Maximum number of fields parsed from the header file
#define MAX_FIELDS 50
// Maximum allowed field name length
#define MAX_FIELD_NAME 50
// Buffer size for reading the header file
#define BUFFER_SIZE 256
// Maximum size of a single record.
#define MAX_RECORD_SIZE 1024
// Maximum number of unique event types that can be tracked
#define MAX_EVENTS 5000
// Maximum number of records processed (used for memory allocation)
#define MAX_RECORDS 5000
// Maximum number of time measurements that can be stored
#define MAX_TIME_ENTRIES 3000
// Number of records read per batch to optimize file I/O
#define RECORDS_PER_BATCH 2000

// Variables for tracking parsed data:
// Tracks the number of unique event types recorded
extern int eventCount;
// Stores the number of fields defined in the header file
extern int numFields;
// Stores the total size of a single record, computed from field widths
extern int recordSize;
// Stores the total number of records in the data file (computed dynamically)
extern int totalRecords;

// Indices of key timestamp fields (initialized to -1 to indicate "not found"). These values
// are assigned dynamically when parsing the header file. Index for "received_datetime" (when
// a call was first received).
extern int receivedIndex;
// Index for "dispatch_datetime" (when a unit was dispatched)
extern int dispatchIndex;
// Index for "onscene_datetime" (when a unit arrived on the scene)
extern int onsceneIndex;
// Index for "enroute_datetime" (when a unit began traveling to the scene)
extern int enrouteIndex;
// Index for "call_type_final_desc" (the final categorized event type)
extern int eventTypeIndex;

// A mutex to prevent race conditions when threads update shared statistics
extern pthread_mutex_t statsMutex;

// Data structure to store statistical data for each event type
typedef struct {
    // Stores the event type name
    char eventType[MAX_FIELD_NAME];
    // Number of times this event type occurs
    int count;
    // Array storing time differences between dispatch and received
    int dispatch_received[MAX_TIME_ENTRIES];
    // Array storing time differences between on-scene and enroute
    int onscene_enroute[MAX_TIME_ENTRIES];
    // Array storing time differences between on-scene and received
    int onscene_received[MAX_TIME_ENTRIES];
    // Tracks the number of recorded time entries
    int timeIndex;
} EventData;

// Data structure to store statistical breakdowns for specific geographic areas
typedef struct {
    // Stores the name of the area (police district/neighborhood)
    char name[MAX_FIELD_NAME];
    // Total number of events recorded in this area
    int totalEvents;
    // Time differences for dispatch-received calls
    int dispatch_received[MAX_TIME_ENTRIES];
    // Time differences for on-scene-enroute calls
    int onscene_enroute[MAX_TIME_ENTRIES];
    // Time differences for on-scene-received calls
    int onscene_received[MAX_TIME_ENTRIES];
    // Tracks the number of stored time values
    int timeIndex;
    // Stores breakdown of events specific to this area
    EventData *areaEvents;
} AreaData;

// Data structure to store metadata for each field defined in the header file
typedef struct {
    // Number of characters allocated for this field in a record
    int width;
    // Name of the field (parsed from the header file)
    char name[MAX_FIELD_NAME];
} FieldInfo;

// Struct to store arguments passed to worker threads. Each thread processes a subset of records
// from the data file.
typedef struct {
    // File path to the dataset being processed
    const char *dataFile;
    // Starting index of records assigned to this thread
    int startRecord;
    // Ending index of records assigned to this thread
    int endRecord;
    // Index of the subfield being analyzed (e.g., police district)
    int subFieldIndex;
} ThreadArgs;

// Struct to store statistical results after computation. This is used to store aggregated
// statistical values for reporting purposes.
typedef struct {
    // Minimum value in the dataset
    int minVal;
    // Maximum value in the dataset
    int maxVal;
    // Median value (middle of sorted data)
    int median;
     // First quartile (25th percentile)
    int q1;
    // Third quartile (75th percentile)
    int q3;
    // Interquartile range (q3 - q1)
    int iqr;
    // Lower bound (q1 - 1.5 * IQR, but no less than minVal)
    int lowerBound;
    // Upper bound (q3 + 1.5 * IQR, but no greater than maxVal)
    int upperBound;
    // Mean (average value)
    double mean;
    // Standard deviation (measure of data variability)
    double stddev;
} StatResults;

// Gloval array to store statistics for different event types
extern EventData *eventStats;

// Global array to store statistics for two selected areas specified in CL arguments
extern AreaData selectedAreas[2];

// Global array to store field definitions extracted from the header file
extern FieldInfo fields[MAX_FIELDS];

// Parse the header file and extract field names and widths
void parseHeaderFile(const char *headerFile);

// Calculates totalr ecords used for threading calculations
void calculateTotalRecords(const char *dataFile);

// TESTING FUNC: Displays header info
// void printHeaderInfo();

#endif