/*
 * Aidan Matheney
 * aidan.matheney@und.edu
 *
 * CSCI 451 HW11 program 2
 */

#include "../include/hw11-program2.h"

#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>

int main(int const argc, char ** const argv) {
    if (argc != 6) {
        fprintf(stderr, "Error: expected 5 arguments, got %d\n", argc - 1);
        return EXIT_FAILURE;
    }

    char * const pipe1ReadFileDescriptorString = argv[1];
    char * const pipe2WriteFileDescriptorString = argv[2];
    char * const semaphore1Name = argv[3];
    char * const semaphore2Name = argv[4];
    char * const sharedMemoryKeyString = argv[5];

    int pipe1ReadFileDescriptor = (int)strtol(pipe1ReadFileDescriptorString, NULL, 10);
    int pipe2WriteFileDescriptor = (int)strtol(pipe2WriteFileDescriptorString, NULL, 10);
    key_t sharedMemoryKey = (int)strtol(sharedMemoryKeyString, NULL, 10);

    hw11Program2(
        pipe1ReadFileDescriptor,
        pipe2WriteFileDescriptor,
        semaphore1Name,
        semaphore2Name,
        sharedMemoryKey
    );
    return EXIT_SUCCESS;
}
