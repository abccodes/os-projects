/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Arric Sekhon, Aidan Bayer-Calvert, Fabian Camarena Veliz, and Subhan Khan
* Student IDs:: 923090490, 922119403, 924100096, and 921938807
* GitHub-Name:: FlavyFabo
* Group-Name:: AAXF
* Project:: Basic File System
*
* File:: mfs.c
*
* Description:: 
*	This is the file system interface.
*	This is the interface needed by the driver to interact with
*	your filesystem.
*
**************************************************************/

#include "mfs.h"
#include "vcb.h"
#include "rootdir.h"
#include "freeSpace.h"
#include "fsLow.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

// Global volume control block
extern VCB *g_vcb;

// Initial CWD set to root
static char current_working_directory[PATH_MAX] = "/";

// Helper: Create absolute path
static void make_absolute_path(char *destination, const char *base, const char *name) 
{   
    // if base as root
    if (base[0] == '/' && base[1] == '\0')  {

        // append name to root
        snprintf(destination, PATH_MAX, "/%s", name);
    } else {

        // apend with "/"
        snprintf(destination, PATH_MAX, "%s/%s", base, name);
    }

    // ensure null termination
    destination[PATH_MAX - 1] = '\0';
}

// Helper: Split path into parent and child for directory
// This function takes a path and splits it into parent and child components.
// It handles absolute and relative paths, ensuring that the parent path is valid.
// It also removes any trailing slashes from the path.
// The function returns 0 on success, and -1 on failure. The parent and child strings
// are null-terminated and should be large enough to hold the respective components.
// The function also handles special cases like root directories and empty paths.
static int split_path(const char *path, char *parent, char *child) 
{   
    // Invalid path
    if (!path || !*path) {
        return -1;
    }

    // Copy path to temp_path
    char temp_path[PATH_MAX];
    strncpy(temp_path, path, PATH_MAX - 1);
    temp_path[PATH_MAX - 1] = '\0';

    size_t len = strlen(temp_path);
    while (len > 1 && temp_path[len - 1] == '/') {
        // Remove trailing slashes
        temp_path[--len] = '\0';
    }

    // Find last slash "/"
    char *last_slash = strrchr(temp_path, '/');

    // No slash means current directory
    if (!last_slash) {
        strcpy(parent, ".");
        strcpy(child, temp_path);
        return 0;
    }

    // Root directory
    if (last_slash == temp_path) {
        strcpy(parent, "/");
        strcpy(child, last_slash + 1);
        return 0;
    }

    // Terminate parent path
    *last_slash = '\0';

    strcpy(parent, temp_path);
    strcpy(child, last_slash + 1);

    // Success
    return 0;
}

// Return current working directory
char *fs_getcwd(char *buf, size_t size) 
{
    // Invalid buffer or size
    if (!buf || size == 0) {
        return NULL; 
    }
    
    // Copy current working directory
    strncpy(buf, current_working_directory, size - 1);
    
    // Ensure null termination
    buf[size - 1] = '\0';
    
    // Return buffer
    return buf;
}

// Change working directory
int fs_setcwd(char *path) 
{
    // Check if path is valid
    if (!path || !*path) { 
        return -1;
    }

    // Construct absolute path
    char absolute_path[PATH_MAX];

    // Determine if the input path is absolute or relative.
    // If the path starts with '/', it is an absolute path, so copy it directly.
    // else, construct an absolute path by combining the current working directory
    //  with the given relative path using make_absolute_path.
    // After constructing the path, ensure it is null-terminated to avoid buffer overflows.
    if (path[0] == '/') {
        strncpy(absolute_path, path, PATH_MAX - 1);
    } else { 
        make_absolute_path(absolute_path, current_working_directory, path);
    }

    absolute_path[PATH_MAX - 1] = '\0';

    // Remove any trailing slashes (e.g., "/dir/" -> "/dir")
    size_t len = strlen(absolute_path);
    while (len > 1 && absolute_path[len - 1] == '/') {
        absolute_path[--len] = '\0';
    }
    // Check if the path is a valid directory
    fdDir *directory = fs_opendir(absolute_path);
    if (!directory) {
        errno = ENOENT;
        return -1;
    }

    // Close the directory handle after checking
    fs_closedir(directory);

    // If the path is ".." or ends with "/..", set the current working directory to root
    // This is a special case to handle parent directory navigation.
    // Otherwise, copy the absolute path to the current working directory.
    // This ensures that the current working directory is always a valid path.
    // The current working directory is limited to PATH_MAX - 1 characters to ensure null 
    // termination. Finally, ensure the current working directory is null-terminated.
    if (
        strcmp(absolute_path, "/..") == 0 ||
        strcmp(absolute_path + strlen(absolute_path) - 3, "/..") == 0
        ) {
        strncpy(current_working_directory, "/", PATH_MAX - 1);
    } else {
        strncpy(current_working_directory, absolute_path, PATH_MAX - 1);
    }
    current_working_directory[PATH_MAX - 1] = '\0';

    return 0;
}

