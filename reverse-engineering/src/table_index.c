/*
 * Copyright (c) 2025 Chang Liu
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "utils.h"


#define I_MEMORY_SIZE (40 * 4096)

#define LD_OFFSET 408

char stld[104 * 4];
void (*stld_func_base)(void* a1, void* a2);
void (*stld_func_collide)(void* a1, void* a2);
char *instruction_page_base, *instruction_page_collide;

/**
 * Generate the following machine codes:
 *  mov x2, #0x1
 *  .rep 50
 *  mul x0, x0, x2
 *  .endr
 *  str x2, [x0]
 *  ldr x3, [x1]
 *  .rep 50
 *  mul x3, x3, x2
 *  .endr
 *  ret
 */
void prepare_func() {
    char mov_x2_0x1[4] = {0x22, 0, 0x80, 0xd2};
    char mul_x0_x2[4] = {0, 0x7c, 0x02, 0x9b};
    char str_x2_x0[4] = {0x02, 0, 0, 0xf9};
    char ldr_x3_x1[4] = {0x23, 0, 0x40, 0xf9};
    char mul_x3_x2[4] = {0x63, 0x7c, 0x02, 0x9b};
    char ret[4] = {0xc0, 0x03, 0x5f, 0xd6};
    for(int i = 0; i < 4; ++ i) {   
        stld[i] = mov_x2_0x1[i];
    }
    for(int i = 0; i < 50; ++ i) {
        for(int j = 0; j < 4; ++ j) {
            stld[4 + i * 4 + j] = mul_x0_x2[j];
        }
    }
    for(int i = 0; i < 4; ++ i) {
        stld[i + 204] = str_x2_x0[i];
    }
    for(int i = 0; i < 4; ++ i) {
        stld[i + 208] = ldr_x3_x1[i];
    }
    for(int i = 0; i < 50; ++ i) {
        for(int j = 0; j < 4; ++ j) {
            stld[212 + i * 4 + j] = mul_x3_x2[j];
        }
    }
    for(int i = 0; i < 4; ++ i) {
        stld[i + 412] = ret[i];
    }
}

/**
 *  Create a delayed store-load pair in the memory space named instruction_page_base.
 */
void init_func_base() {
    prepare_func();
    srand(time(NULL));
    int rand_offset = rand() & 508;
    if ((instruction_page_base = (char*)mmap(NULL, 4096, PROT_READ | PROT_WRITE, \
        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) == (char*)-1) {
        printf("mmap failed\n");
        exit(-1);
    }
    for(int i = 0; i < 816; ++ i) {
        instruction_page_base[i + rand_offset] = stld[i];
        asm volatile("ic ivau, %0" :: "r"(&instruction_page_base[i]));
    }
    if (mprotect(instruction_page_base, 4096, PROT_READ | PROT_EXEC) != 0) {
        printf("mprotect failed\n");
        exit(-1);
    }
    stld_func_base = (void*)(instruction_page_base + rand_offset);
}

/**
 *  Initialize the memory space named instruction_page_collide.
 */
void init_func_collide() {
    if((instruction_page_collide = (char*)mmap(NULL, I_MEMORY_SIZE, PROT_READ | PROT_WRITE, \
        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) == (char*)-1) {
        printf("mmap failed\n");
        exit(-1);
    }
}

/**
 *  Inject codes to the instruction_page_collide dynamically.
 */
void update_func_collide(int offset) {
    prepare_func();
    if ((offset & 3) != 0) {
        printf("offset (%d) is not 4-byte aligned!\n", offset);
        exit(-1);
    }
    if (mprotect(instruction_page_collide, I_MEMORY_SIZE, PROT_READ | PROT_WRITE) != 0) {
        printf("mprotect failed\n");
        exit(-1);
    }
    for(int i = 0; i < 816; ++ i) {
        instruction_page_collide[i + offset] = stld[i];
        asm volatile("ic ivau, %0" :: "r"(&instruction_page_collide[i + offset]));
    }
    if (mprotect(instruction_page_collide, I_MEMORY_SIZE, PROT_READ | PROT_EXEC) != 0) {
        printf("mprotect failed\n");
        exit(-1);
    }
    stld_func_collide = (void*)(instruction_page_collide + offset);
}

void exit_func() {
    munmap(instruction_page_base, 4096);
    munmap(instruction_page_collide, I_MEMORY_SIZE);
}

int collide_lab() {
    int A[10];
    uint64_t time1, time2;
    uint64_t samples[100];

    // Init the MDP state with 30 DA
    // MDP counter value: -> 0
    for(int i = 0; i < 30; ++ i) {
        stld_func_base(&A[0], &A[8]);
        asm volatile("dsb ish");
        asm volatile("isb");
    }

    // Train the MDP state with 2 SA
    // MDP counter value: 0 -> 15
    for(int i = 0; i < 2; ++ i) {
        stld_func_base(&A[0], &A[0]);
        asm volatile("dsb ish");
        asm volatile("isb");
    }

    // Try to update the MDP state with another store-load pair
    // If the collision occurs, the MDP counter value will be updated
    // MDP counter value: 15-> 10 (if collided)
    for(int i = 0; i < 5; ++ i) {
        stld_func_collide(&A[0], &A[8]);
        asm volatile("dsb ish");
        asm volatile("isb");
    }

    // Probe the MDP state
    // MDP counter value: 15 / 10 -> 0
    for(int i = 0; i < 20; ++ i) {
        time1 = readCLK();
        stld_func_base(&A[0], &A[8]);
        time2 = readCLK();
        samples[i] = time2 - time1;  
    }

    int cnt = 0;
    for(int i = 0; i < 20; ++ i) {
        // printf("%lu ", samples[i]);
        if (samples[i] > MDP_HIT_THRESHOLD) {
            cnt ++;
        }
    }
    // printf("\n");
    return cnt == 10 ? 1 : 0;
}

int main() {
    init_func_base();
    init_func_collide();
    printf("0x%lx\n", (size_t) stld_func_base + LD_OFFSET);
    for(int i = 0; i < I_MEMORY_SIZE - 816 ; i += 4) {
        update_func_collide(i);
        int collide_cnt = 0;
        for (int try = 9; try > 0; -- try) {
            int res = collide_lab();
            collide_cnt += res;
        }
        if (collide_cnt > 5) {
            printf("0x%lx\n", (size_t) stld_func_collide + LD_OFFSET);
        }
    }
    exit_func();
    return 0;
}