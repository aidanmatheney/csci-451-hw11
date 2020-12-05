#include "../include/hw11-program0.h"

#include "../include/util/string.h"
#include "../include/util/random.h"
#include "../include/util/semaphore.h"
#include "../include/util/pipe.h"
#include "../include/util/process.h"
#include "../include/util/guard.h"

#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <semaphore.h>

static void createHw11Semaphore(char * *outSemaphoreName, sem_t * *outSemaphore);

/**
 * Run CSCI 451 HW11 program 0.
 */
void hw11Program0(
    char const * const program1Path,
    char const * const program2Path,
    char const * const program3Path
) {
    guardNotNull(program1Path, "program1Path", "hw11Program0");
    guardNotNull(program2Path, "program2Path", "hw11Program0");
    guardNotNull(program3Path, "program3Path", "hw11Program0");

    char *semaphoreName;
    sem_t *semaphore;
    createHw11Semaphore(&semaphoreName, &semaphore);

    int pipe1ReadFileDescriptor;
    int pipe1WriteFileDescriptor;
    safeCreatePipe(&pipe1ReadFileDescriptor, &pipe1WriteFileDescriptor, "hw11Program0");

    int pipe2ReadFileDescriptor;
    int pipe2WriteFileDescriptor;
    safeCreatePipe(&pipe2ReadFileDescriptor, &pipe2WriteFileDescriptor, "hw11Program0");

    char * const program1PathForArg = formatString("%s", program1Path);
    char * const program2PathForArg = formatString("%s", program2Path);
    char * const program3PathForArg = formatString("%s", program3Path);

    char * const pipe1ReadFileDescriptorString = formatString("%d", pipe1ReadFileDescriptor);
    char * const pipe1WriteFileDescriptorString = formatString("%d", pipe1WriteFileDescriptor);
    char * const pipe2ReadFileDescriptorString = formatString("%d", pipe2ReadFileDescriptor);
    char * const pipe2WriteFileDescriptorString = formatString("%d", pipe2WriteFileDescriptor);

    // Close the semaphore so program1 can write to pipe1 before program2 begins reading
    safeSemWait(semaphore, "hw11Program0");

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
                semaphoreName,
                pipe1WriteFileDescriptorString,
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
                semaphoreName,
                pipe1ReadFileDescriptorString,
                pipe2WriteFileDescriptorString,
                NULL
            },
            "hw11Program0"
        );
    }

    safeWaitpid(program1ProcessId, NULL, 0, "hw11Program0");
    safeWaitpid(program2ProcessId, NULL, 0, "hw11Program0");

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
                pipe2ReadFileDescriptorString,
                NULL
            },
            "hw11Program0"
        );
    }

    safeWaitpid(program3ProcessId, NULL, 0, "hw11Program0");

    safeSemClose(semaphore, "hw11Program0");
    free(semaphoreName);

    close(pipe1ReadFileDescriptor);
    close(pipe1WriteFileDescriptor);
    close(pipe2ReadFileDescriptor);
    close(pipe2WriteFileDescriptor);

    free(program1PathForArg);
    free(program2PathForArg);
    free(program3PathForArg);

    free(pipe1ReadFileDescriptorString);
    free(pipe1WriteFileDescriptorString);
    free(pipe2ReadFileDescriptorString);
    free(pipe2WriteFileDescriptorString);
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
