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

void update_func(int lsb12, uint64_t* func_entries, int entry_size) {
    if((instruction_page = (char*)mmap(NULL, I_MEMORY_SIZE, PROT_READ | PROT_WRITE, \
        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) == (char*)-1) {
        printf("mmap failed\n");
        exit(-1);
    }
    if (lsb12 < LD_OFFSET) {
        lsb12 = lsb12 + PG_SIZE - LD_OFFSET;
    } else {
        lsb12 = lsb12 - LD_OFFSET;
    }
    for(int i = 0; i < entry_size; ++ i) {
        if ((lsb12 & 3) != 0) {
            printf("offset (%d) is not 4-byte aligned!\n", lsb12);
            exit(-1);
        }

        for(int j = 0; j < 256; j ++) {
            instruction_page[j + lsb12 + i*PG_SIZE] = stld[j];
            if ((j & 3) == 0)
                asm volatile("ic ivau, %0" :: "r"(&instruction_page[j + lsb12]));
        }

        func_entries[i] = (uint64_t)instruction_page + lsb12 + i*PG_SIZE;
        // printf("func_entries[%d]: 0x%p\n", i, func_entries[i]);
    }
    if(mprotect(instruction_page, I_MEMORY_SIZE, PROT_READ | PROT_EXEC) != 0) {
        printf("cannot execute\n");
        exit(-1);
    }
}

void update_multifunc(int* lsb_entries, uint64_t* func_entries, int entry_size) {
    if (entry_size > 8) {
        printf("entry_size should be less than or equal to 8!\n");
        exit(-1);
    }
    char *instruction_pages[MAX_FUNC_ENTRIES];
    for(int i = 0; i < entry_size; i++) {
        if((instruction_pages[i] = (char*)mmap(NULL, I_MEMORY_SIZE, PROT_READ | PROT_WRITE, \
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) == (char*)-1) {
            printf("mmap failed\n");
            exit(-1);
        }
    }
    for(int i = 0; i < entry_size; i++) {
        int lsb15 = lsb_entries[i];
        uint64_t page_lsb15 = (uint64_t)instruction_pages[i] & 0x7fff;
        int offset;
        // printf("lsb15: %lx, page_lsb15: %lx\n", lsb15, page_lsb15);
        if (lsb15 < page_lsb15 + LD_OFFSET) {
            offset = lsb15 + 8 * PG_SIZE - page_lsb15 - LD_OFFSET;
        } else {
            offset = lsb15 - page_lsb15 - LD_OFFSET;
        }
        if ((offset & 3) != 0) {
            printf("offset (%d) is not 4-byte aligned!\n", offset);
            exit(-1);
        }

        for(int j = 0; j < 256; j ++) {
            instruction_pages[i][j + offset] = stld[j];
            if ((j & 3) == 0)
                asm volatile("ic ivau, %0" :: "r"(&instruction_pages[i][j + lsb15]));
        }

        func_entries[i] = (uint64_t)instruction_pages[i] + offset;
    }
    for(int i = 0; i < entry_size; ++ i) {
        if(mprotect(instruction_pages[i], I_MEMORY_SIZE, PROT_READ | PROT_EXEC) != 0) {
            printf("cannot execute\n");
            exit(-1);
        }
    }
}