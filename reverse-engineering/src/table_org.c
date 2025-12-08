/*
 * Copyright (c) 2025 Chang Liu
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include "utils.h"

#ifndef SAMPLED_LD_NUM
#define SAMPLED_LD_NUM 1
#endif

#define REPEAT 100

extern void stld_lab(void* addr_store, void* addr_load_alias, void* addr_samples, void* addr_load_nonalias);

size_t samples[SAMPLED_LD_NUM * 20];
int upd_cnt[SAMPLED_LD_NUM];

int main() {
    size_t A[30];
    
    // Hot CPU, optional
    stld_lab(&A[0], &A[10], &samples[0], &A[10]);

    for(int i = 0; i < SAMPLED_LD_NUM; ++ i) {
        upd_cnt[i] = 0;
    }

    for(int try = REPEAT; try > 0; -- try) {
        // Execute stld_lab containing several store-load pairs
        stld_lab(&A[0], &A[0], &samples[0], &A[10]);

        for(int l = 0; l < SAMPLED_LD_NUM; ++ l) {
            int val = 0;
            for(int i = 0; i < 20; ++ i) {
                // printf("%ld ", samples[i]);
                if (samples[i] > MDP_HIT_THRESHOLD) {
                    val ++;
                }
            }
            if (val > 10) {
                upd_cnt[l] ++;
            }
        }
    }

    for(int i = 0; i < SAMPLED_LD_NUM; ++ i) {
        printf("%.6f\n", (float) upd_cnt[i] / REPEAT);
    }    

    return 0;
}