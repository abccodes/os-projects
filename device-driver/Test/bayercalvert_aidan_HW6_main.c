/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Aidan Bayer-Calvert
* Student ID:: 922119403
* GitHub Name:: abccodes
* Project:: Assignment 6 device driver
*
* File:: bayercalvert_aidan_HW2_main.c
*
* Description:: This is the user‐space test application for the Caesar‐cipher kernel driver
* exported at /dev/main. It parses command‐line arguments, configures the driver via ioctl,
* writes the input string, reads back the transformed output, and prints the result.
**************************************************************/

// headers for console I/O and system calls
// printf, fprintf, perror
#include <stdio.h>

// exit, EXIT_FAILURE
#include <stdlib.h>

// open flags: O_RDWR, etc.
#include <fcntl.h>

// close, read, write
#include <unistd.h>

// ioctl system call and helpers
#include <sys/ioctl.h>

// strlen, strcpy, memset, etc.
#include <string.h>


// The path used by our driver; matches the DEVICE_NAME above.
#define DEVICE "/dev/main"

// Repeat the ioctl command definitions so user code and kernel match. Arbitrary values for Key,
// A.
#define KEY 'A'
#define IOCTL_SET_KEY  _IOW(KEY, 0, int)
#define IOCTL_SET_MODE _IOW(KEY, 1, int)

// Main method holding all main magic. argc/argv to read args
int main(int argc, char *argv[]) {

    // Stores file descriptors
    int fd;

    // Captures returnCodesurn codes from ioctl
    int returnCodes; 

    // Key and mode hold cipher parameters
    int key;
    int mode;

    // Points to the string to encrypt/decrypt
    const char *input;

    // Fixed buffer to read result back. 1024 is an arbitrary number for the size
    char output[1024];

    // Tracks how many bytes were written or read for error checks
    ssize_t bytes;

    // Ensure the user passed exactly three arguments: mode(e/d), key, and string
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <e|d> <key> <string>\n", argv[0]);
        exit(1);
    }

    // Convert first argument into the integer flags understood by the driver
    mode = (argv[1][0] == 'e') ? 0 : 1;
    key = atoi(argv[2]);
    input = argv[3];

    // Open a handle to /dev/main for reading and writing
    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        exit(1);
    }

    // Configure the driver’s key and mode via ioctl
    returnCodes = ioctl(fd, IOCTL_SET_KEY, &key);
    if (returnCodes < 0) {
        perror("IOCTL_SET_KEY");
        close(fd);
        exit(1);
    }

    returnCodes = ioctl(fd, IOCTL_SET_MODE, &mode);
    if (returnCodes < 0) {
        perror("IOCTL_SET_MODE");
        close(fd);
        exit(1);
    }

    // Send the input string into the driver for processing.
    bytes = write(fd, input, strlen(input));
    if (bytes < 0) {
        perror("Write failed");
        close(fd);
        exit(1);
    }

    // Read back the transformed string into our local buffer
    bytes = read(fd, output, sizeof(output));
    if (bytes < 0) {
        perror("Read failed");
        close(fd);
        exit(1);
    }

    // Null termination to avoid issues before printing
    output[bytes] = '\0';

    // Show user final text
    printf("Result: %s\n", output);

    // Always close file descriptors when done.
    close(fd);

    // Exit with success
    return 0;
}