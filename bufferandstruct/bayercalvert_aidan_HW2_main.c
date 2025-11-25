/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Aidan Bayer-Calvert
* Student ID:: 922119403
* GitHub Name:: abccodes
* Project:: Assignment 2 – Buffer and Structures
*
* File:: bayercalvert_aidan_HW2_main.c
*
* Description:: This file sets up a personalInfo data structure using command line inputs, 
* then stores and processses strings by using a buffer. Finally, it checks and returns the
* results.
**************************************************************/

// Import assignment 2 file to use personalInfo struct and function definitions.
#include "assignment2.h"

// Include library that contains printing to console funcitonality.
#include <stdio.h>

// Include library that contains malloc opperations.
#include <stdlib.h>

// Include library that contains opperation for finding length of string and for the function
// that copies a specified number of bytes from a source memory location to destination 
// memory.
#include <string.h>

// Allows the program to access inputs from command line.
int main(int argc, char *argv[]) {
    // Determining the size of the personalInfo struct in bytes to ensure the program can
    // allocate the correct amount of memory. Saving this amount to a variable.
    int personalInfoByteSize = sizeof(personalInfo);

    // Instantiating personalinfo struct called info from assignment 2 header file by 
    // allocating memory. Passing in previosly caculated amount of bytes needed for 
    // personalInfo memory.
    personalInfo *info = malloc(personalInfoByteSize);

    // Retrieve first name from the first command-line argument and set to a variable.
    char *firstNameInput = argv[1];

    // Retrieve last name from the second command-line argument and set to a variable.
    char *lastNameInput = argv[2];

    // Assigning the first and last names gathered to the info struct.
    info->firstName = firstNameInput;
    info->lastName = lastNameInput;

    // Assigning hardcoded studentid int to studentID field in info struct.
    info->studentID = 922119403;

    // Assigning SENIOR enum value to level field in info struct.
    info->level = SENIOR;

    // Populating at least 6 coding languages into languages field in info struct.
    info->languages = KNOWLEDGE_OF_JAVA | KNOWLEDGE_OF_JAVASCRIPT
    | KNOWLEDGE_OF_SWIFT | KNOWLEDGE_OF_PYTHON | KNOWLEDGE_OF_MIPS_ASSEMBLER 
    | KNOWLEDGE_OF_CPLUSPLUS;

    // Initialize a variable to track the position where inputted text ends.
    int nullPos = 0;

    // Loop through the input text (third command-line argument) to copy it into the message
    // field of the info struct. The loop ensures no more than 99 characters are copied to
    // prevent exceeding the size limit.
    for(int i = 0; i < 99; i++) {

        // If the input text ends, stop copying and record the ending position.
        if(argv[3][i] == '\0') {
            nullPos = i;
            break;
        }

        // Add character to info message array from current character in third command line
        // argument.
        info->message[i] = argv[3][i];

        // Update the position for marking the end of the text to the next spot.
        nullPos = i + 1;
    }

    // Mark the end of the text in the info container which ensures that the destination text
    // is recognized as complete, regardless of whether it was fully copied or not.
    info->message[nullPos] = '\0';

    // // Tests to verify personalInfo info is correct by printing all attributes to console.
    // printf("first name: %s\n", info->firstName);
    // printf("last name: %s\n", info->lastName);
    // printf("student id: %d\n", info->studentID);
    // printf("grade: %d\n", info->level);
    // printf("languages: %d\n", info->languages);
    // printf("message: %s\n", info->message);

    // Validate the populated personalInfo info structure. A return value of 0 confirms that
    // the operation succeeded and a return value of 1 indicates it failed.
    int writePersonalInfoResult = writePersonalInfo(info);

    // Check if writing personal info was sucessful or unsucessful by printing out to console
    // sucess or fail based on personalInfoResult variable, 1=fail or 0=pass.
    if (writePersonalInfoResult == 0) {
        printf("Success, write personal info succeeded.");
    } else {
        printf("Error, write personal info failed.");
    }

    // Allocate BLOCK_SIZE amount of memory(256 Bytes) for a buffer which stores data before
    // committing it. 
    char *buffer = malloc(BLOCK_SIZE);

    // Retrieves the first string to process from getNext(). Every call to getNext() or in
    // other words, every new string receieved, will be stored in this variable.
    const char *nextString = getNext();

    // Tracks current position in the buffer to ensure the correct placement of data and
    // prevent exceeding buffer size.
    int currBufferPosition = 0;

    while(nextString != NULL) {

        // Calculating the length of the nextstring, and setting to variable which holds the
        // number of bytes in the string. Each character in a string is 1 byte so the length
        // of the string is equivalent to number of bytes.
        int stringBytes = strlen(nextString);

        // Tracking the current position or how much of the current string has been processed
        // and copied into the buffer. Allows for partial copying if full string does not fit
        // in buffer.
        int stringPosition = 0;

        // Continueing to process current string until all of its bytes have been copied into
        // the buffer.
        while (stringPosition < stringBytes) {

            // Calculation of how much space remains in the buffer. Used to prevent overflow, 
            // and ensure data is only copied to BLOCK_SIZE
            int spaceRemaining = BLOCK_SIZE - currBufferPosition;

            // Calculation to determine how many bytes of the string can fit into the 
            // remaining buffer space. If the remaining string is larger than the buffer 
            // space, copy only what fits into the buffer space. If the remaining string is
            // not larger than buffer space, copy the entire string. 
            int copyLength;
            if (stringBytes - stringPosition > spaceRemaining) {
                copyLength = spaceRemaining;
            } else {
                copyLength = stringBytes - stringPosition;
            }

            // Copying the raw memory data of the portion the string calculated from above
            // into the buffer by starting at the current buffer position. 
            memcpy(buffer + currBufferPosition, nextString + stringPosition, copyLength);

            // Updating the buffer position to reflect added data.
            currBufferPosition += copyLength;

            // Updating the string position to indicate how much of the string has been 
            // processed.
            stringPosition += copyLength;

            //If the buffer is full after adding data, commit it for further processing.
            if (currBufferPosition == BLOCK_SIZE) {
                commitBlock(buffer);
                currBufferPosition = 0;
            }
        }

        // Debugging line to print out all strings.
        // printf("%s\n", nextString);

        // Update next string to the next string from getNext().
        nextString = getNext();
    }

    // If there is any remaining data in the buffer after processing all strings, commit block
    // so no data is accidentally lost.
    if (currBufferPosition > 0) {
        commitBlock(buffer);
    }

    // Calls a function, checkIt, to verify the correctness of the processed data. This returns a
    // result that we save in a variable to return from the main function.
    int checkItResult = checkIt();

    // Free the allocated memory for the buffer to avoid memory issues.
    free(buffer);

    // Free the allocated memory for the info structure to avoid memory issues.
    free(info);

    // Return the results of checkit.
    return checkItResult;

}