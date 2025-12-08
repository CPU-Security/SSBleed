/*
 * Copyright (c) 2025 Hongpei Zheng
 * SPDX-License-Identifier: Apache-2.0
 */
#include "inst.h"
#include <stdio.h>
#include <stdlib.h>

char stld[64 * 4];
char *instruction_page;

/**
 *  mov x2, #0x1
 *  .rep 30
 *  mul x0, x0, x2
 *  .endr
 *  str x2, [x0]
 *  ldr x3, [x1]
 *  .rep 30
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
    for(int i = 0; i < 30; ++ i) {
        for(int j = 0; j < 4; ++ j) {
            stld[4 + i * 4 + j] = mul_x0_x2[j];
        }
    }
    for(int i = 0; i < 4; ++ i) {
        stld[i + 124] = str_x2_x0[i];
    }
    for(int i = 0; i < 4; ++ i) {
        stld[i + 128] = ldr_x3_x1[i];
    }
    for(int i = 0; i < 30; ++ i) {
        for(int j = 0; j < 4; ++ j) {
            stld[132 + i * 4 + j] = mul_x3_x2[j];
        }
    }
    for(int i = 0; i < 4; ++ i) {
        stld[i + 252] = ret[i];
    }
}

void init_func_base() {
    prepare_func();
    if((instruction_page = (char*)mmap(NULL, I_MEMORY_SIZE, PROT_READ | PROT_WRITE, \
        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) == (char*)-1) {
        printf("mmap failed\n");
        exit(-1);
    }
}

void exit_func() {
    munmap(instruction_page, I_MEMORY_SIZE);
}

void update_func(int* offsets, uint64_t* func_entries, int entry_size) {
    if((instruction_page = (char*)mmap(NULL, I_MEMORY_SIZE, PROT_READ | PROT_WRITE, \
        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) == (char*)-1) {
        printf("mmap failed\n");
        exit(-1);
    }
    for(int i = 0; i < entry_size; ++ i) {
        int offset = offsets[i];
        // printf("offset: %d\n", offset);
        if ((offset & 3) != 0) {
            printf("offset (%d) is not 4-byte aligned!\n", offset);
            exit(-1);
        }

        for(int j = 0; j < 256; j ++) {
            instruction_page[j + offset] = stld[j];
            if ((j & 3) == 0)
                asm volatile("ic ivau, %0" :: "r"(&instruction_page[j + offset]));
        }

        func_entries[i] = (uint64_t)instruction_page + offset;
    }
    if(mprotect(instruction_page, I_MEMORY_SIZE, PROT_READ | PROT_EXEC) != 0) {
        printf("cannot execute\n");
        exit(-1);
    }
}