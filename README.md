# OS Projects - Operating Systems Course Assignments

A collection of Operating Systems (San Francisco State University, CSC-415) programming assignments completed in C on Ubuntu Linux.

---

## Important Disclaimer

**This repository is for personal use and educational reference only.**

If you are currently enrolled in the course:
- You may use this repository **as a reference** to understand concepts and approaches
- **Do not copy code directly** - this constitutes academic dishonesty
- If you do reference or adapt any code, **you must cite this repository** properly

---

## Assignments Overview

| Assignment | Folder | Description |
|------------|--------|-------------|
| HW1 | `commandline` | Command Line Arguments - Introduction to C programming and command line argument handling |
| HW2 | `bufferandstruct` | Buffering and Structures - Working with pointers, structures, and block operations |
| HW3 | `simpleshell` | Simple Shell with Pipes - Process creation using fork, exec, wait, and pipe implementation |
| HW4 | `policedatathreads` | Processing Fixed Length Record (FLR) Data with Threads - Multi-threaded data processing with mutex locks |
| HW5 | `buffered-io` | Buffered I/O - Implementing buffered read operations for file handling |
| HW6 | `device-driver` | Device Driver - Linux kernel module development with user/application interaction |
| Group Project | `filesystem` | File System - Complete file system implementation with directory structures and file operations |

---

## Build Instructions

Each assignment folder contains its own `Makefile`. To build and run:

```
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
