/*
 * Copyright (c) 2025 Chang Liu
 * SPDX-License-Identifier: Apache-2.0
 */
 
.global speculative_execution
.align 16
.rep 42
nop
.endr
speculative_execution:
    mov x8, x0
    mov x9, #1
    ldr x8, [x8]
    cmp x3, x8
    b.hi func_end
    ldrb w8, [x1, x3] 
    lsr x10, x8, x4
    and x10, x10, x9  // speculatively loaded bit: 0 / 1
    lsl x10, x10, #3
    str x2, [x2, x10] // delayed store
    ldr x0, [x2]      // speculative load
func_end:
    ret


.align 16
.global stld
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