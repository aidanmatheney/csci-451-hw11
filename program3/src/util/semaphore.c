#include "../../include/util/semaphore.h"

#include "../../include/util/guard.h"
#include "../../include/util/error.h"

#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>

/**
 * Open an existing semaphore with the given name. If the operation fails, abort the program with an error message.
 *
 * @param name The name of the existing semaphore.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The address of the existing semaphore.
 */
sem_t *safeSemOpen(char const * const name, char const * const callerDescription) {
    guardNotNull(name, "name", "safeSemOpen");
    guardNotNull(callerDescription, "callerDescription", "safeSemOpen");

    sem_t * const semaphore = sem_open(name, 0);
    if (semaphore == SEM_FAILED) {
        int const semOpenErrorCode = errno;
        char const * const semOpenErrorMessage = strerror(semOpenErrorCode);

        abortWithErrorFmt(
            "%s: Failed to open semaphore named %s using sem_open (error code: %d; error message: \"%s\")",
            callerDescription,
            name,
            semOpenErrorCode,
            semOpenErrorMessage
        );
        return NULL;
    }

    return semaphore;
}

/**
 * Create a new semaphore or open an existing semaphore with the given name. If the operation fails, abort the program
 * with an error message.
 *
 * @param name The name of the new or existing semaphore.
 * @param permissions The permissions to be placed on the new semaphore.
 * @param initialValue The initial value for the new semaphore.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The address of the new or existing semaphore.
 */
sem_t *safeSemOpenOrCreate(
    char const * const name,
    mode_t const permissions,
    unsigned int const initialValue,
    char const * const callerDescription
) {
    guardNotNull(name, "name", "safeSemOpenOrCreate");
    guardNotNull(callerDescription, "callerDescription", "safeSemOpenOrCreate");

    sem_t * const semaphore = sem_open(name, O_CREAT, permissions, initialValue);
    if (semaphore == SEM_FAILED) {
        int const semOpenErrorCode = errno;
        char const * const semOpenErrorMessage = strerror(semOpenErrorCode);

        abortWithErrorFmt(
            "%s: Failed to open or create semaphore named %s using sem_open (error code: %d; error message: \"%s\")",
            callerDescription,
            name,
            semOpenErrorCode,
            semOpenErrorMessage
        );
        return NULL;
    }

    return semaphore;
}

/**
 * Create a new semaphore with the given name. If the operation fails, abort the program with an error message.
 *
 * @param name The name of the new semaphore.
 * @param permissions The permissions to be placed on the new semaphore.
 * @param initialValue The initial value for the new semaphore.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The address of the new semaphore.
 */
sem_t *safeSemCreate(
    char const * const name,
    mode_t const permissions,
    unsigned int const initialValue,
    char const * const callerDescription
) {
    guardNotNull(name, "name", "safeSemCreate");
    guardNotNull(callerDescription, "callerDescription", "safeSemCreate");

    sem_t * const semaphore = sem_open(name, O_CREAT | O_EXCL, permissions, initialValue);
    if (semaphore == SEM_FAILED) {
        int const semOpenErrorCode = errno;
        char const * const semOpenErrorMessage = strerror(semOpenErrorCode);

        abortWithErrorFmt(
            "%s: Failed to create semaphore named %s using sem_open (error code: %d; error message: \"%s\")",
            callerDescription,
            name,
            semOpenErrorCode,
            semOpenErrorMessage
        );
        return NULL;
    }

    return semaphore;
}

/**
 * Create a new semaphore with the given name. If a semaphore with this name already exists, return null. If the
 * operation otherwise fails, abort the program with an error message.
 *
 * @param name The name of the new semaphore.
 * @param permissions The permissions to be placed on the new semaphore.
 * @param initialValue The initial value for the new semaphore.
 *
 * @returns The address of the new semaphore, or null if a semaphore with the given name already exists.
 */
sem_t *tryCreateSemaphore(char const * const name, mode_t const permissions, unsigned int const initialValue) {
    guardNotNull(name, "name", "tryCreateSemaphore");

    sem_t * const semaphore = sem_open(name, O_CREAT | O_EXCL, permissions, initialValue);
    if (semaphore == SEM_FAILED) {
        int const semOpenErrorCode = errno;
        if (semOpenErrorCode == EEXIST) {
            return NULL;
        }

        char const * const semOpenErrorMessage = strerror(semOpenErrorCode);

        abortWithErrorFmt(
            "tryCreateSemaphore: Failed to create semaphore named %s using sem_open (error code: %d; error message: \"%s\")",
            name,
            semOpenErrorCode,
            semOpenErrorMessage
        );
        return NULL;
    }

    return semaphore;
}

/**
 * Close the given named semaphore. If the operation fails, abort the program with an error message.
 *
 * @param semaphore The semaphore.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 */
void safeSemClose(sem_t * const semaphore, char const * const callerDescription) {
    guardNotNull(semaphore, "semaphore", "tryCreateSemaphore");
    guardNotNull(callerDescription, "callerDescription", "tryCreateSemaphore");

    int const semCloseResult = sem_close(semaphore);
    if (semCloseResult != 0) {
        int const semCloseErrorCode = errno;
        char const * const semCloseErrorMessage = strerror(semCloseErrorCode);

        abortWithErrorFmt(
            "%s: Failed to close semaphore using sem_close (error code: %d; error message: \"%s\")",
            callerDescription,
            semCloseErrorCode,
            semCloseErrorMessage
        );
    }
}

/**
 * Decrement (lock) the given semaphore. If the semaphore's value is greater than zero, then the decrement proceeds, and
 * the function returns immediately. If the semaphore currently has the value zero, then the call blocks until either it
 * becomes possible to perform the decrement (i.e., the semaphore value rises above zero), or a signal handler
 * interrupts the call. If the operation fails, abort the program with an error message.
 *
 * @param semaphore The semaphore.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 */
void safeSemWait(sem_t * const semaphore, char const * const callerDescription) {
    guardNotNull(semaphore, "semaphore", "safeSemWait");
    guardNotNull(callerDescription, "callerDescription", "safeSemWait");

    int const semWaitResult = sem_wait(semaphore);
    if (semWaitResult != 0) {
        int const semWaitErrorCode = errno;
        char const * const semWaitErrorMessage = strerror(semWaitErrorCode);

        abortWithErrorFmt(
            "%s: Failed to lock semaphore using sem_wait (error code: %d; error message: \"%s\")",
            callerDescription,
            semWaitErrorCode,
            semWaitErrorMessage
        );
    }
}

/**
 * Increment (unlock) the given semaphore. If the semaphore's value consequently becomes greater than zero, then another
 * process or thread blocked in a sem_wait call will be woken up and proceed to lock the semaphore. If the operation
 * fails, abort the program with an error message.
 *
 * @param semaphore The semaphore.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 */
void safeSemPost(sem_t * const semaphore, char const * const callerDescription) {
    guardNotNull(semaphore, "semaphore", "safeSemPost");
    guardNotNull(callerDescription, "callerDescription", "safeSemPost");

    int const semPostResult = sem_post(semaphore);
    if (semPostResult != 0) {
        int const semPostErrorCode = errno;
        char const * const semPostErrorMessage = strerror(semPostErrorCode);

        abortWithErrorFmt(
            "%s: Failed to unlock semaphore using sem_post (error code: %d; error message: \"%s\")",
            callerDescription,
            semPostErrorCode,
            semPostErrorMessage
        );
    }
}
