/*
 * Copyright (c) 2025 Hongpei Zheng
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdint.h>
#include <sched.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "driver.h"
#include "inst.h"

#define MDP_THRESHOLD 200
#define SIZE_EVICT_SET 32
#define SIZE_PRIMITIVE 32
#define TRY_TIMES 100
FILE* syscallfile;
FILE* logfile;
int results[TRY_TIMES][2];
double timestamps[TRY_TIMES];
int stldtime[TRY_TIMES][8][16];

double get_timestamp() {
    static struct timeval start_time;
    static int first_call = 1;
    
    if (first_call) {
        gettimeofday(&start_time, NULL);
        first_call = 0;
        return 0.0;
    }
    
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    
    return (double)(current_time.tv_sec - start_time.tv_sec) * 1000000.0 +
           (double)(current_time.tv_usec - start_time.tv_usec);
}


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
    snprintf(path, sizeof(path), "data/%s", filename); // 拼接路径
    FILE* filetmp = fopen(path, "w");
    if (filetmp == NULL) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }
    return filetmp;
}

void closefile(FILE* file) {
    fclose(file);
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

void monitor(int offset, uint64_t* entries, int entry_len) {
    // monitor specific load offset in entries, check its MDP residue state to see if it was executed during interrupt
    int A[10];
    int sample[entry_len][2][16];
    uint64_t t, end, trigger;
    int index;
    for(int i=0; i<entry_len; i++) {
        if (((entries[i] + LD_OFFSET) & 0x7fff) == offset) {
            index = i;
        }
    }
    void(*stld[entry_len])(void*, void*);
    void* tmp;
    for(int i = 0; i < entry_len; ++ i) {
        stld[i] = (void*)entries[i];

    }
    for (int i=0; i<TRY_TIMES; i++) {
        results[i][0] = 0;
        results[i][1] = 0;
        timestamps[i] = 0.0;
    }
    for (int iter=0; iter<TRY_TIMES; iter++) {
        trigger = 0;
        // usleep(1);
        // 模拟随机中断事件
        int rand_int = random() % 10;
        if (rand_int == 0) {
            trigger = 1;
            results[iter][0] = 1;
        } else if (rand_int == 1) {
            trigger = 2;
            results[iter][0] = 2;
        } else if (rand_int == 2) {
            trigger = 3;
            results[iter][0] = 3;
        } else {
            results[iter][0] = 0;
        }
        timestamps[iter] = get_timestamp();
        for(int i = 0; i < 16; ++ i) {
            for(int j = 0; j < entry_len; ++ j)
                sample[j][0][i] = sample[j][1][i] = 0;
        }
        for (int round = 0; round < 2; ++ round) {
                for (int i = 0; i < entry_len; ++ i) {
                    for(int j = 0; j < 3; ++ j) {
                        stld[i](&A[0], &A[0]);
                    }
                }
                if (round == 1)
                    switch (trigger)
                    {
                    case 1:
                        send_raw_packet();
                        break;
                    case 2:
                        tmp = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);
                        break;
                    case 3:
                        syscallfile = fopen("data/tmp.txt", "a+");
                    default:
                        break;
                    }
        
                for (int i = 0; i < entry_len; ++ i) {
                    int val = 0;
                    for(int j = 0; j < 15; ++ j) {
                        t = rdtsc();
                        stld[i](&A[0], &A[4]);
                        end = rdtsc();
                        if(end - t > MDP_THRESHOLD) {
                            val += 1;
                        }
                        stldtime[iter][i][j] = end - t;
                    }
                    if (val >= 0 && val < 16) {
                        sample[i][round][val] ++;
                    }
                }
            // }
        }
        int value[2][2];
        if (syscallfile != NULL) {
            closefile(syscallfile);
            syscallfile = NULL;
        }
        
        value[0][0] = sample[index][0][14];
        value[0][1] = sample[index][1][14];
        value[1][0] = sample[index][0][15];
        value[1][1] = sample[index][1][15];
        if (value[0][0] != value[0][1] || value[1][0] != value[1][1]) {
            results[iter][1] = 1;
        }
    }
    munmap(tmp, 4096);
    for (int i = 0; i < TRY_TIMES; ++ i) {
        fprintf(logfile, "%.2f %d %d %d\n", timestamps[i], stldtime[i][index][14], results[i][0], results[i][1]);
    }
}

int main(int argc, char* argv[]) {
    logfile = open_file(argv[1]);
    bind_cpu(1);
    srand(time(NULL));
    init_func_base();
    int func_num = 8;
    uint64_t entry[func_num];
    int offset = (int)strtol(argv[2], NULL, 16);
    update_func(offset&0xfff, entry, func_num);
    monitor(offset, entry, func_num);
    exit_func();
    closefile(logfile);
    return 0;
}