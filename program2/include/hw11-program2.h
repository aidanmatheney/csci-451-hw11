#pragma once

#include <sys/types.h>

void hw11Program2(
    int pipe1ReadFileDescriptor,
    int pipe2WriteFileDescriptor,
    char const *semaphore1Name,
    char const *semaphore2Name,
    key_t sharedMemoryKey
);
