#pragma once

#include <sys/types.h>

void hw11Program3(
    char const *outputFilePath,
    int pipe2ReadFileDescriptor,
    char const *semaphore2Name,
    key_t sharedMemoryKey
);
