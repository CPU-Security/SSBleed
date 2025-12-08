/*
 * Copyright (c) 2025 Hongpei Zheng
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef DRIVER_H
#define DRIVER_H

#define _GNU_SOURCE
#include <sched.h>  // sched_setaffinity
#include <fcntl.h>  // O_RDWR
#include <unistd.h>  // close
#include <stdlib.h>  // malloc
#include <sys/ioctl.h>  // ioctl

#define CPU 0

#define DEVICE "/dev/mdu_kernel"

void bind_cpu(int core_id);
void init_device();
void exit_device();
void context_switch(int cmd);

#endif