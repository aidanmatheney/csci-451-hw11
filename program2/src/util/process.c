#include "../../include/util/process.h"

#include "../../include/util/guard.h"
#include "../../include/util/error.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

/**
 * Clone the calling process, creating an exact copy. If the operation fails, abort the program with an error message.
 *
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The child process ID to the parent process and 0 to the child process.
 */
pid_t safeFork(char const * const callerDescription) {
    guardNotNull(callerDescription, "callerDescription", "safeFork");

    pid_t const forkResult = fork();
    if (forkResult == -1) {
        int const forkErrorCode = errno;
        char const * const forkErrorMessage = strerror(forkErrorCode);

        abortWithErrorFmt(
            "%s: Failed to fork process using fork (error code: %d; error message: \"%s\")",
            callerDescription,
            forkErrorCode,
            forkErrorMessage
        );
        return -1;
    }

    return forkResult;
}

/**
 * Replace the current process image with a new process image.
 *
 * @param filePath The path to the program file to execute. If this does not include a slash character, PATH will be
 *                 searched.
 * @param argv A null-terminated array of null-terminated strings that represents the argument list available to the new
 *             program. The first argument, by convention, should point to the filename associated with the file being
 *             executed.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 */
void safeExecvp(
    char const * const filePath,
    char * const * const argv,
    char const * const callerDescription
) {
    guardNotNull(filePath, "filePath", "safeExecvp");
    guardNotNull(argv, "argv", "safeExecvp");
    guardNotNull(callerDescription, "callerDescription", "safeExecvp");

    int const execResult = execvp(filePath, argv);
    if (execResult == -1) {
        int const execErrorCode = errno;
        char const * const execErrorMessage = strerror(execErrorCode);

        abortWithErrorFmt(
            "%s: Failed to replace process with \"%s\" using execvp (error code: %d; error message: \"%s\")",
            callerDescription,
            filePath,
            execErrorCode,
            execErrorMessage
        );
    }
}

/**
 * Wait for the process specified by the given ID.
 *
 * @param processId The process ID.
 * @param outStatusPtr If not null and this function returns because the status of a child process is available, that
 *                     status will be stored here.
 * @param options waitpid options.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The process ID.
 */
pid_t safeWaitpid(
    pid_t const processId,
    int * const outStatusPtr,
    int const options,
    char const * const callerDescription
) {
    guardNotNull(callerDescription, "callerDescription", "safeWaitPid");

    pid_t const waitpidResult = waitpid(processId, outStatusPtr, options);
    if (waitpidResult == -1) {
        int const waitpidErrorCode = errno;
        char const * const waitpidErrorMessage = strerror(waitpidErrorCode);

        abortWithErrorFmt(
            "%s: Failed to wait for process using waitpid (error code: %d; error message: \"%s\")",
            callerDescription,
            waitpidErrorCode,
            waitpidErrorMessage
        );
        return -1;
    }

    return waitpidResult;
}
