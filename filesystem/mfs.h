/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Arric Sekhon, Aidan Bayer-Calvert, Fabian Camarena Veliz, and Subhan Khan
* Student IDs:: 923090490, 922119403, 924100096, and 921938807
* GitHub-Name:: FlavyFabo
* Group-Name:: AAXF
* Project:: Basic File System
*
* File:: mfs.h
*
* Description:: 
*	This is the file system interface.
*	This is the interface needed by the driver to interact with
*	your filesystem.
*
**************************************************************/


#ifndef _MFS_H
#define _MFS_H
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <limits.h> 
#include "directory.h"

#include "b_io.h"

#include <dirent.h>

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifndef DT_REG
#define DT_REG 8
#endif

#ifndef DT_DIR
#define DT_DIR 4
#endif

#ifndef DT_LNK
#define DT_LNK 10
#endif

// File type identifiers for our file system
#define FT_REGFILE  DT_REG
#define FT_DIRECTORY DT_DIR
#define FT_LINK     DT_LNK

#ifndef uint64_t
typedef u_int64_t uint64_t;
#endif
#ifndef uint32_t
typedef u_int32_t uint32_t;
#endif

// This structure is returned by fs_readdir to provide the caller with information
// about each file as it iterates through a directory
struct fs_diriteminfo
	{
    unsigned short d_reclen;    /* length of this record */
    unsigned char fileType;    
    char d_name[256]; 			/* filename max filename is 255 characters */
	};

// This is a private structure used only by fs_opendir, fs_readdir, and fs_closedir
// Think of this like a file descriptor but for a directory - one can only read
// from a directory.  This structure helps you (the file system) keep track of
// which directory entry you are currently processing so that everytime the caller
// calls the function readdir, you give the next entry in the directory
typedef struct {
    directory_entry *entries;     // Pointer to loaded directory entries
    uint32_t total_entries;       // Total number of entries loaded
    uint32_t current_pos;         // Current position for iteration (fs_readdir)
    uint32_t block_count;         // Number of blocks occupied by directory
    uint32_t starting_block;      // Starting LBA of directory on disk
} fdDir;

// Key directory functions
int fs_mkdir(const char *pathname, mode_t mode);
int fs_rmdir(const char *pathname);

// Directory iteration functions
fdDir * fs_opendir(const char *pathname);
struct fs_diriteminfo *fs_readdir(fdDir *dirp);
int fs_closedir(fdDir *dirp);

// Misc directory functions
char * fs_getcwd(char *pathname, size_t size);
int fs_setcwd(char *pathname);   //linux chdir
// int fs_isFile(char * filename);	//return 1 if file, 0 otherwise
// int fs_isDir(char * pathname);		//return 1 if directory, 0 otherwise
int fs_isFile(const char * filename);  
int fs_isDir(const char * pathname);   
int fs_delete(char* filename);	//removes a file


// This is the strucutre that is filled in from a call to fs_stat
struct fs_stat
	{
	off_t     st_size;    		/* total size, in bytes */
	blksize_t st_blksize; 		/* blocksize for file system I/O */
	blkcnt_t  st_blocks;  		/* number of 512B blocks allocated */
	time_t    st_accesstime;   	/* time of last access */
	time_t    st_modtime;   	/* time of last modification */
	time_t    st_createtime;   	/* time of last status change */
	
	/* add additional attributes here for your file system */
	};

int fs_stat(const char *path, struct fs_stat *buf);

#endif

