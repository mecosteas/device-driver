/**************************************************************
* Class:  CSC-415-0# Fall 2020
* Name: Roberto Herman
* Student ID: 918009734
* Project: Assignment 6 – Device Driver
* GitHub: mecosteas
*
* File: myLKM.c
*
* Description: A char driver. The user writes their name to it
* and they can read their name backwards from the device. Then,
* they can write the nth term of the Fibonacci sequence and
* read back the result of the nth term.
*
**************************************************************/

#include <linux/init.h>    // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>  // Core header for loading LKMs into the kernel
#include <linux/device.h>  // Header to support the kernel Driver Model
#include <linux/kernel.h>  // Contains types, macros, functions for the kernel
#include <linux/fs.h>      // Header for the Linux file system support
#include <linux/uaccess.h> // Required for the copy to user function
#include <linux/ioctl.h>

// #define "ioctl name" __IOX("magic number","command number","argument type")
#define INPUT_NTH _IOW('a', 1, unsigned long *)
#define OUTPUT_NTH _IOR('a', 2, unsigned long *)

#define DEVICE_NAME "myLKM"  // The device will appear at /dev/myLKM using this value
#define CLASS_NAME "LKMchar" // The device class -- this is a character device driver

MODULE_LICENSE("GPL");           // The license type -- this affects available functionality
MODULE_AUTHOR("Roberto Herman"); // The author -- visible when you use modinfo
MODULE_DESCRIPTION("This module computes the nth result of the Fibonacci series,\n"
                   "as well as reverse a given string.\n"); // The description -- see modinfo
MODULE_VERSION("0.1");                                      // A version number to inform users

static int majorNumber;      // Stores the device number -- determined automatically
static char name[256] = {0}; // Memory for the string that is passed from userspace
static char reverseName[256] = {0};
static unsigned long fibAnswer = 0;
static unsigned long nthTerm = 0;
static short size_of_name;  // Used to remember the size of the name stored
static int numberOpens = 0; // Counts the number of times the device is opened
static struct class *myLKMclass = NULL;
static struct device *myLKMdevice = NULL;

// The prototype functions for the character driver -- must come before the struct definition
static int openMyLKM(struct inode *, struct file *);
static int releaseMyLKM(struct inode *, struct file *);
static ssize_t readFromMyLKM(struct file *, char *, size_t, loff_t *);
static ssize_t writeToMyLKM(struct file *, const char *, size_t, loff_t *);
static long myLKMioctl(struct file *, unsigned int cmd, unsigned long arg);

static struct file_operations fops =
    {
        .open = openMyLKM,
        .read = readFromMyLKM,
        .write = writeToMyLKM,
        .release = releaseMyLKM,
        .unlocked_ioctl = myLKMioctl,
};

/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init initMyLKM(void)
{
    printk(KERN_INFO "myLKM: Initializing the myLKM device\n");

    // Try to dynamically allocate a major number for the device -- more difficult but worth it
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0)
    {
        printk(KERN_ALERT "myLKM failed to register a major number\n");
        return majorNumber;
    }
    printk(KERN_INFO "myLKM: registered correctly with major number %d\n", majorNumber);

    // Register the device class
    myLKMclass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(myLKMclass))
    { // Check for error and clean up if there is
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(myLKMclass); // Correct way to return an error on a pointer
    }
    printk(KERN_INFO "myLKM: device class registered correctly\n");

    // Register the device driver
    myLKMdevice = device_create(myLKMclass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(myLKMdevice))
    {                              // Clean up if there is an error
        class_destroy(myLKMclass); // Repeated code but the alternative is goto statements
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(myLKMdevice);
    }
    printk(KERN_INFO "myLKM: device class created correctly\n");
    return 0;
}

/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit exitMyLKM(void)
{
    device_destroy(myLKMclass, MKDEV(majorNumber, 0)); // remove the device
    class_unregister(myLKMclass);                      // unregister the device class
    class_destroy(myLKMclass);                         // remove the device class
    unregister_chrdev(majorNumber, DEVICE_NAME);       // unregister the major number
    printk(KERN_INFO "myLKM: Goodbye from the LKM!\n");
}

/** @brief The device open function that is called each time the device is opened.
 *         This will only increment the numberOpens counter in this case.
 */
static int openMyLKM(struct inode *inodep, struct file *filep)
{
    numberOpens++;
    printk(KERN_INFO "myLKM: Device has been opened %d time(s)\n", numberOpens);
    return 0;
}

/** @brief Sends the user their reversed name
 *  @return 0 upon success
 */
static ssize_t readFromMyLKM(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
    int error_count = 0;
    // copy_to_user has the format ( * to, *from, size) and returns 0 on success
    error_count = copy_to_user(buffer, reverseName, size_of_name);

    if (error_count == 0)
    { // if true then have success
        printk(KERN_INFO "myLKM: Sent %lu bytes to the user\n", sizeof(int));
        return 0; // clear the position to the start and return 0
    }
    else
    {
        printk(KERN_INFO "myLKM: Failed to send %d characters to the user\n", error_count);
        return -EFAULT; // Failed -- return a bad address message (i.e. -14)
    }
}
/**
 * @brief  writes from given userName to our name variable and reverses it into reverseName
 * @return length of bytes written
 */
static ssize_t writeToMyLKM(struct file *filep, const char *userName, size_t len, loff_t *offset)
{
    int end, start;

    copy_from_user(&name, userName, len);
    size_of_name = len;

    // reverse their name
    for (end = len - 1, start = 0; start < len; start++, end--)
    {
        reverseName[start] = name[end];
    }

    printk(KERN_INFO "myLKM: Received %zu characters from the user\n", len);
    printk(KERN_INFO "myLKM: Reversed [%s] to [%s]\n", name, reverseName);
    return len;
}

/**
 * @brief when cmd is INPUT_NTH, it stores the given arg into nthTerm.
 *        when cmd is OUTPUT_NTH, it calculates the nth term of the Fibonacci
 *        sequence and copies it to the user's arg
 * @return 0 uppon success
 */
static long myLKMioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd)
    {
    case INPUT_NTH:
        copy_from_user(&nthTerm, (unsigned long *)arg, sizeof(nthTerm));
        printk(KERN_INFO "nth term given: %lu\n", nthTerm);
        break;

    case OUTPUT_NTH:
        // i am putting this empty statement here, otherwise I get the error:
        // error: a label can only be part of a statement and a declaration is not a statement
        ;
        // end of empty statement

        int error_count = 0;
        int i = 0;
        // array stores fibonacci numbers
        int fib[nthTerm + 2]; // 1 extra to handle case, n = 0

        // first 2 numbers of sequence
        fib[0] = 0;
        fib[1] = 1;

        // from 2 to nth term
        for (i = 2; i <= nthTerm; i++)
        {
            // accumulate previous 2 values
            fib[i] = fib[i - 1] + fib[i - 2];
        }

        fibAnswer = fib[nthTerm];
        printk(KERN_INFO "Answer computed: %lu", fibAnswer);

        // copy_to_user has the format ( * to, *from, size) and returns 0 on success
        error_count = copy_to_user((unsigned long *)arg, &fibAnswer, sizeof(fibAnswer));
        if (error_count != 0)
        {
            printk(KERN_INFO "myLKM: Failed to send %d characters to the user\n", error_count);
            return -EFAULT; // Failed -- return a bad address message (i.e. -14)
        }
        break;
    }

    return 0;
}

static int releaseMyLKM(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "myLKM: Device successfully closed\n");
    return 0;
}

module_init(initMyLKM);
module_exit(exitMyLKM);