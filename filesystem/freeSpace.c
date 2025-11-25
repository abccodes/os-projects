/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Arric Sekhon, Aidan Bayer-Calvert, Fabian Camarena Veliz, and Subhan Khan
* Student IDs:: 923090490, 922119403, 924100096, and 921938807
* GitHub-Name:: FlavyFabo
* Group-Name:: AAXF
* Project:: Basic File System
*
* File:: freeSpace.c
*
* Description:: This .c file is part of a basic file system, and it's 
*               used to manage which blocks of disk space are free or 
*               used. It sets up a bitmap in memory to track all blocks, 
*               lets you reserve space (allocate), and later frees it 
*               when you're done (release). It also updates the actual 
*               disk so your changes are saved.
*
**************************************************************/

#include <stdint.h>          
#include <limits.h>          
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "fsLow.h"
#include "vcb.h"
#include "freeSpace.h"

// Global volume control block defined in fsinit
extern VCB *g_vcb;

// init free space bitmap in memory and persist it
void initFreeSpaceManager(FreeSpaceManager *fsm, VCB *vcb, uint32_t blkSize) 
{   
    // Avoid undefined behavior if passed uninitialized pointers
    if (!fsm || !vcb) {
        return;
    }

    // Record how many blocks the volume contains and the block size for consistency
    fsm->totalBlocks = vcb->totalBlocks;
    fsm->blockSize   = blkSize;

    // Compute the raw number of bytes needed for the bitmap, rounding up bits-to-bytes
    uint32_t bmpBytes  = (fsm->totalBlocks + 7) / 8;

    // Round that up to whole blocks so we can read/write in block multiples
    uint32_t bmpBytesR = ((bmpBytes + blkSize - 1) / blkSize) * blkSize;

    // Store the actual byte length separately from the rounded length
    fsm->bitmapSize = bmpBytes;

    // Allocate the in-memory bitmap, zeroed to mark all blocks initially free
    fsm->bitmap     = calloc(bmpBytesR, 1);
    if (!fsm->bitmap) {
        // Report an out-of-memory condition, since FS cannot function without this
        fprintf(stderr, "initFreeSpaceManager: out of memory\n");
        return;
    }

    // Mark the reserved region (VCB, bitmap blocks, root directory, etc.) as used
    for (uint64_t b = 0; b < vcb->freeBlockStart; ++b)
        fsm->bitmap[b >> 3] |= (1 << (b & 7));

    // Persist the initialized bitmap to disk so allocation state is durable
    uint32_t blks = bmpBytesR / blkSize;
    LBAwrite(fsm->bitmap, blks, vcb->bitmapStart);

}

// Helper to mark a contiguous run of blocks as in-use in the bitmap
static int mark_run_used(FreeSpaceManager *fsm, uint32_t start, uint32_t len, uint32_t *out)
{
    // By iterating exactly len times we guarantee contiguous allocation
    for (uint32_t i = 0; i < len; i++) {
        uint32_t b = start + i;

        // Optionally return the block numbers to the caller for tracking
        if (out) {
            out[i] = b;
        }

        // Set the corresponding bit to mark the block as allocated
        fsm->bitmap[b >> 3] |= (1 << (b & 7));
    }

    // Return success
    return 0;
}

// Allocate a sequence of  count free blocks, update bitmap and write out
int allocateBlocks(FreeSpaceManager *fsm, uint32_t count, uint32_t *out)
{
    // Avoid undefined behavior by checking if args exist correctly
    if (!fsm || !fsm->bitmap) {
        return -1;
    }

    uint32_t consec = 0;

    // Walk the bitmap looking for a run of unset bits of length count
    for (uint32_t i = 0; i < fsm->totalBlocks; ++i) {

        // Test the bit. non-zero means used, zero means free
        int used = (fsm->bitmap[i >> 3] & (1 << (i & 7))) != 0;

        // Reset on used, increment on free to track consecutive free run
        consec = used ? 0 : consec + 1;

        // Once we see enough, allocate that run starting at (i+1-count)
        if (consec == count) {
            uint32_t start = i + 1 - count;
            mark_run_used(fsm, start, count, out);

            // Flush the modified bitmap back to disk immediately to maintain consistency
            uint32_t blks = (fsm->bitmapSize + fsm->blockSize - 1) / fsm->blockSize;
            LBAwrite(fsm->bitmap, blks, g_vcb->bitmapStart);

            // Return success
            return 0;
        }
    }

    // If we exit the loop without finding a run, signal out-of-space
    return -1;
}

// Free previously allocated blocks so they can be reused later
int releaseBlocks(FreeSpaceManager *fsm, uint32_t *blockList, uint32_t count)
{
    // Avoid undefined behavior by checking if args exist correctly
    if (!fsm || !fsm->bitmap) {
        return -1;
    }

    // Iterate only the blocks the caller recorded, avoiding a full scan
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t b = blockList[i];

        // ignore out of range to avoid out of range errors
        if (b >= fsm->totalBlocks) {
            continue;
        }

        // Clear the bit to mark the block as free again
        fsm->bitmap[b >> 3] &= ~(1 << (b & 7));
    }

    // Immediately persist changes so free-space is visible on-disk
    uint32_t blks = (fsm->bitmapSize + fsm->blockSize - 1) / fsm->blockSize;
    LBAwrite(fsm->bitmap, blks, g_vcb->bitmapStart);

    // Success
    return 0;
}

// Allocate exactly one free block or fail for easy readability instead of calling allocate blocks
// over and over.
uint32_t alloc_one_block(FreeSpaceManager *fsm)
{
    uint32_t blk;

    // Reuse the multi-block allocator with count=1 for simplicity
    return (allocateBlocks(fsm, 1, &blk) == 0) ? blk : UINT32_MAX;
}