// Check if path is directory
int fs_isDir(const char *path) {

    // open directory
    fdDir *dir = fs_opendir(path);

    // if failed to open, return 0
    if (!dir) {

        // Return success
        return 0;
    } 

    // close directory
    fs_closedir(dir);

    // Return fail
    return 1;
}

//Check if path is file 
int fs_isFile(const char *path) {
    char parent[PATH_MAX], leaf[NAME_MAX];

    // Split the full path into parent directory and leaf name (file name)
    // e.g., "/dir/file.txt" -> parent = "/dir", leaf = "file.txt"
    if (split_path(path, parent, leaf) < 0)
        return 0;  // Invalid path

    // Try to open the parent directory
    fdDir *dir = fs_opendir(parent);
    if (!dir) {
        // Failed to open directory, so file can't exist
        return 0;
    }

    // Loop through entries in the directory
    for (int i = 0; i < dir->total_entries; i++) {

        // If this entry is in use and its name matches the target leaf
        if (dir->entries[i].inUse && strcmp(dir->entries[i].name, leaf) == 0) {

            // Check if the matched entry is a regular file
            int isFile = (dir->entries[i].fileType == FT_REGFILE);

            // Clean up
            fs_closedir(dir);

            // Return true (1) if it's a file, false (0) otherwise
            return isFile;
        }
    }

    // File not found in directory entries
    fs_closedir(dir);

    // Return success
    return 0;
}


// Load root directory
// This function loads the root directory structure from disk into memory.
// It calculates how many blocks are needed for the root directory,
// reads those blocks from disk into a buffer, and wraps them in a fdDir structure.
static fdDir *load_root_directory(void) {

    // Get block size from VCB
    uint32_t block_size = g_vcb->blockSize;

    // Calculate how many blocks are needed for the root directory
    uint32_t blocks = (50 * sizeof(directory_entry) + block_size - 1) / block_size;

    // Allocate memory for the directory entries 
    directory_entry *buffer = calloc(blocks, block_size);
    if (!buffer) return NULL;

    // Read the root directory from disk into the buffer
    LBAread(buffer, blocks, g_vcb->rootDirStart);

    // Check if the read was successful
    fdDir *root_dir = calloc(1, sizeof(fdDir));
    if (!root_dir) {
        free(buffer);
        return NULL;
    }

    // initialize fdDir structure
    root_dir->entries = buffer;
    root_dir->total_entries = (blocks * block_size) / sizeof(directory_entry);
    root_dir->block_count = blocks;
    root_dir->starting_block = g_vcb->rootDirStart;

    // return the root directory handle
    return root_dir;
}

