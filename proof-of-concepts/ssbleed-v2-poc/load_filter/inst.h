/*
 * Copyright (c) 2025 Hongpei Zheng
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef INST_H
#define INST_H

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdint.h>

#define I_MEMORY_SIZE (10 * 4096)
#define LD_OFFSET 128
#define PG_SIZE 4096

void init_func_base();
void exit_func();
void update_func(int* offsets, uint64_t* func_entries, int entry_size);

#endif