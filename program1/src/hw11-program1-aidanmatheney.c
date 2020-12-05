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
    if (argc != 3) {
        fprintf(stderr, "Error: expected 2 arguments, got %d\n", argc - 1);
        return EXIT_FAILURE;
    }

    char * const semaphoreName = argv[1];
    char * const pipe1WriteFileDescriptorString = argv[2];

    int pipe1WriteFileDescriptor = (int)strtol(pipe1WriteFileDescriptorString, NULL, 10);

    hw11Program1(
        "./input.data",
        semaphoreName,
        pipe1WriteFileDescriptor
    );
    return EXIT_SUCCESS;
}
