/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Arric Sekhon, Aidan Bayer-Calvert, Fabian Camarena Veliz, and Subhan Khan
* Student IDs:: 923090490, 922119403, 924100096, and 921938807
* GitHub-Name:: FlavyFabo
* Group-Name:: AAXF
* Project:: Basic File System
*
* File:: freeSpace.h
*
* Description:: This code creates a system to keep track of which parts 
*               of the storage are free or used by using a bitmap, and 
*               it includes functions to set it up, give out space, and 
*               free it when it's no longer needed.
*
**************************************************************/

#ifndef FREESPACE_H
#define FREESPACE_H

#include <stdint.h>
#include "vcb.h"

// Structure for managing free space using a bitmap
typedef struct FreeSpaceManager {
    // Array representing the bitmap
    uint8_t *bitmap;

    // Total number of blocks in the volume
    uint32_t totalBlocks;

    // Size of the bitmap in bytes (totalBlocks/8 rounded up)
    uint32_t bitmapSize;

    // Block size to 512bytes (as specified in directions)
    uint32_t blockSize;

} FreeSpaceManager;

// Now that the type is known, we can declare the external variable
extern FreeSpaceManager g_fsm;

// Initialize the free space system. Allocates memory for the bitmap and mark the blocks used. 
// It writes the initialized bitmap to disk at the block indicated by VCB.bitmapStart.
void initFreeSpaceManager(FreeSpaceManager *fsm, VCB *vcb, uint32_t blockSize);

// Gets the number of blocks to allocate and a pointer to an array to store their indices.
// Searches for 'count' consecutive free blocks, marks them as used, and writes the updated
// bitmap back to disk.
int allocateBlocks(FreeSpaceManager *fsm, uint32_t count, uint32_t *allocatedBlocks);

// Takes a list of block numbers and a count, and marks each block as free in the
// bitmap. After updating the bitmap, it writes the updated bitmap back to disk. Releases/frees
// set of blocks.
int releaseBlocks(FreeSpaceManager *fsm, uint32_t *blockList, uint32_t count);

// Allocates a single block and returns its number, or UINT32_MAX if failed
uint32_t alloc_one_block(FreeSpaceManager *fsm);

#endif
