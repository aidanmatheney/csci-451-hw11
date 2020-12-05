#include "../../include/util/pipe.h"

#include "../../include/util/guard.h"
#include "../../include/util/error.h"

#include <unistd.h>
#include <string.h>
#include <errno.h>

/**
 * Create a pipe. If the operation fails, abort the program with an error message.
 *
 * @param outReadFileDescriptor The address of where to store the file descriptor of the read end of the new pipe.
 * @param outWriteFileDescriptor The address of where to store the file descriptor of the write end of the new pipe.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 */
void safeCreatePipe(
    int * const outReadFileDescriptor,
    int * const outWriteFileDescriptor,
    char const * const callerDescription
) {
    guardNotNull(outReadFileDescriptor, "outReadFileDescriptor", "safeCreatePipe");
    guardNotNull(outWriteFileDescriptor, "outWriteFileDescriptor", "safeCreatePipe");
    guardNotNull(callerDescription, "callerDescription", "safeCreatePipe");

    int fileDescriptors[2];
    int const pipeResult = pipe(fileDescriptors);
    if (pipeResult != 0) {
        int const pipeErrorCode = errno;
        char const * const pipeErrorMessage = strerror(pipeErrorCode);

        abortWithErrorFmt(
            "%s: Failed to create pipe using pipe (error code: %d; error message: \"%s\")",
            callerDescription,
            pipeErrorCode,
            pipeErrorMessage
        );
        return;
    }

    *outReadFileDescriptor = fileDescriptors[0];
    *outWriteFileDescriptor = fileDescriptors[1];
}
