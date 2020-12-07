#include "../include/hw11-program1.h"

#include "../include/util/StringBuilder.h"

#include "../include/util/file.h"
#include "../include/util/semaphore.h"
#include "../include/util/guard.h"

#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>

/**
 * Run CSCI 451 HW11 program 1.
 */
void hw11Program1(
    char const * const inputFilePath,
    int const pipe1WriteFileDescriptor,
    char const * const semaphore1Name
) {
    guardNotNull(inputFilePath, "inputFilePath", "hw11Program1");
    guardNotNull(semaphore1Name, "semaphore1Name", "hw11Program1");

    FILE * const inputFile = safeFopen(inputFilePath, "r", "hw11Program1");
    FILE * const pipe1WriteFile = safeFdopen(pipe1WriteFileDescriptor, "w", "hw11Program1");
    sem_t * const semaphore1 = safeSemOpen(semaphore1Name, "hw11Program1");

    // semaphore1 is already locked

    while (true) {
        StringBuilder const currentWordBuilder = StringBuilder_create();
        bool atEof = true;

        char currentCharacter;
        while (safeFgetc(&currentCharacter, inputFile, "hw11Program1")) {
            atEof = false;

            if (currentCharacter == ' ' || currentCharacter == '\r' || currentCharacter == '\n') {
                break;
            }

            if (currentCharacter == ',' || currentCharacter == '.') {
                if (StringBuilder_length(currentWordBuilder) > 0) {
                    StringBuilder_appendChar(currentWordBuilder, currentCharacter);
                }
                break;
            }

            StringBuilder_appendChar(currentWordBuilder, currentCharacter);
        }

        if (atEof) {
            StringBuilder_destroy(currentWordBuilder);
            break;
        }

        if (StringBuilder_length(currentWordBuilder) == 0) {
            StringBuilder_destroy(currentWordBuilder);
            continue;
        }

        char * const currentWord = StringBuilder_toStringAndDestroy(currentWordBuilder);
        safeFprintf(pipe1WriteFile, "hw11Program1", "%s\n", currentWord);
        free(currentWord);
    }
    safeFprintf(pipe1WriteFile, "hw11Program1", "\n");
    safeSemPost(semaphore1, "hw11Program1");

    fclose(inputFile);
    fclose(pipe1WriteFile);
}
