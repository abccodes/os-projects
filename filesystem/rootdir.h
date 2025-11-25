/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Arric Sekhon, Aidan Bayer-Calvert, Fabian Camarena Veliz, and Subhan Khan
* Student IDs:: 923090490, 922119403, 924100096, and 921938807
* GitHub-Name:: FlavyFabo
* Group-Name:: AAXF
* Project:: Basic File System
*
* File:: rootdir.h
*
* Description:: Function declarations for root directory initialization
*
**************************************************************/

#ifndef ROOTDIR_H
#define ROOTDIR_H

#include "directory.h"
#include "freeSpace.h"


// Creates a new directory with specified number of entries
// directory_entry* createDir(int initialNumEntries, directory_entry* parent);
directory_entry* createDir(int initialNumEntries, directory_entry* parent, FreeSpaceManager* fsm);

// Writes a directory to disk using LBA write.
void writeDir(directory_entry* parent, int blocksNeeded, int startBlock);

#endif
