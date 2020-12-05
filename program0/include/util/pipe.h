#pragma once

void safeCreatePipe(
    int *outReadFileDescriptor,
    int *outWriteFileDescriptor,
    char const *callerDescription
);
