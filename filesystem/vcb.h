/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Arric Sekhon, Aidan Bayer-Calvert, Fabian Camarena Veliz, and Subhan Khan
* Student IDs:: 923090490, 922119403, 924100096, and 921938807
* GitHub-Name:: FlavyFabo
* Group-Name:: AAXF
* Project:: Basic File System
*
* File:: vcb.h
*
* Description:: This code defines a structure that holds important 
*               information about the file system, like its name, size,
*               where the free space and root directory start, so the 
*               system knows how to manage and organize the storage.
*
**************************************************************/

#ifndef VCB_H
#define VCB_H

#include <stdlib.h>

// Signature for the VCB
#define VCB_SIGNATURE 0xDEADFACEDEADFACEULL

typedef struct VCB{
    char      volumeName[64];  // Volume name
    uint64_t  signature;       // Signature
    uint64_t  blockSize;       // Block size
    uint64_t  totalBlocks;     // Total number of blocks
    uint64_t  bitmapStart;     // Start of bitmap
    uint64_t  freeBlockStart;  // Start of free block list
    uint64_t  rootDirStart;    // Start of root directory
    uint64_t  freeBlockCount;  // Number of free blocks
} VCB;

extern VCB* g_vcb;
#endif 