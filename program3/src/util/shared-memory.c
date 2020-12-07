#include "../../include/util/shared-memory.h"

#include "../../include/util/guard.h"
#include "../../include/util/error.h"

#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

/**
 * Attempt to create shared memory with the given key and size. If the operation fails, return -1.
 *
 * @param key The key.
 * @param size The size.
 * @param permissions The permissions to be placed on the new shared memory.
 *
 * @returns The ID of the newly created shared memory, or -1 if the operation failed.
 */
int tryCreateSharedMemory(key_t const key, size_t const size, mode_t const permissions) {
    return shmget(key, size, IPC_CREAT | IPC_EXCL | (int)permissions);
}

/**
 * Open the shared memory specified by the given key. If the operation fails, abort the program with an error message.
 *
 * @param key The key.
 * @param size The size.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The ID of the shared memory.
 */
int safeOpenSharedMemory(key_t const key, size_t const size, char const * const callerDescription) {
    guardNotNull(callerDescription, "callerDescription", "safeOpenSharedMemory");

    int const shmgetResult = shmget(key, size, 0);
    if (shmgetResult == -1) {
        int const shmgetErrorCode = errno;
        char const * const shmgetErrorMessage = strerror(shmgetErrorCode);

        abortWithErrorFmt(
            "%s: Failed to open shared memory using shmget (error code: %d; error message: \"%s\")",
            callerDescription,
            shmgetErrorCode,
            shmgetErrorMessage
        );
        return -1;
    }

    return shmgetResult;
}

/**
 * Destroy the shared memory specified by the given ID. If the operation fails, abort the program with an error message.
 *
 * @param id The ID.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 */
void safeDestroySharedMemory(int const id, char const * const callerDescription) {
    guardNotNull(callerDescription, "callerDescription", "safeDestroySharedMemory");

    int const shmctlResult = shmctl(id, IPC_RMID, NULL);
    if (shmctlResult == -1) {
        int const shmctlErrorCode = errno;
        char const * const shmctlErrorMessage = strerror(shmctlErrorCode);

        abortWithErrorFmt(
            "%s: Failed to destroy shared memory using shmctl (error code: %d; error message: \"%s\")",
            callerDescription,
            shmctlErrorCode,
            shmctlErrorMessage
        );
    }
}

/**
 * Attach the shared memory specified by the given ID. If the operation fails, abort the program with an error message.
 *
 * @param id The ID.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The address of the shared memory.
 */
void *safeAttachSharedMemory(int const id, char const * const callerDescription) {
    guardNotNull(callerDescription, "callerDescription", "safeAttachSharedMemory");

    void * const memory = shmat(id, NULL, 0);
    if (memory == (void *)-1) {
        int const shmatErrorCode = errno;
        char const * const shmatErrorMessage = strerror(shmatErrorCode);

        abortWithErrorFmt(
            "%s: Failed to attach shared memory using shmat (error code: %d; error message: \"%s\")",
            callerDescription,
            shmatErrorCode,
            shmatErrorMessage
        );
        return NULL;
    }

    return memory;
}

/**
 * Detach the shared memory specified by the given address. If the operation fails, abort the program with an error
 * message.
 *
 * @param memory The address.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 */
void safeDetatchSharedMemory(void const * const memory, char const * const callerDescription) {
    guardNotNull(callerDescription, "callerDescription", "safeDetatchSharedMemory");

    int const shmdtResult = shmdt(memory);
    if (shmdtResult == -1) {
        int const shmdtErrorCode = errno;
        char const * const shmdtErrorMessage = strerror(shmdtErrorCode);

        abortWithErrorFmt(
            "%s: Failed to detach shared memory using shmdt (error code: %d; error message: \"%s\")",
            callerDescription,
            shmdtErrorCode,
            shmdtErrorMessage
        );
    }
}
