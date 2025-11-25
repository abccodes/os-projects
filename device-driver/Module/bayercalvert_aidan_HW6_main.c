/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Aidan Bayer-Calvert
* Student ID:: 922119403
* GitHub Name:: abccodes
* Project:: Assignment 6 device driver
*
* File:: bayercalvert_aidan_HW2_main.c
*
* Description:: Implements a loadable Linux kernel character device driver called main that
* exposes standard file operations (open, read, write, release) plus custom ioctl commands to
* perform Caesar‐cipher encryption and decryption on user-supplied strings. It handles safe data 
* transfer between user and kernel space and lets applications set the cipher key and mode at
* runtime.
**************************************************************/

// Include core kernel module definitions so the OS knows how to load, initialize, and teardown
// this code as a dynamically loadable component.
#include <linux/module.h>

// Define interfaces for registering file operations tied to our virtual device (open, read,
// write, ioctl, release).
#include <linux/fs.h>
#include <linux/cdev.h>


// Needed to move data between user applications and kernel space safely
#include <linux/uaccess.h>

// Provide constants and helpers to define new control commands (ioctl)
#include <linux/ioctl.h>

// After creating a device class, we need definitions for managing it in sysfs
#include <linux/device.h>

// Unique identifiers for distinguishing this device's commands. key acts as a namespace to avoid
// conflicting ioctl commands.
#define KEY 'A'

// Command for setting the cipher key
#define IOCTL_SET_KEY  _IOW(KEY, 0, int)

// Command for encrypt/decrypt mode
#define IOCTL_SET_MODE _IOW(KEY, 1, int)

// Default identities for our character device. Main is just an arbitrary name for this
#define DEVICE_NAME "main"
#define CLASS_NAME  "main"

// Keep track of which major number the kernel assigned to our device
static int majorNumber;

// Pointers to hold our device class and the device entry itself
static struct class*  mainClass  = NULL;
static struct device* mainDevice = NULL;

// Abstraction for kernel character devices we register one instance
static struct cdev mainCdev;

// Define a fixed-size internal buffer to hold the string being processed
#define BUF_LEN 1024
static char  kernel_buffer[BUF_LEN];
static size_t buf_len = 0;

// Parameters for the Caesar cipher: the shift key and whether to encrypt or decrypt
static int key = 3;

// 0=encrypt, 1=decrypt
static int mode = 0; 

// Read handler invoked when a process reads from /dev/main. Need to maintain a file offset
// so repeated reads work correctly. copy_to_user bridges kernel_buffer -> user application.
static ssize_t main_read(struct file *filp, char __user *usr_buf, size_t count, loff_t *offset) {
    // If all data has been read, signal end-of-file
    if (*offset >= buf_len) {
        return 0;
    }

    // Trim read size to what remains in our buffer
    if (count > buf_len - *offset) {
        count = buf_len - *offset;
    }

    // Send data to user space and error-check
    if (copy_to_user(usr_buf, kernel_buffer + *offset, count)) {
        return -EFAULT;
    }

    // Advance the offset so next read continues correctly
    *offset += count;

    return count;
}

// Write handler: invoked when a process writes to /dev/main. We grab the incoming string, store it,
// then apply our cipher in-place.
static ssize_t main_write(
    struct file *filp, 
    const char __user *usr_buf,
    size_t count,
    loff_t *offset
    ) {

    size_t i;
    char   ch;

    // Prevent overflow: cap to our buffer length minus one for null-terminator
    if (count > BUF_LEN - 1) {
        count = BUF_LEN - 1;
    }

    // Copy data from user-supplied buffer into kernel space
    if (copy_from_user(kernel_buffer, usr_buf, count)) {
        return -EFAULT;
    }

    buf_len = count;

    // Null terminate for saftey
    kernel_buffer[buf_len] = '\0';

    // Loop through each character to shift alphabetic bytes. Non-alphabetic remain unchanged.
    for (i = 0; i < buf_len; i++) {

        ch = kernel_buffer[i];

        if (ch >= 'A' && ch <= 'Z') {

            // Use the mode flag to reverse the shift if decrypting
            ch = (mode == 0)
               ? ((ch - 'A' + key) % 26) + 'A'
               : ((ch - 'A' - key + 26) % 26) + 'A';
        } else if (ch >= 'a' && ch <= 'z') {

            // Same logic for lowercase letters
            ch = (mode == 0)
               ? ((ch - 'a' + key) % 26) + 'a'
               : ((ch - 'a' - key + 26) % 26) + 'a';
        }
        kernel_buffer[i] = ch;
    }

    // Indicate all bytes were "written"
    return count;
}

