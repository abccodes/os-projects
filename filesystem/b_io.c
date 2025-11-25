/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Arric Sekhon, Aidan Bayer-Calvert, Fabian Camarena Veliz, and Subhan Khan
* Student IDs:: 923090490, 922119403, 924100096, and 921938807
* GitHub-Name:: FlavyFabo
* Group-Name:: AAXF
* Project:: Basic File System
*
* File:: b_io.c
*
* Description:: This code is a simple version of file reading and 
*				writing that sets up the basics for opening, reading, 
*				writing, and closing files, but most of the real work 
*				still needs to be filled in.
**************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <errno.h>
#include "fsLow.h"
#include "freeSpace.h"
#include "b_io.h"
#include "mfs.h"
#include "rootdir.h"

// Maximum number of current open files
#define MAXFCBS 20

// Size for read/write buffer (not used yet)
#define B_CHUNK_SIZE 512

// Global fsm for block allocation
extern FreeSpaceManager g_fsm;

// helper to allocate single block
extern uint32_t alloc_one_block(FreeSpaceManager *fsm);

// Array of file control blocks(FCBs)
b_fcb fcbArray[MAXFCBS];

//Indicates that this has not been initialized
int startup = 0;

//Method to initialize our file system by setting up all the FCB entries as unused
void b_init ()
{
	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++) {

        //indicates a free fcbArray
		fcbArray[i].buf = NULL;
	}
	
    // record initialization is complete
	startup = 1;
}

//Method to get a free FCB element index
b_io_fd b_getFCB ()
{

    // Iterate to find free fcb
	for (int i = 0; i < MAXFCBS; i++) {
		if (fcbArray[i].buf == NULL) {

            //return the index of the free fcbArray
			return i;
		}
	}

    // no free fcbs
	return (-1);
}

// helper, split a path into parent and leaf. Eg: “/dir/file” -> “/dir” + “file”
static int local_split_path(const char *path, char *parent, char *leaf)
{
    // Guarding for invalid input to avoid random errors
    if (!path || !*path) {
        return -1;
    }
        
    char tmp[PATH_MAX];

    // copy to avoid modifying callers buffer
    strncpy(tmp, path, PATH_MAX - 1);

    // null terminator to avoid errors
    tmp[PATH_MAX - 1] = '\0';

    // Remove trailing slash (except for root) to normalize path for parsing
    size_t len = strlen(tmp);
    while (len > 1 && tmp[len - 1] == '/') {
        tmp[--len] = '\0';
    }

    // find last directory seperator
    char *slash = strrchr(tmp, '/');

    // no slash means current directory and return success if complete
    if (!slash) {
        strcpy(parent, ".");
        strcpy(leaf, tmp);
        return 0;
    }

    // slash at start = root dir and return success if complete
    if (slash == tmp) {
        strcpy(parent, "/");
        strcpy(leaf, slash + 1);
        return 0;
    }

    // split into parent and leaf
    *slash = '\0';
    strcpy(parent, tmp);
    strcpy(leaf,  slash + 1);

    // Return success
    return 0;
}


// Method to open the target directory
// If the path is absolute, open it directly. If it's relative, open the current working
// directory. This is a helper function for the b_open function
// Helper: produce a normalized, single-level absolute path
static fdDir *open_target_dir(const char *parentPath)
{
    return fs_opendir(parentPath);
}

