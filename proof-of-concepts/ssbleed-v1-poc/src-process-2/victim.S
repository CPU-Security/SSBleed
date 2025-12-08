/*
 * Copyright (c) 2025 Chang Liu
 * SPDX-License-Identifier: Apache-2.0
 */
 
.align 16
.text
.global secret_dependent_branch
secret_dependent_branch:
    dsb ish         // memory barrier: avoid speculation
    isb
    cmp x0, #0      // if (x0 == 0)
    b.eq ldr_2      //   goto ldr_2
    nop
ldr_1:              // else
    ldr x0, [sp]    //   x0 = *sp
    ret             //   return
ldr_2:
    ldr x0, [sp]    //   x0 = *sp
    ret             //   return