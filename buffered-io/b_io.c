/**************************************************************
* Class::  CSC-415-03 Spring 2024
* Name:: Aidan Bayer-Calvert
* Student ID:: 922119403
* GitHub-Name:: abccodes
* Project:: Assignment 5 – Buffered I/O read
*
* File:: b_io_c
*
* Description:: This file contains implementations of b_open, b_read, and b_close,
* which together manage file metadata, perform buffered block reads using LBAread,
* and clean up resources.
*
**************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "b_io.h"
#include "fsLowSmall.h"

#define MAXFCBS 20	//The maximum number of files open at one time


// This structure is all the information needed to maintain an open file
// It contains a pointer to a fileInfo strucutre and any other information
// that you need to maintain your open file.
typedef struct b_fcb
	{
	fileInfo * fi;	//holds the low level systems file info

	// 512-byte internal buffer	
	char *buffer;

	// Index into buffer (next unread byte)            
	int bufferIndex;

	// Number of bytes currently in buffer 	
	int bufferSize;

	// Absolute position in file, ex: total bytes read so far) 
	int filePosition;

	// Total size of file in bytes
	int fileSize;

	// Current block number relative to file's starting block
	int blockIndex;

	} b_fcb;
	
//static array of file control blocks
b_fcb fcbArray[MAXFCBS];

// Indicates that the file control block array has not been initialized
int startup = 0;	

// Method to initialize our file system / file control blocks
// Anything else that needs one time initialization can go in this routine
void b_init ()
	{
	if (startup)
		return;			//already initialized

	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
		{
		fcbArray[i].fi = NULL; //indicates a free fcbArray
		}
		
	startup = 1;
	}

//Method to get a free File Control Block FCB element
b_io_fd b_getFCB ()
	{
	for (int i = 0; i < MAXFCBS; i++)
		{
		if (fcbArray[i].fi == NULL)
			{
			fcbArray[i].fi = (fileInfo *)-2; // used but not assigned
			return i;		//Not thread safe but okay for this project
			}
		}

	return (-1);  //all in use
	}

// b_open is called by the "user application" to open a file.  This routine is 
// similar to the Linux open function.  	
// You will create your own file descriptor which is just an integer index into an
// array of file control blocks (fcbArray) that you maintain for each open file.  
// For this assignment the flags will be read only and can be ignored.

b_io_fd b_open (char * filename, int flags)
	{
	if (startup == 0) b_init();  //Initialize our system

	// TESTING
	// printing filename and letting dev know b_open is being called
	// printf("b_open is called with the filename '%p'\n");

	// Get a free file control block to represent open file
	b_io_fd fd = b_getFCB();

	// Check if block is valid/available if not return an error(-1). Helps system resources and
	// prevents unexpected behavior
	if (fd < 0) {
		return - 1;
	}

	// Retrieving file metadata. ex: its size and starting block.Important to know how much data
	// to read and where file starts
	fileInfo *fileInformation = GetFileInfo(filename);

	// If the file does not exist or its metadata is not being retrieved we free up file control
	// block to ensure no unexpected behvavior and return err(-1)
	if (fileInformation == NULL) {
		fcbArray[fd].fi = NULL;
		return -1;
	}

	// Link retrieved metadata with our conrol block which is important for future acts that rely
	// on file properties
	fcbArray[fd].fi = fileInformation;

	// Record the total size of file so we know when we reach end
	fcbArray[fd].fileSize = fileInformation->fileSize;

	//init file reading position starting from beginning
	fcbArray[fd].filePosition = 0;

	// Block index is set to starting block to direct read operations
	fcbArray[fd].blockIndex = fileInformation->location;

	// Start buffer pointer at the beggining. At start no data is loaded
	fcbArray[fd].bufferIndex = 0;

	// No valid data is in buffer at first so size starts at 0
	fcbArray[fd].bufferSize = 0;

	// TESTING
	// Verifying file metadata
	// printf(
	// "b_open, file '%s', fileSize: %d, starting block: %d\n",
	// filename, info->fileSize, info->location
	// );

	// Allocate a fixed size buffer for this file, this way allocated individual buffers per each
	// file
	fcbArray[fd].buffer = (char *) malloc(B_CHUNK_SIZE);

	// If memory allocation fails, clean up state and return err(-1)
	if (fcbArray[fd].buffer == NULL) {
		fcbArray[fd].fi = NULL;
		return -1; 
	}
	
	// TESTING
	// printing allocated buffer and filename to verify looks correct
	// printf("allocated buffer at %p. file '%s', fd %d\n", fcbArray[fd].buffer, filename, fd);

	// Return unique file descriptor that identifies opened file. Will be important when
	// using read and close file operations.
	return fd;
	}



// b_read functions just like its Linux counterpart read.  The user passes in
// the file descriptor (index into fcbArray), a buffer where thay want you to 
// place the data, and a count of how many bytes they want from the file.
// The return value is the number of bytes you have copied into their buffer.
// The return value can never be greater then the requested count, but it can
// be less only when you have run out of bytes to read.  i.e. End of File	
int b_read (b_io_fd fd, char * buffer, int count)
	{
		
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}

	// and check that the specified FCB is actually in use	
	if (fcbArray[fd].fi == NULL)		//File not open for this descriptor
		{
		return -1;
		}	

	// TESTING
	// Printing that b_read has been called the fd it has been called on to verify when it is
	// being called
	// printf("b_read: called with fd=%d\n", fd);

	// Get file control block for open file using the file id
	b_fcb *fcb = &fcbArray[fd];

	// Calculate the remaining amount of bytes available to read in the file. Needed to see when
	// we reach end of file
	int bytesRemaining = fcb->fileSize - fcb->filePosition;

	// If there is nothing left to read, signal end-of-file
	if (bytesRemaining <= 0) {
		return 0;
	}

	// We only need to transfer at most the number of bytes remaining in the file. do this by using
	// therequested count if enough data exists or only the remaining bytes if the request is 
	// larger than what’s left
	int bytesToTransfer = (count < bytesRemaining) ? count : bytesRemaining;
	
	// Total bytes transferred to callerBuffer
	int totalCopied = 0;
	
	// Go until we have copied the requested amount or until the end of the file
	while (totalCopied < bytesToTransfer) {
		// First, if there is leftover data in our internal buffer, use it before reading
		// new blocksif 
		if (fcb->bufferIndex < fcb->bufferSize) {
			// Calculate how many bytes are currently available in the internal buffer
			int availableInBuffer = fcb->bufferSize - fcb->bufferIndex;
			
			// We need only as many bytes as remain to satisfy the request
			int bytesNeeded = bytesToTransfer - totalCopied;
			int bytesFromBuffer = 
			(availableInBuffer < bytesNeeded) ? availableInBuffer : bytesNeeded;

			// Instead of copying byte-by-byte, we copy a block of bytes 
			memcpy(buffer + totalCopied, fcb->buffer + fcb->bufferIndex, bytesFromBuffer);
			
			// TESTING
			// Printing to console to verify the buffer size, how many bytes were copies
			// the index. Used to see if looks to be working correctly.
			// printf(
			// 	"copied %d bytes from buffer. bufferIndex %d, bufferSize %d\n",
			// 	bytesFromBuffer, fcb->bufferIndex, fcb->bufferSize
			// );

			// Update our internal pointer and the caller's progress
			fcb->bufferIndex += bytesFromBuffer;
			fcb->filePosition += bytesFromBuffer;
			totalCopied += bytesFromBuffer;

			// Continue on in the loop, if we still need more data we will read more
			continue;
		}
		// If our internal buffer is empty we need to fetch a new 512-byte block. We check if we
		// can optimize a direct block read to the caller's buffer. If the caller needs at least
		// one full block, and the file has a full block available we bypass our internal buffer
		
		// Determine how many more bytes we need to complete
		int bytesNeeded = bytesToTransfer - totalCopied;

		// Compute the remaining bytes available in the file so we know if a full block can be 
		// read
		int bytesLeft = fcb->fileSize - fcb->filePosition;
		
		// if have at least a full block of data we can adjust for better performance
		if (bytesNeeded >= B_CHUNK_SIZE && bytesLeft >= B_CHUNK_SIZE) {
			// TESTING
			// printf("direct block read at blockIndex=%d\n", fcb->blockIndex);
			
			// Fill in buffer with full block, pass buffer offset by total copied. The block index
			// keeps track of where we read from
			uint64_t blocksRead = LBAread(buffer + totalCopied, 1, fcb->blockIndex);

			// If LBAread fails to read a full block we handle as an error
			if (blocksRead != 1) {
				return -1; 
			}

			// Update our tracking variables such that the entire block was used
			totalCopied += B_CHUNK_SIZE;
			fcb->filePosition += B_CHUNK_SIZE;
			fcb->blockIndex++;

			// Since we filled buffer directly our internal buffer remains empty. Meaning
			// bufferindex and buffersize stay at 0
			continue;
		}

		//TESTING
		// Verify when reading new block into buffer from blockindx
		// printf("new block into buffer from BI=%d.\n", fcb->blockIndex);

		// Otherwise perform a buffer read by requesting a new block of data into our
		// internal buffer. We will request B_CHUNK_SIZe bytes even if file might not have
		// that much left
		uint64_t blocksRead = LBAread(fcb->buffer, 1, fcb->blockIndex);

		// Should be able to read full block, if not handle err
		if (blocksRead != 1) {
			return -1;
		}

		// Determine the actual number of bytes we got. For the final block of the file bc we
		// will probably get fewer than B_CHUNK_SIZE bytes
		// Recalculate the remaining bytes in the file after the latest read
		int updatedBytesLeft = fcb->fileSize - fcb->filePosition;
		// Use the remaining bytes if they are less than a full block but if the arent,
		// use B_CHUNK_SIZE
		int bytesFromBlock = (updatedBytesLeft < B_CHUNK_SIZE) ? updatedBytesLeft : B_CHUNK_SIZE;

		// Reset the internal buffer indixes to reflect the new data
		fcb->bufferSize = bytesFromBlock; 
		fcb->bufferIndex = 0;

		// After filling the internal buffer the loop will copy from it
		fcb->blockIndex++;
	}

	// TESTING
	// Seeing how much total copied bytes are going to be returned
	// printf("totalCopied=%d bytes for fd=%d.\n", totalCopied, fd);

	// Return the total number of bytes transferred
	return totalCopied;
	}


// b_close frees and allocated memory and places the file control block back 
// into the unused pool of file control blocks.
int b_close (b_io_fd fd)
	{
	
	// TESTING
	// Verifying when bclose starts and ends to make sure finishes running with no errs
	// printf('b_close called for fd %d\n' , fd);

	// Free the buffer that was allocated in b_open. Each file has its own buffer so releasing
	// it here prevents memory leaks
	if (fcbArray[fd].buffer != NULL) {
		free(fcbArray[fd].buffer);
		fcbArray[fd].buffer = NULL;
	}

	// Set file pointer to NULL. Makes sure that other files can reuse this slot
	fcbArray[fd].fi = NULL;

	//TESTING
	// printf(b_close finished sucess);

	// return 0 = sucess
	return 0;
	}
	
