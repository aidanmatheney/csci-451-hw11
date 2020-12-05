/*
 * Aidan Matheney
 * aidan.matheney@und.edu
 *
 * CSCI 451 HW11 program 3
 */

#include "../include/hw11-program3.h"

#include <stdlib.h>
#include <stdio.h>

int main(int const argc, char ** const argv) {
    if (argc != 2) {
        fprintf(stderr, "Error: expected 1 argument, got %d\n", argc - 1);
        return EXIT_FAILURE;
    }

    char * const pipe2ReadFileDescriptorString = argv[1];

    int pipe2ReadFileDescriptor = (int)strtol(pipe2ReadFileDescriptorString, NULL, 10);

    hw11Program3(
        "./shared1.dat",
        "./shared2.dat",
        "./output.data",
        pipe2ReadFileDescriptor
    );
    return EXIT_SUCCESS;
}
