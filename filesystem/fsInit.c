/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Arric Sekhon, Aidan Bayer-Calvert, Fabian Camarena Veliz, and Subhan Khan
* Student IDs:: 923090490, 922119403, 924100096, and 921938807
* GitHub-Name:: FlavyFabo
* Group-Name:: AAXF
* Project:: Basic File System
*
* File:: fsInit.c
*
* Description:: Main driver for file system assignment. 
* File is where file system is started and initialized.
*
*
**************************************************************/

// Memory allocation/deallocation
#include <stdlib.h>

// low-leevel os calls
#include <unistd.h>

// definitions for data types
#include <sys/types.h>

// user outputs to console
#include <stdio.h>

// string and memory manipulation/management
#include <string.h>

// low-level disk operations
#include "fsLow.h"

// Contains file system interface  
#include "mfs.h"

// defines volume control block
#include "vcb.h"

// Definitions for directory managmenet
#include "directory.h"

// Free space management funcs
#include "freeSpace.h"

// Funcs for root dir creations
#include "rootdir.h"

// Global pointer to VCB. Allows various parts of system to access vcb metadata.
VCB *g_vcb = NULL;
FreeSpaceManager g_fsm = {0};

// Orchestrates the complete init of the file system. Verifies wether disk volume is formatted
// by checking sigiture in VCB. If volume is not formatted, it performs formatting tasks, init
// the VCB, setting up free space management system, and creating root dir. Implement if it has
// already been formatted steps in future milestones.
int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize) {
    // Test message, provide info about intended config.
    // printf("Initializing File System with %llu blocks with a block size of %llu\n", 
    //        (unsigned long long)numberOfBlocks, (unsigned long long)blockSize);
    
    // Allocate memory for the VCB. Allocating at size of one block to make sure it fits into
    // one disk block
    g_vcb = (VCB*) malloc(blockSize);

    // Seee if memory failed, if so return err and print err
    if (g_vcb == NULL) {
        fprintf(stderr, "ERROR: Failed to allocate memory for VCB\n");
        return -1;
    }

    // Retrieve existing VCB from disk by reading block 0. Used to check if volume was already
    // formatted
    LBAread(g_vcb, 1, 0);

    // Check the VCB signature to see if the volume is already formatted
    if (g_vcb->signature != VCB_SIGNATURE) {
        printf("Volume not formatted. Formatting now...\n");

        // Clear the VCB memory to eliminate left over data
        memset(g_vcb, 0, blockSize);

        // Initialize the VCB with fields that identify and configure the volume
        strncpy(g_vcb->volumeName, "CSC415_FS", sizeof(g_vcb->volumeName) - 1);
        g_vcb->signature = VCB_SIGNATURE;
        g_vcb->blockSize = blockSize;
        g_vcb->totalBlocks = numberOfBlocks;

        // VCB in block 0, bitmap starts at block 1
        g_vcb->bitmapStart = 1;
        // Compute bitmap size in bytes and blocks
        uint32_t bmpBytes    = (numberOfBlocks + 7) / 8;
        uint32_t bmpBlocks   = (bmpBytes + blockSize - 1) / blockSize;

        // First free data block starts after bitmap
        g_vcb->freeBlockStart = g_vcb->bitmapStart + bmpBlocks;
        

        // Initialize free space manager that creates and persists the free space map. Will
        // allocate the bitmap, mark the reserved blocks, and write the map to disk.
        // FreeSpaceManager fsm = {0};
        // initFreeSpaceManager(&fsm, g_vcb, (uint32_t)blockSize);
        initFreeSpaceManager(&g_fsm, g_vcb, (uint32_t)blockSize);

        // Create root directory (50 initial entries). Top level container for all files and
        // subdirectories. The creaDir func inits a directory with a certain amount of entries.
        directory_entry* root = createDir(50, NULL, &g_fsm);
        g_vcb->rootDirStart = root[0].location;

        // Calculate how many blocks the root directory occupies
        uint32_t rootBytes  = 50 * sizeof(directory_entry);
        uint32_t rootBlocks = (rootBytes + blockSize - 1) / blockSize;

        // Flush bitmap again to disk after root directory setup
        uint32_t bmpBlocksFinal = (g_fsm.bitmapSize + blockSize - 1) / blockSize;
        LBAwrite(g_fsm.bitmap, bmpBlocksFinal, g_vcb->bitmapStart);

        // Write updated VCB to disk to make sure all metadata including computed free space is
        // persistant.
        writeDir(root, rootBlocks, g_vcb->rootDirStart);
        g_vcb->freeBlockCount = numberOfBlocks - (g_vcb->rootDirStart + rootBlocks);
        LBAwrite(g_vcb, 1, 0);
        printf("Root directory initialized. VCB updated and written to disk.\n");

        // Free root directory memory to avoid memory issues
        free(root);
        free(g_fsm.bitmap);

        // Return with no errors
        return 0;
    } else {
        // If volume already formatted load metadata from disk. Implement in future milestones.
        printf("Volume already formatted. Loading file system metadata...\n");
        initFreeSpaceManager(&g_fsm, g_vcb, (uint32_t)blockSize);
    }

    // Return with no errors
    return 0;
}

// Exiting file system safely by freeing memory. Avoiding memory issues/leaks
void exitFileSystem () {
    printf("System exiting....\n");
    if (g_vcb != NULL) {
        free(g_vcb);
        g_vcb = NULL;
    }
}