# OS Projects - Operating Systems Course Assignments

A collection of Operating Systems (CSC-415) programming assignments completed in C on Ubuntu Linux.

---

## ⚠️ Important Disclaimer

**This repository is for personal use and educational reference only.**

If you are currently enrolled in the course:
- You may use this repository **as a reference** to understand concepts and approaches
- **Do not copy code directly** - this constitutes academic dishonesty
- If you do reference or adapt any code, **you must cite this repository** properly

### How to Cite

If you use any code or concepts from this repository, include a citation in your source code:

```c
// Code referenced/adapted from: https://github.com/abccodes/os-projects
// Specific file: [filename]
// Accessed: [date]
```

---

## Assignments Overview

| Assignment | Folder | Description |
|------------|--------|-------------|
| HW1 | `commandline` | Command Line Arguments - Introduction to C programming and command line argument handling |
| HW2 | `bufferandstruct` | Buffering and Structures - Working with pointers, structures, and block operations |
| HW3 | `simpleshell` | Simple Shell with Pipes - Process creation using fork, exec, wait, and pipe implementation |
| HW4 | `policedatathreads` | Processing FLR Data with Threads - Multi-threaded data processing with mutex locks |
| HW5 | `buffered-io` | Buffered I/O - Implementing buffered read operations for file handling |
| HW6 | `device-driver` | Device Driver - Linux kernel module development with user/application interaction |
| Group Project | `filesystem` | File System - Complete file system implementation with directory structures and file operations |

---

## Assignment Details

### Assignment 1 - Command Line Arguments (`commandline`)
Introduction to C programming and the Linux development environment. Demonstrates parsing and displaying command line arguments.

### Assignment 2 - Buffering and Structures (`bufferandstruct`)
Covers memory management, pointers, character strings, enumerated types, bitmap fields, and buffering data into blocks. Includes hexdump analysis.

### Assignment 3 - Simple Shell (`simpleshell`)
Implementation of a basic shell that supports:
- Command execution via fork/exec/wait
- Piping between processes
- Custom prompt configuration

### Assignment 4 - Police Data Threads (`policedatathreads`)
Multi-threaded processing of Fixed Length Record (FLR) data files. Features:
- Thread-safe data structures
- Statistical calculations (median, quartiles, IQR, standard deviation)
- Mutex locks for critical sections

### Assignment 5 - Buffered I/O (`buffered-io`)
Implementation of buffered file I/O operations:
- `b_open` - Open files with file descriptor tracking
- `b_read` - Buffered reading with chunk management
- `b_close` - Resource cleanup

### Assignment 6 - Device Driver (`device-driver`)
Linux kernel module development including:
- Loadable device driver implementation
- Open, release, read, write, and ioctl operations
- User-space application interaction

### File System Group Project (`filesystem`)
Comprehensive file system implementation featuring:
- Volume formatting and initialization
- Free space management
- Directory operations (mkdir, rmdir, opendir, readdir, etc.)
- File operations (open, read, write, seek, close)
- Shell interface for file system interaction

---

## Build Instructions

Each assignment folder contains its own `Makefile`. To build and run:

```bash
cd <assignment-folder>
make          # Compile the program
make run      # Execute the program (may require RUNOPTIONS)
make clean    # Clean build artifacts
```

---

## Technologies

- **Language:** C
- **Platform:** Ubuntu Linux (Virtual Machine)
- **Build System:** Make
- **Threading:** POSIX Threads (pthreads)
- **Kernel Development:** Linux Kernel Module Framework

---

## License

This project is for educational purposes. See individual assignment folders for specific licensing information where applicable.

---

*Remember: Learning comes from understanding, not copying. Use this as a guide to develop your own solutions.*