// Open directory
// This function opens a directory specified by the path.
// It checks if the path is valid, converts it to an absolute path if necessary,
// and then attempts to load the directory structure from disk.
// If the directory is found, it returns a pointer to a fdDir structure containing
// the directory entries. If the directory is not found or an error occurs,
// it returns NULL and sets errno appropriately.
fdDir *fs_opendir(const char *path) {
    if (!path || !*path) {
        return NULL;
    }

    // Build a cleaned-up absolute path in 'abs'
    char abs[PATH_MAX];
    if (path[0] == '/') {
        strncpy(abs, path, PATH_MAX-1);
    } else {
        make_absolute_path(abs, current_working_directory, path);
    }

    abs[PATH_MAX-1] = '\0';

    // strip trailing slashes
    size_t len = strlen(abs);
    while (len > 1 && abs[len-1] == '/') {
        abs[--len] = '\0';
    }

    // handle “..” at end → root
    if (strcmp(abs, "/..") == 0 ||(len > 3 && strcmp(abs + len-3, "/..") == 0))
    {
        strcpy(abs, "/");
    }

    // If it’s exactly “/”, return root
    if (strcmp(abs, "/") == 0) {
        return load_root_directory();
    }

    // Tokenize everything after the leading “/”
    //    e.g. "/a/b/c" → ["a","b","c"]
    char *copy = strdup(abs+1), *saveptr;
    if (!copy) return NULL;
    char *components[PATH_MAX/2];
    int ncomp = 0;
    for (char *tok = strtok_r(copy, "/", &saveptr);
         tok;
         tok = strtok_r(NULL, "/", &saveptr))
    {
        components[ncomp++] = tok;
    }

    // start at root
    fdDir *current = load_root_directory();
    if (!current) {
        free(copy); return NULL;
    }

    // For each path component, descend one level
    for (int i = 0; i < ncomp; i++) {
        directory_entry *found = NULL;
        for (uint32_t j = 0; j < current->total_entries; j++) {
            directory_entry *e = &current->entries[j];
            if 
            (
            e->inUse
             && strcmp(e->name, components[i]) == 0
             && e->fileType == DIRECTORY
            )
            {
                found = e;
                break;
            }
        }
        if (!found) {
            fs_closedir(current);
            free(copy);
            errno = ENOENT;
            return NULL;
        }

        // read that child directory’s blocks
        uint32_t bs = g_vcb->blockSize;
        uint32_t nb = (found->fileSize + bs - 1) / bs;
        directory_entry *buf = calloc(nb, bs);
        
        if (!buf) {
            fs_closedir(current);
            free(copy);
            return NULL;
        }
        LBAread(buf, nb, found->location);

        // build a new fdDir and free the old
        fdDir *next = calloc(1, sizeof *next);
        next->entries        = buf;
        next->total_entries  = (nb * bs) / sizeof(directory_entry);
        next->block_count    = nb;
        next->starting_block = found->location;
        fs_closedir(current);
        current = next;
    }

    free(copy);
    return current;
}

// Read directory
struct fs_diriteminfo *fs_readdir(fdDir *dir_ptr) {

    // Invalid directory pointer
    if (!dir_ptr) {
        return NULL;
    }


    // Cast to fdDir
    fdDir *dir_handle = (fdDir *)dir_ptr;

    // Static info structure to hold directory entry info
    static struct fs_diriteminfo info;

    // Iterate through directory entries until a valid in-use entry is found
    while (dir_handle->current_pos < dir_handle->total_entries) {

        // Get the current entry and advance the position
        directory_entry *entry = &dir_handle->entries[dir_handle->current_pos++];

        // Skip unused entries
        if (!entry->inUse) {
            continue;
        }

        // Fill the info structure with entry details
        info.fileType = (entry->fileType == DIRECTORY) ? FT_DIRECTORY : FT_REGFILE;
        strncpy(info.d_name, entry->name, sizeof info.d_name);

        // Return the filled info structure
        return &info;
    }

    // No more entries to read
    return NULL;
}

// Close an opened directory
// This function releases all memory associated with a directory that was opened using fs_opendir.
// It ensures proper cleanup by freeing the directory entries and the directory structure itself.
int fs_closedir(fdDir *dir_ptr) {
    
    // Invalid directory pointer
    if (!dir_ptr) {
        return -1;
    }

    // Free the directory handle
    fdDir *dir_handle = (fdDir *)dir_ptr;

    // Free the entries
    free(dir_handle->entries);

    // Free the Data structure itself
    free(dir_handle);

    // Success
    return 0;
}

