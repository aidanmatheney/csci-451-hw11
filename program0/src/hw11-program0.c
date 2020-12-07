#include "../include/hw11-program0.h"

#include "../include/util/string.h"
#include "../include/util/random.h"
#include "../include/util/semaphore.h"
#include "../include/util/shared-memory.h"
#include "../include/util/pipe.h"
#include "../include/util/process.h"
#include "../include/util/guard.h"

#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <semaphore.h>

struct Hw11WordCounts {
    unsigned int type1;
    unsigned int type2;
};

static void createHw11Semaphore(char * *outSemaphoreName, sem_t * *outSemaphore);
static void createHw11SharedMemory(key_t *outKey, int *outId);

/**
 * Run CSCI 451 HW11 program 0.
 */
void hw11Program0(
    char const * const inputFilePath,
    char const * const outputFilePath,
    char const * const program1Path,
    char const * const program2Path,
    char const * const program3Path
) {
    guardNotNull(inputFilePath, "inputFilePath", "hw11Program0");
    guardNotNull(outputFilePath, "outputFilePath", "hw11Program0");
    guardNotNull(program1Path, "program1Path", "hw11Program0");
    guardNotNull(program2Path, "program2Path", "hw11Program0");
    guardNotNull(program3Path, "program3Path", "hw11Program0");

    int pipe1ReadFileDescriptor;
    int pipe1WriteFileDescriptor;
    safeCreatePipe(&pipe1ReadFileDescriptor, &pipe1WriteFileDescriptor, "hw11Program0");

    int pipe2ReadFileDescriptor;
    int pipe2WriteFileDescriptor;
    safeCreatePipe(&pipe2ReadFileDescriptor, &pipe2WriteFileDescriptor, "hw11Program0");

    char *semaphore1Name;
    sem_t *semaphore1;
    createHw11Semaphore(&semaphore1Name, &semaphore1);

    char *semaphore2Name;
    sem_t *semaphore2;
    createHw11Semaphore(&semaphore2Name, &semaphore2);

    key_t sharedMemoryKey;
    int sharedMemoryId;
    createHw11SharedMemory(&sharedMemoryKey, &sharedMemoryId);

    char * const inputFilePathForArg = formatString("%s", inputFilePath);
    char * const outputFilePathForArg = formatString("%s", outputFilePath);
    char * const program1PathForArg = formatString("%s", program1Path);
    char * const program2PathForArg = formatString("%s", program2Path);
    char * const program3PathForArg = formatString("%s", program3Path);

    char * const pipe1ReadFileDescriptorString = formatString("%d", pipe1ReadFileDescriptor);
    char * const pipe1WriteFileDescriptorString = formatString("%d", pipe1WriteFileDescriptor);
    char * const pipe2ReadFileDescriptorString = formatString("%d", pipe2ReadFileDescriptor);
    char * const pipe2WriteFileDescriptorString = formatString("%d", pipe2WriteFileDescriptor);
    char * const sharedMemoryKeyString = formatString("%d", sharedMemoryKey);

    // Lock semaphore1 so program1 can write to pipe1 before program2 begins reading
    safeSemWait(semaphore1, "hw11Program0");
    // Lock semaphore2 so program2 can write to sharedMemory before program3 begins reading
    safeSemWait(semaphore2, "hw11Program0");

    pid_t const program1ProcessId = safeFork("hw11Program0");
    if (program1ProcessId == 0) {
        // Program 1

        close(pipe1ReadFileDescriptor);
        close(pipe2ReadFileDescriptor);
        close(pipe2WriteFileDescriptor);

        safeExecvp(
            program1Path,
            (char * const []){
                program1PathForArg,
                inputFilePathForArg,
                pipe1WriteFileDescriptorString,
                semaphore1Name,
                NULL
            },
            "hw11Program0"
        );
    }

    pid_t const program2ProcessId = safeFork("hw11Program0");
    if (program2ProcessId == 0) {
        // Program 2

        close(pipe1WriteFileDescriptor);
        close(pipe2ReadFileDescriptor);

        safeExecvp(
            program2Path,
            (char * const []){
                program2PathForArg,
                pipe1ReadFileDescriptorString,
                pipe2WriteFileDescriptorString,
                semaphore1Name,
                semaphore2Name,
                sharedMemoryKeyString,
                NULL
            },
            "hw11Program0"
        );
    }

    pid_t const program3ProcessId = safeFork("hw11Program0");
    if (program3ProcessId == 0) {
        // Program 3

        close(pipe1ReadFileDescriptor);
        close(pipe1WriteFileDescriptor);
        close(pipe2WriteFileDescriptor);

        safeExecvp(
            program3Path,
            (char * const []){
                program3PathForArg,
                outputFilePathForArg,
                pipe2ReadFileDescriptorString,
                semaphore2Name,
                sharedMemoryKeyString,
                NULL
            },
            "hw11Program0"
        );
    }

    safeWaitpid(program1ProcessId, NULL, 0, "hw11Program0");
    safeWaitpid(program2ProcessId, NULL, 0, "hw11Program0");
    safeWaitpid(program3ProcessId, NULL, 0, "hw11Program0");

    close(pipe1ReadFileDescriptor);
    close(pipe1WriteFileDescriptor);
    close(pipe2ReadFileDescriptor);
    close(pipe2WriteFileDescriptor);

    safeSemClose(semaphore1, "hw11Program0");
    free(semaphore1Name);
    safeSemClose(semaphore2, "hw11Program0");
    free(semaphore2Name);

    safeDestroySharedMemory(sharedMemoryId, "hw11Program0");

    free(inputFilePathForArg);
    free(outputFilePathForArg);
    free(program1PathForArg);
    free(program2PathForArg);
    free(program3PathForArg);

    free(pipe1ReadFileDescriptorString);
    free(pipe1WriteFileDescriptorString);
    free(pipe2ReadFileDescriptorString);
    free(pipe2WriteFileDescriptorString);
    free(sharedMemoryKeyString);
}

static void createHw11Semaphore(char * * const outSemaphoreName, sem_t * * const outSemaphore) {
    char *semaphoreName;
    sem_t *semaphore;
    while (true) {
        semaphoreName = formatString(
            "/csci-451-hw11-aidanmatheney-73a9fdb179f8496ebe3635e86e035743-semaphore-%d%d%d%d",
            randomInt(0, INT_MAX),
            randomInt(0, INT_MAX),
            randomInt(0, INT_MAX),
            randomInt(0, INT_MAX)
        );
        semaphore = tryCreateSemaphore(semaphoreName, S_IRUSR | S_IWUSR, 1);
        if (semaphore != NULL) {
            break;
        }

        free(semaphoreName);
    }

    *outSemaphoreName = semaphoreName;
    *outSemaphore = semaphore;
}

static void createHw11SharedMemory(key_t * const outKey, int * const outId) {
    key_t key;
    int id;
    while (true) {
        key = randomInt(0, INT_MAX);
        id = tryCreateSharedMemory(key, sizeof (struct Hw11WordCounts), S_IRUSR | S_IWUSR);
        if (id != -1) {
            break;
        }
    }

    *outKey = key;
    *outId = id;
}
