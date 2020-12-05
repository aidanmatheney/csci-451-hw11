#pragma once

#include <semaphore.h>

sem_t *safeSemOpen(char const *name, char const *callerDescription);
sem_t *safeSemOpenOrCreate(
    char const *name,
    mode_t permissions,
    unsigned int initialValue,
    char const *callerDescription
);
sem_t *safeSemCreate(
    char const *name,
    mode_t permissions,
    unsigned int initialValue,
    char const *callerDescription
);
sem_t *tryCreateSemaphore(char const *name, mode_t permissions, unsigned int initialValue);

void safeSemClose(sem_t *semaphore, char const *callerDescription);

void safeSemWait(sem_t *semaphore, char const *callerDescription);
void safeSemPost(sem_t *semaphore, char const *callerDescription);
