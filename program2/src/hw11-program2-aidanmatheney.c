/*
 * Aidan Matheney
 * aidan.matheney@und.edu
 *
 * CSCI 451 HW11 program 2
 */

#include "../include/hw11-program2.h"

#include <stdlib.h>
#include <stdio.h>

int main(int const argc, char ** const argv) {
    if (argc != 4) {
        fprintf(stderr, "Error: expected 3 arguments, got %d\n", argc - 1);
        return EXIT_FAILURE;
    }

    char * const semaphoreName = argv[1];
    char * const pipe1ReadFileDescriptorString = argv[2];
    char * const pipe2WriteFileDescriptorString = argv[3];

    int pipe1ReadFileDescriptor = (int)strtol(pipe1ReadFileDescriptorString, NULL, 10);
    int pipe2WriteFileDescriptor = (int)strtol(pipe2WriteFileDescriptorString, NULL, 10);

    hw11Program2(
        "./shared1.dat",
        "./shared2.dat",
        semaphoreName,
        pipe1ReadFileDescriptor,
        pipe2WriteFileDescriptor
    );
    return EXIT_SUCCESS;
}
