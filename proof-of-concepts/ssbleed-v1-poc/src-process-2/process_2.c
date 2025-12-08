/*
 * Copyright (c) 2025 Chang Liu
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>

#define secret_len 43

char* secret = "SSBleed-V1 Proof-of-concept on Neoverse-N2\0";
int message[8 * secret_len];

extern void secret_dependent_branch(int secret_bit);

/**
 * Convert from bytes to bits
 */
void encode(int* message) {
    for(int i = 0; i < secret_len; i ++) {
        for(int j = 0; j < 8; ++ j) {
            message[i * 8 + j] = (secret[i] >> j) & 1;
        }
    }
}

int main(int argc, char* argv[]){
    encode(message);
    sem_t *mutex1 = sem_open("/name_sem1", 0);
    sem_t *mutex2 = sem_open("/name_sem2", 0);
    while(mutex1 == NULL || mutex2 == NULL) {
        mutex1 = sem_open("/name_sem1", 0);
        mutex2 = sem_open("/name_sem2", 0);
    }
    for(int s = 0; s < 8 * secret_len; ++ s) {
        sem_wait(mutex2);  // Synchronize
        secret_dependent_branch(message[s]);  // A secret-dependent demo
        sem_post(mutex1);  // Synchronize
    }
    sem_close(mutex1);
    sem_close(mutex2);
    return 0;
}