/*
Method to find or create a directory entry in the given directory
This function performs two main tasks:
1. Searches the provided directory (fdDir *dir) for a file/directory named `leaf`.
    -> If a matching entry is found and is in use, it returns its index. 
2. If not found:
    -> If the O_CREAT flag is not set, it returns -1 and sets errno to ENOENT (No such file).
    -> If O_CREAT is set, it tries to find an unused slot in the directory (starting from index
    2,
        skipping "." and "..").
        -> If a free slot is found, it initializes a new regular file entry (type = FT_REGFILE),
          sets metadata like timestamps and owner, marks it as in-use, and writes the updated
          directory back to disk using writeDir.
        -> If the directory is full (no free slot available), it returns -1 and sets errno to 
        ENOSPC.

This is a helper function for the b_open function
*/
static int find_or_create_in_dir(fdDir *dir, const char *leaf, int flags)
{
    // Search for existing entry
    for (uint32_t i = 0; i < dir->total_entries; ++i) {
        if (dir->entries[i].inUse && strcmp(dir->entries[i].name, leaf) == 0) {
            // it is found
            return i;                      
        }
    }

    // If not found, check if we can create it
    if (!(flags & O_CREAT)) {
            //File does not exist, and O_CREAT not set. Return failure.
            errno = ENOENT;
            return -1; 
    }
        
    // Available slot for new entry
    // Skip entries 0 and 1, which are reserved for "." and ".."
    uint32_t slot = 2;

    while (slot < dir->total_entries && dir->entries[slot].inUse) {
        ++slot;
    }

    if (slot == dir->total_entries) {
        // No available slot for new file
        errno = ENOSPC;
        return -1; 
    }

    // Initialize new file entry
    directory_entry *de = &dir->entries[slot];
    memset(de, 0, sizeof *de);
    strncpy(de->name, leaf, sizeof de->name - 1);
    de->fileType = FT_REGFILE;
    de->fileSize = 0;
    de->ownerID = getuid();
    de->created_at = de->modified_at = de->accessed_at = time(NULL);
    de->inUse = 1;

    // Allocate a new data block for the file
    writeDir(dir->entries, dir->block_count, dir->starting_block);

    // Return index of newly created file entry
    return slot;
    }


// Interface to open a file
// This function opens a file and returns a file descriptor (fd) for it.
// It handles the following tasks:
// 1. Initializes the file system if not already done.
// 2. Splits the provided filename into parent directory and leaf name.
// 3. Opens the parent directory and finds or creates the file entry.
// 4. If O_TRUNC is set, it resets the file size and updates the modified timestamp.
// 5. Allocates an in-memory file control block (FCB) for the opened file.
// 6. Sets the file pointer to the end of the file if O_APPEND is set.
// 7. Returns the file descriptor (fd) for the opened file or -1 on error.
b_io_fd b_open(char *filename, int flags) 
{   
    // lazy init
    if (!startup) {
        b_init();
    }

    // Split the filename into parent directory and leaf name
    char parent[PATH_MAX], leaf[NAME_MAX];

    // invalid path check to avoid errors
    if (local_split_path(filename, parent, leaf) < 0) { 
        errno = EINVAL; 
        return -1; 
    }

    // Open the target directory
    fdDir *pdir = open_target_dir(parent);
    if (!pdir) {
        return -1;
    }

    int slot = find_or_create_in_dir(pdir, leaf, flags);
    if (slot < 0) { 
        fs_closedir(pdir); 
        return -1; 
    }

    directory_entry *de = &pdir->entries[slot];

    // If the file is new and has no data blocks, allocate one
    if (de->fileSize == 0 && de->location == 0) {

        uint32_t blk = alloc_one_block(&g_fsm);

        if (blk == UINT32_MAX) { 
            fs_closedir(pdir); errno = ENOSPC; 
            return -1; 
        }

        de->location = blk;
    }

    // If O_TRUNC is set, reset file size and update modified timestamp
    if ((flags & O_TRUNC) && (flags & (O_WRONLY|O_RDWR))) {
        de->fileSize   = 0;
        de->modified_at= time(NULL);
    }

    // Write the updated directory entry back to disk
    writeDir(pdir->entries, pdir->block_count, pdir->starting_block);

    uint32_t dirStart  = pdir->starting_block;
    uint32_t dirBlocks = pdir->block_count;

    // done with directory
    fs_closedir(pdir);

    // Allocate a file control block (FCB) for the opened file
    int fcb = b_getFCB();
    if (fcb < 0) { 
        errno = EMFILE; return -1;
    }
    memset(&fcbArray[fcb], 0, sizeof(b_fcb));

    // Initialize the FCB
    fcbArray[fcb].flags = flags;
    fcbArray[fcb].dirSlot = slot;
    fcbArray[fcb].dirStart = dirStart;
    fcbArray[fcb].dirBlocks = dirBlocks;
    fcbArray[fcb].start_lba = de->location;
    fcbArray[fcb].fileSize = de->fileSize;
    fcbArray[fcb].pos = (flags & O_APPEND) ? de->fileSize : 0;
    fcbArray[fcb].curr_lba = UINT32_MAX;
    fcbArray[fcb].buf = malloc(B_CHUNK_SIZE);

    // Check if memory allocation was successful
    if (!fcbArray[fcb].buf) { 
        errno = ENOMEM; 
        return -1; 
    }

    // return file descriptor
        return fcb;
}


