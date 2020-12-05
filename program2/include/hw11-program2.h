#pragma once

void hw11Program2(
    char const *type1WordCountOutputFilePath,
    char const *type2WordCountOutputFilePath,
    char const *semaphoreName,
    int pipe1ReadFileDescriptor,
    int pipe2WriteFileDescriptor
);
