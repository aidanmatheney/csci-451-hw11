#include "../include/hw11-program3.h"

#include "../include/util/file.h"
#include "../include/util/semaphore.h"
#include "../include/util/shared-memory.h"
#include "../include/util/guard.h"

#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <semaphore.h>

struct Hw11WordCounts {
    unsigned int type1;
    unsigned int type2;
};

/**
 * Run CSCI 451 HW11 program 3.
 */
void hw11Program3(
    char const * const outputFilePath,
    int const pipe2ReadFileDescriptor,
    char const * const semaphore2Name,
    key_t const sharedMemoryKey
) {
    guardNotNull(outputFilePath, "outputFilePath", "hw11Program3");
    guardNotNull(semaphore2Name, "semaphore2Name", "hw11Program3");

    FILE * const outputFile = safeFopen(outputFilePath, "w", "hw11Program3");
    FILE * const pipe2ReadFile = safeFdopen(pipe2ReadFileDescriptor, "r", "hw11Program3");
    sem_t * const semaphore2 = safeSemOpen(semaphore2Name, "hw11Program3");
    int const sharedMemoryId = safeOpenSharedMemory(sharedMemoryKey, sizeof (struct Hw11WordCounts), "hw11Program3");
    struct Hw11WordCounts * const wordCounts = safeAttachSharedMemory(sharedMemoryId, "hw11Program3");

    safeSemWait(semaphore2, "hw11Program3");
    bool wroteFirstWord = false;
    while (true) {
        char * const currentPigLatinWord = readFileLine(pipe2ReadFile);
        if (currentPigLatinWord == NULL) {
            break;
        }
        if (currentPigLatinWord[0] == '\0') {
            free(currentPigLatinWord);
            break;
        }

        safeFprintf(
            outputFile,
            "hw11Program3",
            wroteFirstWord ? " %s" : "%s",
            currentPigLatinWord
        );
        wroteFirstWord = true;

        free(currentPigLatinWord);
    }
    printf("Type 1: %u\nType 2: %u\n", wordCounts->type1, wordCounts->type2);
    safeSemPost(semaphore2, "hw11Program3");

    safeFprintf(outputFile, "hw11Program3", "\n");

    fclose(pipe2ReadFile);
    fclose(outputFile);
}
