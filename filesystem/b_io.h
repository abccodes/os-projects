/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Arric Sekhon, Aidan Bayer-Calvert, Fabian Camarena Veliz, and Subhan Khan
* Student IDs:: 923090490, 922119403, 924100096, and 921938807
* GitHub-Name:: FlavyFabo
* Group-Name:: AAXF
* Project:: Basic File System
*
* File:: b_io.h
*
* Description:: This code is a basic setup that lets you use your 
*               own versions of open, read, write, seek, and close 
*               functions for working with files.
**************************************************************/

#ifndef _B_IO_H
#define _B_IO_H

#include <stdint.h>
#include <sys/types.h> 
#include <fcntl.h>

typedef int b_io_fd;

typedef struct b_fcb {
    int flags;               // File open flags (O_RDONLY, O_WRONLY, etc.)
    uint32_t dirSlot;        // Index of the file in its directory
    uint32_t dirStart;       // Starting block of the directory
    uint32_t dirBlocks;      // Number of blocks used by the directory
    uint32_t start_lba;      // First block of file data on disk
    uint32_t curr_lba;       // Currently cached LBA block
    off_t fileSize;          // Current size of the file
    off_t pos;               // Current position in the file
    char *buf;               // Pointer to the file's I/O buffer
    uint32_t buflen;         // Length of valid data in buf
} b_fcb;

// open or create a file within filesystem
b_io_fd b_open (char * filename, int flags);

// read raw bytes from the open file descriptor
int b_read (b_io_fd fd, char * buffer, int count);

// write raw bytes to the open file descriptor
int b_write (b_io_fd fd, char * buffer, int count);

// reposition the current read/write offset
int b_seek (b_io_fd fd, off_t offset, int whence);

// close the file descriptor and free resources
int b_close (b_io_fd fd);

#endif

