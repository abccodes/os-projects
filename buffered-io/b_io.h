/**************************************************************
* Class::  CSC-415-03 Spring 2024
* Name::   Aidan Bayer-Calvert
* Student ID:: 922119403
* GitHub-Name:: abccodes
* Project:: Assignment 5 – Buffered I/O read
*
* File:: b_io.h
*
* Description:: Definitino of b_io_fd type and the prototypes
*               of the functions accessible by user applications
*
**************************************************************/

#ifndef _B_IO_H
#define _B_IO_H

// File descriptor is integer index inside the FCB array
typedef int b_io_fd;

// Opens the specified file by retrieving its metadata and initializing its file control block.
// Returns a file descriptor on success, or -1 on failure if there is an unexpected issue
b_io_fd b_open (char * filename, int flags);

// Reads up to count bytes from the file corresponding to the file descriptor into the provided
// buffer and returns the number of bytes read or 0 if the end of file is reached
int b_read (b_io_fd fd, char * buffer, int count);

// Closes the file associated with the given file descriptor by freeing allocated resources and
// marking the file control block as available, returns 0 as a sucess
int b_close (b_io_fd fd);

#endif

