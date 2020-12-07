/*
 * Aidan Matheney
 * aidan.matheney@und.edu
 *
 * CSCI 451 HW11 program 3
 */

#include "../include/hw11-program3.h"

#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>

int main(int const argc, char ** const argv) {
    if (argc != 5) {
        fprintf(stderr, "Error: expected 4 argument, got %d\n", argc - 1);
        return EXIT_FAILURE;
    }

    char * const outputFilePath = argv[1];
    char * const pipe2ReadFileDescriptorString = argv[2];
    char * const semaphore2Name = argv[3];
    char * const sharedMemoryKeyString = argv[4];

    int pipe2ReadFileDescriptor = (int)strtol(pipe2ReadFileDescriptorString, NULL, 10);
    key_t sharedMemoryKey = (int)strtol(sharedMemoryKeyString, NULL, 10);

    hw11Program3(
        outputFilePath,
        pipe2ReadFileDescriptor,
        semaphore2Name,
        sharedMemoryKey
    );
    return EXIT_SUCCESS;
}