// Make directory
// Create a new directory at the given path.
// Only supports creating directories directly under root ("/").
// Returns 0 on success, -1 on error and sets errno appropriately.
int fs_mkdir(const char *path, mode_t mode) {
    if (!path || !*path) {
        errno = EINVAL;
        return -1;
    }

    char parent[PATH_MAX], name[NAME_MAX];
    if (split_path(path, parent, name) < 0) {
        errno = ENOENT;
        return -1;
    }

    // “.” means current_working_directory
    if (strcmp(parent, ".") == 0) {
        strncpy(parent, current_working_directory, PATH_MAX-1);
        parent[PATH_MAX-1] = '\0';
    }

    // open that parent directory (can be any depth now)
    fdDir *pd = fs_opendir(parent);
    if (!pd) return -1;
    fdDir *parent_dir = (fdDir*)pd;

    // find a free slot (skip 0 and 1)
    uint32_t slot = 2;
    while (slot < parent_dir->total_entries && parent_dir->entries[slot].inUse) {
        slot++;
    }
    if (slot == parent_dir->total_entries) {
        fs_closedir(pd);
        errno = ENOSPC;
        return -1;
    }

    // allocate & initialize the new directory blocks
    directory_entry *newdir = createDir(50, parent_dir->entries, &g_fsm);
    if (!newdir) {
        fs_closedir(pd);
        return -1;
    }

    // fill in the parent’s slot
    directory_entry *e = &parent_dir->entries[slot];
    memset(e, 0, sizeof *e);
    strncpy(e->name, name, sizeof e->name - 1);
    e->fileType = DIRECTORY;
    e->location = newdir[0].location;
    e->fileSize = newdir[0].fileSize;
    e->ownerID = getuid();
    e->created_at = e->modified_at = e->accessed_at = time(NULL);
    e->inUse = 1;
    e->file_no = newdir[0].file_no;

    // write parent back out
    writeDir(parent_dir->entries,
             parent_dir->block_count,
             parent_dir->starting_block);

    fs_closedir(pd);
    free(newdir);
    return 0;
}

// Removal of an empty directory, with cleanup of on-disk state
int fs_rmdir(const char *path) {

    // Validate input to prevent undefined behavior
    if (!path || !*path) {

        // Signal invalid argument to caller to maintain error reporting
        errno = EINVAL;
        return -1;
    }

    // Prepare buffers to hold separated path components for ease of use
    char parent[PATH_MAX], leaf[NAME_MAX];

    // Break the full path into container and target names
    if (split_path(path, parent, leaf) < 0) {

        // If splitting fails, the path likely doesn’t exist
        errno = ENOENT;
        return -1;
    }

    // . to the current working directory to support relative paths
    if (strcmp(parent, ".") == 0) {

        // Copy makes sure the rest of the logic works uniformly with absolute paths
        strcpy(parent, current_working_directory);
    }

    /// Enforce that only operations under the root directory are supported
    if (strcmp(parent, "/") != 0) {

        // Notify that deeper nesting isn’t implemented, avoiding inconsistent state
        errno = ENOTSUP; return -1;
    }

    // Verify the target really is a directory to prevent accidental file removal
    if (!fs_isDir(path)) {

        // Distinguish wrong-type errors early to maintain clear semantics
        errno = ENOTDIR; return -1;
    }

    // Open the directory to inspect its contents before deleting
    fdDir *tgt = fs_opendir(path);

    // Any lower-level errors, such as I/O failures
    if (!tgt) {

        // Failure
        return -1;
    }

    // Check that only “.” and “..” remain to ensure emptiness
    for (uint32_t i = 0; i < tgt->total_entries; i++) {
        directory_entry *e = &tgt->entries[i];
        
        // Skip unused entries and the two special self/parent markers
        if (e->inUse && strcmp(e->name, ".")  != 0 && strcmp(e->name, "..") != 0) {
            
            // Close handle to avoid resource leak before signaling failure
            fs_closedir(tgt);

            // Inform that directory contains extra items, preventing data loss
            errno = ENOTEMPTY;

            // Failure
            return -1;
        }
    }

    // Close the directory handle now that checks are complete
    fs_closedir(tgt);

    // Open the parent directory to remove the target entry from its listings
    fdDir *pd = fs_opendir(parent);

    // Propagate open errors
    if (!pd) {
        return -1;
    }
    
    // Initialize slot as not found to detect missing entries
    uint32_t slot = UINT32_MAX;

    // Search for the directory’s entry by name to prepare for its removal
    for (uint32_t i = 0; i < pd->total_entries; i++) {
        if (pd->entries[i].inUse &&
            strcmp(pd->entries[i].name, leaf) == 0) {
            slot = i;
            break;
        }
    }

    // If no matching entry was detected, revert any changes and report error
    if (slot == UINT32_MAX) {
        fs_closedir(pd); errno = ENOENT; return -1;
    }

    // Record where the directory’s blocks start and how large it was
    uint32_t startBlk  = pd->entries[slot].location;
    uint32_t sizeBytes = pd->entries[slot].fileSize;

    // Mark the entry unused to unlink it logically before persisting
    pd->entries[slot].inUse = 0;

    // Persist the updated parent directory structure to disk
    writeDir(pd->entries, pd->block_count, pd->starting_block);

    // Close the parent handle to release memory and locks
    fs_closedir(pd);

    // Determine block count to free entire directory contents area
    uint32_t blkSize = g_vcb->blockSize;
    uint32_t nBlocks = (sizeBytes + blkSize - 1) / blkSize;

    // Allocate an array listing each block for batch release
    uint32_t *blocks = malloc(nBlocks * sizeof *blocks);

    // Populate block addresses for subsequent bitmap updates
    for (uint32_t i = 0; i < nBlocks; i++) {
        blocks[i] = startBlk + i;
    }

    // Prepare the free-space manager to alter the on-disk allocation map
    FreeSpaceManager fsm;
    fsm.totalBlocks = g_vcb->totalBlocks;
    fsm.blockSize   = blkSize;

    // Calculate exact bitmap sizes to read/persist only necessary data
    uint32_t bmpBytes = (fsm.totalBlocks + 7) / 8;
    uint32_t bmpBytesR = ((bmpBytes + blkSize - 1) / blkSize) * blkSize;
    

    // Allocate a buffer to hold the current on-disk bitmap
    fsm.bitmapSize = bmpBytes;
    fsm.bitmap = malloc(bmpBytesR);
    
    // Load the existing allocation map so we can clear these blocks
    LBAread(fsm.bitmap, bmpBytesR / blkSize, g_vcb->bitmapStart);

    // Mark the freed blocks as available in the bitmap
    releaseBlocks(&fsm, blocks, nBlocks);

    // Prepare a zeroed buffer to wipe stale data from freed blocks
    char *zeros = calloc(1, blkSize);
    for (uint32_t b = 0; b < nBlocks; b++) {
        // Overwrite any residual directory entries to avoid future errors in files
        LBAwrite(zeros, 1, startBlk + b);
    }

    // Free to avoid memory issues
    free(zeros);

    // Persist the updated bitmap back to disk to make frees permanent
    LBAwrite(fsm.bitmap, bmpBytesR / blkSize, g_vcb->bitmapStart);

    // Update in-memory free-block count to keep VCB consistent
    g_vcb->freeBlockCount += nBlocks;

    // Clean up buffers to avoid memory leaks
    free(fsm.bitmap);
    free(blocks);

    // 0 Indicates sucess so caller can continue
    return 0;
}

