/*
 * Copyright (c) 2025 Chang Liu
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

#define PAGE_SIZE 4096 // encode spaculatively loaded value into a seperated cache line
#define CACHE_HIT_THRESHOLD (100) // if addr cache hit, time(*addr) < 6
#define MDP_HIT_THRESHOLD (80)

/*
 * Read CNTVCT_EL0 to get a fing-grained timestamp
 */
static inline uint64_t readCLK(){
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

/*
 * Flush address p from all cache levels
 */
void flush(void *p) {
    asm volatile("dc civac, %0"::"r"(p));
    asm volatile("dsb ish");
    asm volatile("isb");
}

#endif