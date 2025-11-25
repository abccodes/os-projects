/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Arric Sekhon, Aidan Bayer-Calvert, Fabian Camarena Veliz, and Subhan Khan
* Student IDs:: 923090490, 922119403, 924100096, and 921938807
* GitHub-Name:: FlavyFabo
* Group-Name:: AAXF
* Project:: Basic File System
*
* File:: rootdir.c
*
* Description:: initialization of the root directory
*
* 
*
**************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "fsLow.h"
#include "mfs.h"
#include "vcb.h"
#include "freeSpace.h"
#include "rootdir.h"

//global var that assigns a unique identifier to each file
static uint32_t next_file_no = 1; 

/*this function initializes a new directory, setting up the "." and ".." entries
Checks if parent is NULL and points to itself (root directory). It also allocates memory, disk blocks,
and writes the directory to disk.
*/
directory_entry* createDir(int initialNumEntries, directory_entry* parent, FreeSpaceManager* fsm){
    //determine # of DE's and blocks
	int initialBytesNeeded = initialNumEntries * sizeof(directory_entry);
	int blocksNeeded = (initialBytesNeeded + (g_vcb->blockSize - 1)) / g_vcb->blockSize;
    int bytesAlloc = blocksNeeded * g_vcb->blockSize;
    int actualEntries = bytesAlloc / sizeof(directory_entry);
    int actualBytesNeeded = actualEntries * sizeof(directory_entry);

    //new Directory entry made
    directory_entry *dir = calloc(1, bytesAlloc);

    // Avoid proceeding with null buffer; log to aid debugging
    if (!dir) {
        fprintf(stderr, "Failed to allocate memory for directory entries.\n");
        return NULL;
    }

    // Prepare to allocate the actual disk blocks that will hold this directory
    u_int32_t* blockList = calloc(blocksNeeded, sizeof(u_int32_t));
    if (!blockList) {
        fprintf(stderr, "Failed to allocate memory for block list.\n");
        free(dir);
        return NULL;
    }

    // Reserve a contiguous run of blocks for the directory in the free-space bitmap
    if (allocateBlocks(fsm, blocksNeeded, blockList) < 0) {
        fprintf(stderr, "Failed to allocate blocks for directory.\n");
        free(blockList);
        free(dir);
        return NULL;
    }

    // The first block in our list becomes the starting block for this dir
    uint32_t startBlock = blockList[0];


    // Capture current time once so all timestamps match exactly
    time_t now = time(NULL);

    // Initialize the "." entry to point to the directory itself
    // always presenet
    strcpy(dir[0].name, ".");
    // self block pointer
    dir[0].location = startBlock;

    // full directory size
    dir[0].fileSize = actualBytesNeeded;

    // mark as directory
    dir[0].fileType = DIRECTORY;

    // creation time
    dir[0].created_at = now;

    // last modification time
    dir[0].modified_at = now;

    // last access time
    dir[0].accessed_at = now;

    // current user owns it
    dir[0].ownerID = getuid();

    // mark entry active
    dir[0].inUse = 1;

    // unique identifier
    dir[0].file_no = next_file_no++;

    // If no parent passed (root initialization), make parent refer to self
    if(parent == NULL) {
        parent = dir;
    } 

    // Initialize .. entry to point to parent
    // parent marker
    strcpy(dir[1].name, "..");

    // parent’s starting block
    dir[1].location = parent[0].location;

    // parent’s size
    dir[1].fileSize = parent[0].fileSize;

    // remains a directory
    dir[1].fileType = DIRECTORY;

    // mirror timestamp for consistency
    dir[1].created_at = now;
    dir[1].modified_at = now;
    dir[1].accessed_at = now;

    // same owner as child
    dir[1].ownerID = getuid();
    dir[1].inUse = 1;
    dir[1].file_no = next_file_no++;

    // Write to disk
    writeDir(dir, blocksNeeded, startBlock);

    uint32_t blkSizeOnDisk = fsm->blockSize;
    uint32_t bmpBlocks     = (fsm->bitmapSize + blkSizeOnDisk - 1)/blkSizeOnDisk;
    LBAwrite(fsm->bitmap, bmpBlocks, g_vcb->bitmapStart);

    // Free the block list after use);
    free(blockList);

    return dir;
}

// Write the directory entries back to disk. Just calling LBA write, can add additional error
// checking in this 'middleware' function at a later point.
void writeDir(directory_entry* parent, int blocksNeeded, int startBlock){
    LBAwrite(parent, blocksNeeded, startBlock);  
}