#include "../include/hw11-program2.h"

#include "../include/util/StringBuilder.h"

#include "../include/util/file.h"
#include "../include/util/semaphore.h"
#include "../include/util/shared-memory.h"
#include "../include/util/guard.h"

#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <semaphore.h>

struct Hw11WordCounts {
    unsigned int type1;
    unsigned int type2;
};

/**
 * Run CSCI 451 HW11 program 2.
 */
void hw11Program2(
    int const pipe1ReadFileDescriptor,
    int const pipe2WriteFileDescriptor,
    char const * const semaphore1Name,
    char const * const semaphore2Name,
    key_t const sharedMemoryKey
) {
    guardNotNull(semaphore1Name, "semaphore1Name", "hw11Program2");
    guardNotNull(semaphore2Name, "semaphore2Name", "hw11Program2");

    FILE * const pipe1ReadFile = safeFdopen(pipe1ReadFileDescriptor, "r", "hw11Program2");
    FILE * const pipe2WriteFile = safeFdopen(pipe2WriteFileDescriptor, "w", "hw11Program2");
    sem_t * const semaphore1 = safeSemOpen(semaphore1Name, "hw11Program2");
    sem_t * const semaphore2 = safeSemOpen(semaphore2Name, "hw11Program2");
    int const sharedMemoryId = safeOpenSharedMemory(sharedMemoryKey, sizeof (struct Hw11WordCounts), "hw11Program2");
    struct Hw11WordCounts * const wordCounts = safeAttachSharedMemory(sharedMemoryId, "hw11Program2");

    unsigned int type1WordCount = 0;
    unsigned int type2WordCount = 0;

    safeSemWait(semaphore1, "hw11Program2");
    while (true) {
        char * const currentWord = readFileLine(pipe1ReadFile);
        if (currentWord == NULL) {
            break;
        }
        if (currentWord[0] == '\0') {
            free(currentWord);
            break;
        }

        StringBuilder const pigLatinWordBuilder = StringBuilder_fromString(currentWord);
        size_t const currentWordLength = StringBuilder_length(pigLatinWordBuilder);
        char const firstCharacter = currentWord[0];
        char const firstCharacterAsLower = (char)tolower(firstCharacter);
        char const lastCharacter = currentWord[currentWordLength - 1];
        if (
            firstCharacterAsLower == 'a'
            || firstCharacterAsLower == 'e'
            || firstCharacterAsLower == 'i'
            || firstCharacterAsLower == 'o'
            || firstCharacterAsLower == 'u'
            || firstCharacterAsLower == 'y'
        ) {
            type1WordCount += 1;

            size_t rayInsertPosition = (lastCharacter == ',' || lastCharacter == '.')
                ? currentWordLength - 1
                : currentWordLength;

            StringBuilder_insert(pigLatinWordBuilder, rayInsertPosition, "ray");
        } else {
            type2WordCount += 1;

            StringBuilder_removeAt(pigLatinWordBuilder, 0);

            size_t xayInsertPosition = (lastCharacter == ',' || lastCharacter == '.')
                ? currentWordLength - 2
                : currentWordLength - 1;

            StringBuilder_insertFmt(pigLatinWordBuilder, xayInsertPosition, "%cay", firstCharacter);
        }

        char * const pigLatinWord = StringBuilder_toStringAndDestroy(pigLatinWordBuilder);
        safeFprintf(pipe2WriteFile, "hw11Program2", "%s\n", pigLatinWord);
        free(pigLatinWord);

        free(currentWord);
    }
    safeSemPost(semaphore1, "hw11Program2");

    safeFprintf(pipe2WriteFile, "hw11Program2", "\n");
    *wordCounts = (struct Hw11WordCounts){
        .type1 = type1WordCount,
        .type2 = type2WordCount
    };
    safeSemPost(semaphore2, "hw11Program2");

    fclose(pipe1ReadFile);
    fclose(pipe2WriteFile);

    safeDetatchSharedMemory(wordCounts, "hw11Program2");
}
