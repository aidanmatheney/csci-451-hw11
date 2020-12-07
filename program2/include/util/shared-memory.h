#pragma once

#include <sys/types.h>
#include <sys/stat.h>

int tryCreateSharedMemory(key_t key, size_t size, mode_t permissions);
int safeOpenSharedMemory(key_t key, size_t size, char const *callerDescription);
void safeDestroySharedMemory(int id, char const *callerDescription);

void *safeAttachSharedMemory(int id, char const *callerDescription);
void safeDetatchSharedMemory(void const *memory, char const *callerDescription);
