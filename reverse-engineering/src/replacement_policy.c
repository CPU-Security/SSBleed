/*
 * Copyright (c) 2025 Chang Liu
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include "utils.h"

#define REPEAT 5

extern void stld_lab(void* addr_store, void* addr_load_alias, void* addr_samples, void* addr_load_nonalias);

size_t samples[20];
int upd_cnt[20];

int main() {
    size_t A[30];
    int i, j, k;

    for(i = 0; i < 20; ++ i) {
        upd_cnt[i] = 0;
    }

    for(int try = REPEAT; try > 0; -- try) {
        // Execute stld_lab containing several store-load pairs
        stld_lab(&A[0], &A[0], &samples[0], &A[8]);
        int val = 0;
        for(i = 0; i < 20; ++ i) {
            // printf("%ld ", samples[i]);
            if (samples[i] > MDP_HIT_THRESHOLD) {
                val ++;
            }
        }
        upd_cnt[val] ++;
    }

    j = k = -1;
    for(i = 0; i < 20; i++) {
        if (j < 0 || upd_cnt[i] >= upd_cnt[j]) {
            k = j;
            j = i;
        } else if (k < 0 || upd_cnt[i] >= upd_cnt[k]) {
            k = i;
        }
    }

    printf("0x%lx ", (size_t)stld_lab);
    printf("%d %d %d %d\n", j, upd_cnt[j], k, upd_cnt[k]);

    return 0;
}