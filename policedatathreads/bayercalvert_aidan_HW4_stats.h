/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Aidan Bayer-Calvert
* Student ID:: 922119403
* GitHub-Name:: abccodes
* Project:: Assignment 4 – Processing FLR Data with Threads
*
* File:: bayercalvert_aidan_HW4_stats.h
*
* Description:: Declares functions for computing and formatting stats data.
*
**************************************************************/

#ifndef STATS_H
#define STATS_H

#include "bayercalvert_aidan_HW4_processing.h"

// Computes stats such as min, max, quartiles, IQR, mean, and standard deviation
void computeStats(int *values, int count, StatResults *result);
// Aggregates time difference values across all events and computes overall statistics
void computeAggregatedStatistics(const char *timeCategory);
// Computes and prints statistics for each unique event type in the dataset
void computeTotalStatisticsByType(const char *timeCategory);
// Computes and prints statistical data for a specific area, such as a police district.
void computeAreaAggregatedStatistics(AreaData *area, const char *timeCategory);
//Organizes and prints statistical summaries, including event-level and area-level statistics
void printStatistics();

#endif