// Begin removal of a regular file, using similar cleanup semantics
int fs_delete(char *path) {
    // Guard against null or empty strings to maintain API contract
    if (!path || !*path) {
        errno = EINVAL; 
        return -1;
    }

    // Separate the path into its parent directory and filename segments
    char parent[PATH_MAX], leaf[NAME_MAX];

    if (split_path(path, parent, leaf) < 0) {
        errno = ENOENT; 
        return -1;
    }

    // Translate relative indicator to absolute working directory
    if (strcmp(parent, ".") == 0) {
        strcpy(parent, current_working_directory);
    }

    // Only direct children of root supported for now
    if (strcmp(parent, "/") != 0) {
        errno = ENOTSUP;
        return -1;
    }

    // Confirm that the path refers to a file, not a directory
    if (!fs_isFile(path)) {
        errno = ENOENT;
        return -1;
    }

    // Open the parent directory to remove the file’s entry
    fdDir *pd = fs_opendir(parent);
    if (!pd) {
        return -1;
    }

    // Find the matching entry by name within that directory
    uint32_t slot = UINT32_MAX;
    for (uint32_t i = 0; i < pd->total_entries; i++) {
        if (pd->entries[i].inUse &&
            strcmp(pd->entries[i].name, leaf) == 0) {
            slot = i;
            break;
        }
    }

    // If the entry didn’t exist, close and report appropriately
    if (slot == UINT32_MAX) {
        fs_closedir(pd);
        errno = ENOENT;
        return -1;
    }

    // Capture where the file’s data begins and its total size
    uint32_t startBlk  = pd->entries[slot].location;
    uint32_t sizeBytes = pd->entries[slot].fileSize;

    // Unlink the entry logically before persisting
    pd->entries[slot].inUse = 0;
    writeDir(pd->entries, pd->block_count, pd->starting_block);
    fs_closedir(pd);

    // Compute how many blocks cover the file’s data
    uint32_t blkSize = g_vcb->blockSize;
    uint32_t nBlocks = (sizeBytes + blkSize - 1) / blkSize;

    // Gather those blocks into a list for release
    uint32_t *blocks = malloc(nBlocks * sizeof *blocks);
    if (!blocks) {

        // Fail gracefully if memory allocation fails
        return -1;
    }
    for (uint32_t i = 0; i < nBlocks; i++) {
        blocks[i] = startBlk + i;
    }

    // Re-use free-space manager setup to clear allocation bits
    FreeSpaceManager fsm;
    initFreeSpaceManager(&fsm, g_vcb, blkSize);
    releaseBlocks(&fsm, blocks, nBlocks);

    // Persist the updated bitmap so that space reuse is possible
    uint32_t bmpBytes  = (fsm.totalBlocks + 7) / 8;
    uint32_t bmpBytesR = ((bmpBytes + blkSize - 1) / blkSize) * blkSize;
    LBAwrite(fsm.bitmap, bmpBytesR / blkSize, g_vcb->bitmapStart);

    // Reflect the newly freed space in the in-memory VCB state
    g_vcb->freeBlockCount += nBlocks;

    // Free dynamic allocations to maintain memory hygiene
    free(fsm.bitmap);
    free(blocks);

    // Success communicates to the caller that deletion is complete
    return 0;
}

