/*
 * Copyright (c) 2025 Hongpei Zheng
 * SPDX-License-Identifier: Apache-2.0
 */
.align 16
.global stld_kernel
.rep 0
    nop
.endr
stld_kernel:
    mov x2, #0x1
    .rep 30
    mul x0, x0, x2
    .endr
    str x2, [x0]
    ldr x3, [x1]
    .rep 30
    mul x3, x3, x2
    .endr
    ret