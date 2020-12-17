/**************************************************************
* Class:  CSC-415-0# Fall 2020
* Name: Roberto Herman
* Student ID: 918009734
* Project: Assignment 6 – Device Driver
* GitHub: mecosteas
*
* File: herman_roberto_HW6_main.c
*
* Description: Program to demonstrate how my device driver works.
* You give it a string and it should reverse it, then it computes
* the nth term of the Fibonacci series.
*
**************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define INPUT_NTH _IOW('a', 1, unsigned long *)  // used for ioctl read
#define OUTPUT_NTH _IOR('a', 2, unsigned long *) // used for ioctl write
#define BUFFER_LENGTH 256

static char reverseName[BUFFER_LENGTH]; // The reversed string buffer from the LKM
unsigned long nthTerm, fibAnswer;

int main()
{
    int ret, fd;
    char userName[BUFFER_LENGTH];
    printf("\nStarting myLKM demo...\n");
    fd = open("/dev/myLKM", O_RDWR);
    if (fd < 0)
    {
        perror("--- Failed to open the device...");
        return errno;
    }
    printf("\nWhat is your name? ");
    scanf("%[^\n]%*c", userName); // Read in a string (with spaces)
    printf("\n>>> Writing [%s] to the device.\n", userName);
    ret = write(fd, userName, strlen(userName)); // Send the string to the LKM
    if (ret < 0)
    {
        perror("--- Failed to write the message to the device.");
        return errno;
    }

    printf("<<< Reading from the device...\n");
    ret = read(fd, reverseName, BUFFER_LENGTH); // Read the response from the LKM
    if (ret < 0)
    {
        perror("--- Failed to read the message from the device.");
        return errno;
    }

    // print the returned string from LKM
    printf("\nDid you know that your name backwards is '%s'?\n", reverseName);

    // fibonacci number ioctl demo
    printf("\nNow let's see what the nth term of the Fibonacci sequence is.\n");
    printf("Enter the nth term you'd like to get from the Fibonacci sequence: ");
    scanf("%lu", &nthTerm);

    printf("\n>>> Writing the %luth term to the device...\n", nthTerm);
    ioctl(fd, INPUT_NTH, (unsigned long *)&nthTerm);

    printf("<<< Reading the %luth term from the device...\n", nthTerm);
    ioctl(fd, OUTPUT_NTH, (unsigned long *)&fibAnswer);

    printf("\nThe %luth term of the Fibonacci sequence is: %lu\n", nthTerm, fibAnswer);

    printf("\nThat's it for my Linux Kernel Module (device driver),"
           "have a wonderful winter break.\n");
    printf("Goodbye!\n");

    printf("Closing driver\n");
    close(fd);

    return 0;
}