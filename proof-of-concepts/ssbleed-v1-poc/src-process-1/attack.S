/*
 * Copyright (c) 2025 Chang Liu
 * SPDX-License-Identifier: Apache-2.0
 */
 
.align 16
.text
.global stld
.rep 16337
    nop
.endr
stld:
    mov x2, #0x1
    .rep 50
    mul x0, x0, x2
    .endr
    str x2, [x0]
    ldr x3, [x1]
    .rep 50
    mul x3, x3, x2
    .endr
    ret