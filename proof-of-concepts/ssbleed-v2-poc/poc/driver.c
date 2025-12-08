/*
 * Copyright (c) 2025 Hongpei Zheng
 * SPDX-License-Identifier: Apache-2.0
 */
#include "driver.h"
#include <stdio.h>

int fd;
struct ioctl_param {
	int cpu;
    size_t res;
} param = {CPU, 0};

void bind_cpu(int core_id) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(core_id, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);
}


void init_device() {
    fd = open(DEVICE, O_RDWR);
    if(fd == -1) {
        printf("Cannot open device, maybe kernel driver is forgotten?\n");
        exit(0);
    }
}

void exit_device() {
    close(fd);
}

void context_switch(int cmd) {
    ioctl(fd, cmd, &param);
}