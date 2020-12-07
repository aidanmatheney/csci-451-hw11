/*
 * Aidan Matheney
 * aidan.matheney@und.edu
 *
 * CSCI 451 HW11 program 0
 */

#include "../include/hw11-program0.h"

#include <stdlib.h>
#include <stdio.h>

int main(int const argc, char ** const argv) {
    if (argc != 3) {
        fprintf(stderr, "Error: expected 2 arguments, got %d\n", argc - 1);
        return EXIT_FAILURE;
    }

    char * const inputFilePath = argv[1];
    char * const outputFilePath = argv[2];

    hw11Program0(
        inputFilePath,
        outputFilePath,
        "./program1",
        "./program2",
        "./program3"
    );
    return EXIT_SUCCESS;
}
