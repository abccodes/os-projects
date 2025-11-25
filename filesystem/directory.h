/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Arric Sekhon, Aidan Bayer-Calvert, Fabian Camarena Veliz, and Subhan Khan
* Student IDs:: 923090490, 922119403, 924100096, and 921938807
* GitHub-Name:: FlavyFabo
* Group-Name:: AAXF
* Project:: Basic File System
*
* File:: directory.h
*
* Description:: This code sets up a structure to store info about 
*               files and folders, like their name, size, location 
*               on disk, and when they were created or changed, so 
*               the system can keep track of everything in a directory.
*
**************************************************************/

#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <stdint.h>
// for off_t type
#include <sys/types.h>
// for time_t type
#include <time.h>

// Defining if it directory or file
enum FileType { FT_FILE, DIRECTORY };

typedef struct directory_entry {
    char name[256];           // Directory or file name 
    uint32_t file_no;         // Unique identifier 
    enum FileType fileType;   // FILE or DIRECTORY
    uint32_t location;        // Starting block number on disk
    off_t fileSize;           // File size in bytes 
    uint8_t  inUse;            // 1 = used, 0 = free
    uint32_t ownerID;         // User ID of the owner
    time_t created_at;        // Creation timestamp
    time_t modified_at;       // Last modified timestamp
    time_t accessed_at;       // last accessed timestamp
} directory_entry;


#endif // DIRECTORY_H