// Ioctl handler: used to configure key and mode on-the-fly without reopening the device.
static long main_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    
    int value;

    // Read the integer argument provided by the application
    if (copy_from_user(&value, (int __user *)arg, sizeof(value))) {
        return -EFAULT;
    }

    switch (cmd) {
        case IOCTL_SET_KEY:

            // Keep shifts within alphabet range
            key = value % 26;
            break;

        case IOCTL_SET_MODE:

            // 0=encrypt, 1=decrypt
            mode = value;
            break;

        default:

            // Unknown command
            return -EINVAL;
    }
    return 0;
}

// Open and release exist so the kernel can manage concurrent access. we log these to help diagnose
// usage patterns.
static int main_open(struct inode *inodep, struct file *filep) {

    // Display to console to view
    printk(KERN_INFO "main: device opened\n");
    return 0;
}

static int main_release(struct inode *inodep, struct file *filep)
{   
    // Display to console to view
    printk(KERN_INFO "main: device closed\n");
    return 0;
}

// Our handlers into a single dispatch table. This tells the OS which function to call for each file
// op.
static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = main_read,
    .write = main_write,
    .unlocked_ioctl = main_ioctl,
    .open = main_open,
    .release = main_release,
};

// init module. Register our character device, create a device class, and make an entry under
// /dev.
static int __init main_init(void) {

    // Ask the kernel to allocate a free major number for our device, using DEVICE_NAME so
    // userspace can reference it by name
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);

    // If allocation fails (negative error code), propagate that error back to the loader so it
    // knows why initialization aborted
    if (majorNumber < 0) {
        return majorNumber;
    }

    // Create a device class under /sys/class so udev (or mdev) can automatically create the /dev
    // node for us
    mainClass = class_create(CLASS_NAME);

    // class_create returns an error‐encoded pointer on failure
    if (IS_ERR(mainClass)) {

        // Undo the char‐device registration since class creation failed
        unregister_chrdev(majorNumber, DEVICE_NAME);

        // Convert the pointer to an integer error code and return it
        return PTR_ERR(mainClass);
    }

    // Actually instantiate the device node (/dev/main) so applications can open it by name. MKDEV
    // combines our major and minor (0).
    mainDevice = device_create(mainClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);

    // Check for errors again and clean up both the class and char‐device registration if
    // device_create failed.
    if (IS_ERR(mainDevice)) {
        class_destroy(mainClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        return PTR_ERR(mainDevice);
    }

    // Initialize our cdev structure to point at our file operations and then add it to the
    // kernel’s internal list so it can route calls.
    cdev_init(&mainCdev, &fops);
    cdev_add(&mainCdev, MKDEV(majorNumber, 0), 1);

    // Return zero to indicate to the kernel that setup completed successfully
    return 0;
}

// Module cleanup: undo everything done in init in reverse order
static void __exit main_exit(void) {
    cdev_del(&mainCdev);
    device_destroy(mainClass, MKDEV(majorNumber, 0));
    class_destroy(mainClass);
    unregister_chrdev(majorNumber, DEVICE_NAME);
}

// Macros to identify our init and exit functions to the kernel.
module_init(main_init);
module_exit(main_exit);

// Needed to add this to fix error. Online: "declare a driver as GPL-compatible so the
// kernel will allow it to use symbols exported only to GPL modules and avoid tainting the system"
MODULE_LICENSE("GPL");