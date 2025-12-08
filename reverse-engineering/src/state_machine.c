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
 * @param operation: define the dependent (SA) or independent (DA) store-load pairs used for training
 * @param operation_num: number of store-load pairs to be executed for training
 * @return: number of mispredicted SSB
 */
int test(int *operation, int operation_num) {
    int tries, i, mix_i, junk = 0;
    register uint64_t time1, time2;
    volatile uint8_t *addr;

    int cache_hit = 0;

    for(tries = 1000; tries > 0; tries--) {
        // Flush cache lines
        for(i = 0; i < 256; i++)
            flush(&array2[i * PAGE_SIZE]);
        
        // Initialize the MDP state
        array1[0] = 0;
        for(i = 0; i < 100; ++ i) {
            if (operation[i] == 0) {
                delayed_idx = 10;
                flush(&delayed_idx);
                stld(0);
            }
        }

        // Train stld with len_alias dependent store-load pairs       
        for(i = 0; i < operation_num; ++ i) {
            // an independent store-load pair (DA)
            // store address: &array1[0]
            // load address: &array1[10]
            if (operation[i] == 0) {
                delayed_idx = 10;
                flush(&delayed_idx);
                stld(0);
            }
            // a dependent store-load pair (SA)
            // store address: &array1[0]
            // load address: &array1[0]
            else {
                delayed_idx = 0;
                flush(&delayed_idx);
                stld(0);
            }
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
            // avoid effects of data prefetchers
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

    // Get input commands
    if (argc != 2) {
        printf("usage: ./state_machine <command-input-file>.\n");
        return 0;
    }
    FILE* fin = fopen(argv[1], "r");
    int operation_num;
    fscanf(fin, "%d", &operation_num);
    int *operation = malloc(sizeof(int) * operation_num);
    for(int i = 0; i < operation_num; ++ i) {
        fscanf(fin, "%d", &operation[i]);
    }
    fclose(fin);

    // Print the number of mispredicted SSB after training of alias SA and non_alias DA.
    printf("%d\n", test(operation, operation_num));
    return 0;
}