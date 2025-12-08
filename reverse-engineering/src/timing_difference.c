/*
 * Copyright (c) 2025 Chang Liu
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include "utils.h"

#define REPEAT 10000

void stld(void* st_addr, void* ld_addr) {
    asm volatile(
        "mov x2, #1 \n\t"
        ".rep 50 \n\t"
        "mul %[st_addr], %[st_addr], x2 \n\t"
        ".endr \n\t"
        "str x2, [%[st_addr]] \n\t"
        "ldr x3, [%[ld_addr]] \n\t"
        ".rep 50 \n\t"
        "mul x3, x3, x2 \n\t"
        ".endr \n\t"
        :
        :[st_addr] "r" (st_addr), [ld_addr] "r" (ld_addr)
    );
}


int main() {
    size_t A[30];
    uint64_t time1, time2, samples[30];

    // Hot CPU, optional
    for(int i = 0; i < 100; ++ i) {
        stld(&A[0], &A[10]);
    }

    for(int i = 0; i < REPEAT; ++ i) {
        // Initialize the MDP state with 3 DA
        // MDP counter value: 0 -> 15
        for(int j = 0; j < 3; ++ j)
            stld(&A[0], &A[0]);
        // Probe the MDP state with 30 DA
        // MDP counter value: 15 -> 0
        // Expected prediction sequence: 15 BLK, 15 SSB
        for(int j = 0; j < 30; ++ j) {
            time1 = readCLK();
            stld(&A[0], &A[10]);
            time2 = readCLK() - time1;  
            samples[j] = time2;
        }
        for(int j = 0; j < 30; ++ j) {
            printf("%ld ", samples[j]);
        }
        printf("\n");
    }

    return 0;
}