// Interface to seek function	
// This function sets the file position indicator for the specified file descriptor (fd).
// It takes an offset (off) and a whence parameter to determine the new position.
// The whence parameter can be SEEK_SET (absolute), SEEK_CUR (relative to current position),
// or SEEK_END (relative to end of file).
// It returns 0 on success or -1 on error. The function also checks for valid file descriptors
// and ensures the resulting position is within file bounds.

int b_seek(b_io_fd fd, off_t off, int whence)
{
    // Check if the file system is initialized
    if (!startup) {
        b_init();
    }

    // Check if the file descriptor is valid
    if (fd < 0 || fd >= MAXFCBS || !fcbArray[fd].buf) { 
        errno = EBADF; 
        return -1; 
    }

    // Check if the file is open
    b_fcb *f = &fcbArray[fd];
    long target;

    // Determine the new position based on whence
    switch (whence) {
        case SEEK_SET: target = off; break;
        case SEEK_CUR: target = (long)f->pos + off; break;
        case SEEK_END: target = (long)f->fileSize + off; break;
        default: errno = EINVAL; return -1;
    }

    // Check if the target position is within valid bounds
    if (target < 0 || target > (long)f->fileSize) { 
        errno = EINVAL; 
        return -1; 
    }

    // Update the file position indicator
    f->pos = (uint32_t)target;
    f->curr_lba = UINT32_MAX;   
    f->buflen = 0;

    // return success
    return 0;
}



// Interface to write function	
// This function writes data from the source buffer (src) to the file associated with the given
// file descriptor (fd). It returns the total number of bytes written or -1 on error.
int b_write(b_io_fd fd, char *src, int count)
{
    // Initialize the file system if not already done
    if (!startup) {
        b_init();
    }

    // Check if the file descriptor is valid and the buffer is allocated
    if (fd < 0 || fd >= MAXFCBS || !fcbArray[fd].buf || count <= 0) { 
        errno = EBADF; 
        return -1; 
    }

    b_fcb *f = &fcbArray[fd];

    // Check if the file was opened with write or read/write permission
    if (!(f->flags & (O_WRONLY | O_RDWR))) { 
        errno = EBADF; 
        return -1; 
    }
    
    // Total bytes written
    int total = 0;

    // Write until all requested bytes are written
    while (total < count) {
        uint32_t need_lba = f->start_lba + (f->pos / B_CHUNK_SIZE);
        uint32_t off = f->pos % B_CHUNK_SIZE;
        uint32_t space = B_CHUNK_SIZE - off;
        uint32_t left = count - total;
        uint32_t n = (left < space) ? left : space;

        // If it's not a full block write, read existing block before writing
        if (off != 0 || n != B_CHUNK_SIZE) {
            LBAread(f->buf, 1, need_lba);
        }

        // Copy data to the buffer
        memcpy(f->buf + off, src + total, n);
        // Write the buffer to disk
        LBAwrite(f->buf, 1, need_lba);

        // Update counters
        total += n;
        f->pos += n;
    }

    // If file grew in size, update its size
    if (f->pos > f->fileSize) {
        f->fileSize = f->pos;

        if (!g_vcb) {
            printf("[ERROR] g_vcb is NULL in b_write\n");
            return -1;
        }

        // Load directory block(s) from disk
        uint32_t bs = g_vcb->blockSize;
        uint32_t bCnt = f->dirBlocks;
        directory_entry *buf = malloc(bCnt * bs);

        LBAread(buf, bCnt, f->dirStart);

        // Update the file size in the directory entry
        buf[f->dirSlot].fileSize = f->fileSize;

        // Write the updated directory entry back to disk
        LBAwrite(buf, bCnt, f->dirStart);
        free(buf);
    }
    // Debugging printf
    // printf("f->fileSize = %u\n", f->fileSize);
    // printf("dirSlot = %u\n", f->dirSlot);
    // printf("dirStart = %u\n", f->dirStart);
    // printf("dirBlocks = %u\n", f->dirBlocks);
    // printf("g_vcb->blockSize = %u\n", g_vcb ? g_vcb->blockSize : 0);
    return total; // return the total number of bytes written
}


// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be enough
// Part 2 is after using what was left in our buffer there is still 1 or more block
//        size chunks needed to fill the callers request.  This represents the number of
//        bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from a refill 
//        of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+

int b_read(b_io_fd fd, char *buffer, int count)
{
    // Check if the file system is initialized
    if (!startup) {
        b_init();
    }

    // Check if the file descriptor is valid and the buffer is allocated
    if (fd < 0 || fd >= MAXFCBS || !fcbArray[fd].buf || count <= 0) {
        errno = EBADF;
        return -1;
    }

    b_fcb *f = &fcbArray[fd];

    // Return 0 if at end of file
    if (f->pos >= f->fileSize) {
        return 0;
    }

    // how many bytes we want to read
    int want = count;

    // how many bytes we actually read
    int copied = 0;

    // Part 1 : fill from existing buffer
    // already have a block in the buffer
    if (f->curr_lba != UINT32_MAX) {
        // offset in the buffer
        uint32_t off   = f->pos % B_CHUNK_SIZE;

        // Bytes available in the buffer
        uint32_t avail = B_CHUNK_SIZE - off;

        // Bytes left in the file
        uint32_t left  = f->fileSize - f->pos;

        // If the buffer is empty, refill it
        uint32_t n = avail < left ? avail : left;
        if (n > (uint32_t)want) {
            n = want;
        }

        // Copy from the buffer
        if (n) {
            memcpy(buffer, f->buf + off, n);
            f->pos += n;
            copied += n;
            want   -= n;
        }
    }

    // Part 2 : fill from disk in multiples of block size
    while (want >= B_CHUNK_SIZE && f->pos + B_CHUNK_SIZE <= f->fileSize) {
        uint32_t lba = f->start_lba + (f->pos / B_CHUNK_SIZE);

        // read a block from disk
        LBAread(buffer + copied, 1, lba);
        f->pos += B_CHUNK_SIZE;
        copied += B_CHUNK_SIZE;
        want   -= B_CHUNK_SIZE;
    }

    // Part 3 : fill from disk less than block size
    if (want > 0 && f->pos < f->fileSize) {
        uint32_t lba = f->start_lba + (f->pos / B_CHUNK_SIZE);

        // Load the block into the buffer if not already loaded
        if (lba != f->curr_lba) {
            LBAread(f->buf, 1, lba);
            f->curr_lba = lba;
        }

        uint32_t off   = f->pos % B_CHUNK_SIZE;
        uint32_t avail = B_CHUNK_SIZE - off;
        uint32_t left  = f->fileSize - f->pos;

        uint32_t n = avail < left ? avail : left;
        if (n > (uint32_t)want) {
            n = want;
        }

        // copy from the buffer
        memcpy(buffer + copied, f->buf + off, n);
        f->pos += n;
        copied += n;
        want   -= n;
    }

    // Return total bytes read
    return copied;
}
	
// Interface to close a file
// This function closes th  file associated with the given file descriptor (fd).
// It performs cleanup by freeing the allocated buffer and resetting the file control block (FCB).
// It returns 0 on success or -1 on error. The function also checks for valid file descriptors
// and ensures the buffer is allocated before attempting to close the file.
int b_close(b_io_fd fd)
{
    // Check if the file system is initialized
    if (fd < 0 || fd >= MAXFCBS || !fcbArray[fd].buf) { 
        errno = EBADF; 
        return -1; 
    }

    // Free the allocated buffer
    free(fcbArray[fd].buf);

    // Clear the entire FCB structure, marking it as free
    memset(&fcbArray[fd], 0, sizeof(b_fcb));

    // return success
    return 0;
}