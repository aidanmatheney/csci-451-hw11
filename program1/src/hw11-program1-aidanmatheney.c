/*
 * Aidan Matheney
 * aidan.matheney@und.edu
 *
 * CSCI 451 HW11 program 1
 */

#include "../include/hw11-program1.h"

#include <stdlib.h>
#include <stdio.h>

int main(int const argc, char ** const argv) {
    if (argc != 4) {
        fprintf(stderr, "Error: expected 3 arguments, got %d\n", argc - 1);
        return EXIT_FAILURE;
    }

    char * const inputFilePath = argv[1];
    char * const pipe1WriteFileDescriptorString = argv[2];
    char * const semaphore1Name = argv[3];

    int pipe1WriteFileDescriptor = (int)strtol(pipe1WriteFileDescriptorString, NULL, 10);

    hw11Program1(
        inputFilePath,
        pipe1WriteFileDescriptor,
        semaphore1Name
    );
    return EXIT_SUCCESS;
}
