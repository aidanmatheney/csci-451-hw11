#include "../include/hw11-program2.h"

#include "../include/util/StringBuilder.h"

#include "../include/util/file.h"
#include "../include/util/semaphore.h"
#include "../include/util/guard.h"

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <semaphore.h>

/**
 * Run CSCI 451 HW11 program 2.
 */
void hw11Program2(
    char const * const type1WordCountOutputFilePath,
    char const * const type2WordCountOutputFilePath,
    char const * const semaphoreName,
    int const pipe1ReadFileDescriptor,
    int const pipe2WriteFileDescriptor
) {
    guardNotNull(type1WordCountOutputFilePath, "type1WordCountOutputFilePath", "hw11Program2");
    guardNotNull(type2WordCountOutputFilePath, "type2WordCountOutputFilePath", "hw11Program2");
    guardNotNull(semaphoreName, "semaphoreName", "hw11Program2");

    sem_t * const semaphore = safeSemOpen(semaphoreName, "hw11Program1");
    FILE * const pipe1ReadFile = safeFdopen(pipe1ReadFileDescriptor, "r", "hw11Program1");
    FILE * const pipe2WriteFile = safeFdopen(pipe2WriteFileDescriptor, "w", "hw11Program1");

    unsigned int type1WordCount = 0;
    unsigned int type2WordCount = 0;

    safeSemWait(semaphore, "hw11Program2");
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
    safeSemPost(semaphore, "hw11Program2");

    safeFprintf(pipe2WriteFile, "hw11Program2", "\n");

    fclose(pipe1ReadFile);
    fclose(pipe2WriteFile);

    FILE * const type1WordCountOutputFile = safeFopen(type1WordCountOutputFilePath, "w", "hw11Program2");
    FILE * const type2WordCountOutputFile = safeFopen(type2WordCountOutputFilePath, "w", "hw11Program2");

    safeFprintf(type1WordCountOutputFile, "hw11Program2", "%u\n", type1WordCount);
    safeFprintf(type2WordCountOutputFile, "hw11Program2", "%u\n", type2WordCount);

    fclose(type1WordCountOutputFile);
    fclose(type2WordCountOutputFile);
}
