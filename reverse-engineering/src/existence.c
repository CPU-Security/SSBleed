/*
 * Copyright (c) 2025 Chang Liu
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

uint64_t delayed_idx;
uint8_t unused[64];
uint8_t array1[64];
uint8_t array2[256 * PAGE_SIZE] __attribute__ ((aligned (1024)));
uint8_t temp = 0;

/*
 * We use a delayed store-load pair to test whether speculative store bypassing (SSB) is used,
 * and, if so, whether a memory dependence predictor (MDP) is used to predict whether SSB is performed.
 */
void stld(int new_val) {
    // The store is delayed by flushing the address &delayed_idx
    array1[delayed_idx] = new_val;
    // The address of this load is ready before the address of the prior store.
    // If SSB is performed, the load is executed out of order.
    // If a misprediction occurs, an old and potentially incorrect value will be forwarded to temp.
    temp = array1[0];
    // We use the Flush+Reload cache side channel to observe the speculatively forwared value.
    temp &= array2[temp * PAGE_SIZE];
}

/*
 * In this function, we execute several stld functions with different data dependence to train the MDP.
 * Then we trigger a delayed store-load pair with a data dependence, and use Flush+Reload to observe
 * whether a mispredicted SSB occurs.
 * @param len_alis: number of dependent store-load pairs (SA) in training
 * @param len_non_alias: number of independent store-load pairs (DA) in training
 * @return: number of mispredicted SSB
 */
int test(int len_alias, int len_non_alias) {
    int tries, i, mix_i, junk = 0;
    register uint64_t time1, time2;
    volatile uint8_t *addr;

    int cache_hit = 0;

    for(tries = 1000; tries > 0; tries--) {
        // Flush cache lines
        for(i = 0; i < 256; i++)
            flush(&array2[i * PAGE_SIZE]);
        
        // Train stld with len_alias dependent store-load pairs
        // store address: &array1[0]
        // load address: &array1[0]
        delayed_idx = 0;
        array1[0] = 0;
        for(i = 0; i < len_alias; ++ i) {
            flush(&delayed_idx);
            stld(0);
        }

        // Train stld with len_non_alias independent store-load pairs
        // store address: &array1[0]
        // load address: &array1[10]
        delayed_idx = 10;
        for(i = 0; i < len_non_alias; ++ i) {
            flush(&delayed_idx);
            stld(0);
        }

        // Trigger a potential mispredicted SSB with a dependent store-load pair
        // store address: &array1[0]
        // load address: &array1[0]
        // old loaded value: 0xf0
        // new loaded value: 0
        array1[0] = 0xf0;
        delayed_idx = 0;
        flush(&delayed_idx);
        stld(0);

        // Reload and probe cache states
        for(i = 0; i < 256; i++) {
            // Avoid effects of data prefetchers
            mix_i = ((i * 167) + 13) & 255;
            addr = &array2[mix_i * PAGE_SIZE];
            time1 = readCLK();
            junk = *addr;
            time2 = readCLK() - time1;
            if (time2 <= CACHE_HIT_THRESHOLD && mix_i == 0xf0)
                cache_hit++;
        }
    }
    return cache_hit;
}

int main(int argc, char** argv) {
    // Initialize pages to avoid CoW
    for(size_t i = 0; i < sizeof(array1); i++) 
        array1[i] = 0;
    for(size_t i = 0; i < sizeof(array2); i++) 
        array2[i] = 0;
    int alias = 0;
    int non_alias = 0;

    if (argc != 3) {
        printf("usage: ./existence <number-of-trained-alias-stld> <number-of-trained-nonalias-stld>\n");
        return 0;
    }
    else {
        alias = atoi(argv[1]);
        non_alias = atoi(argv[2]);
    }

    // Print the number of mispredicted SSB after training of alias SA and non_alias DA.
    printf("%d\n", test(alias,non_alias));
    return 0;
}