int fs_stat(const char *path, struct fs_stat *buf) {

    // Validate inputs so caller has both valid path and buffer to pervent crashes
    if (!path || !*path || !buf) {
        errno = EINVAL;
        return -1;
    }

    // Offer an immediate shortcut for the root directory so we don’t treat “/” as a normal
    // entry to avoid one off erros
    if (strcmp(path, "/") == 0) {

        // Load the root directory
        fdDir *root = fs_opendir("/");

        // If root dir doesnt exist return err to avoid unexpected functionality 
        if (!root) {
            return -1;
        }

        // Total size is blocks * blockSize
        buf->st_size = root->block_count * g_vcb->blockSize;
        buf->st_blksize = g_vcb->blockSize;
        buf->st_blocks = root->block_count;

        // We didn’t track timestamps for root in our VCB, so zero them or use time(NULL)
        buf->st_accesstime = buf->st_modtime = buf->st_createtime = 0;
        fs_closedir(root);

        // Return success
        return 0;
    }

    // Split the path into parent directory + name. How filesystem resolves path for operations.
    char parent[PATH_MAX], leaf[NAME_MAX];

    // If this fails then we return err to avoid unexpected errors
    if (split_path(path, parent, leaf) < 0) {
        errno = ENOENT;
        return -1;
    }

    // Allow callers to use “.” shortcut for current directory without forcing changes elsewhere
    if (strcmp(parent, ".") == 0) {
        strcpy(parent, current_working_directory);
    }

    // Open the parent directory to search for target entry
    fdDir *pd = fs_opendir(parent);

    // If it doesnt exist exit with failure
    if (!pd) {
        return -1;
    }

    // Find the matching entry by name
    directory_entry *de = NULL;
    
    // Search for the matching entry by name so we only process exactly specified targets,
    // preventing ambiguity in statting similarly named files
    for (uint32_t i = 0; i < pd->total_entries; i++) {
        if (pd->entries[i].inUse &&
            strcmp(pd->entries[i].name, leaf) == 0) {
            de = &pd->entries[i];
            break;
        }
    }

    // If no entry matched, close resources and inform caller of the missing file
    if (!de) {
        fs_closedir(pd);
        errno = ENOENT;
        return -1;
    }

    // Populate the fs_stat fields from the directory_entry
    buf->st_size = de->fileSize;
    buf->st_blksize = g_vcb->blockSize;

    // Compute the block count by rounding up to avoid partial-block inconsistencies
    buf->st_blocks = (de->fileSize + g_vcb->blockSize - 1) / g_vcb->blockSize;

    // Transfer timestamps so that consumers can make decisions based on actual I/O events
    buf->st_accesstime = de->accessed_at;
    buf->st_modtime = de->modified_at;
    buf->st_createtime = de->created_at;

    // Release directory handle now that all required information has been extracted
    fs_closedir(pd);

    // Return success
    return 0;
}