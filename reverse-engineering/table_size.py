# Copyright (c) 2025 Chang Liu
# SPDX-License-Identifier: Apache-2.0

import json
import os
import random
import matplotlib.pyplot as plt

'''
Generate the stld function with sevel store-load pairs
The generated assembly function will be written to `data/asm.S`
'''
def generate_microbenchmark(random_ld_list, tested_ld_list, probe_ld_list):
    func_head = '''
.align 16
.global stld_lab
stld_lab: 
    mov x10, #0x1
'''
    trigger = '''.rept 10
    mul x0, x0, x10
    .endr
    str x2, [x0]
    ldr x4, [x1]
    dsb ish
    isb 
'''
    add_snippet = lambda nop_pading, tag: f'''
    b label_{tag:x}
    .rept {nop_pading}
    nop
    .endr
label_{tag:x}:
    {trigger}
'''
    add_probe = lambda nop_pading_tested_ld, tag_tested_ld: f'''
    mov x5, #20
    b label_{tag_tested_ld:x}
    .rept {nop_pading_tested_ld}
    nop
    .endr
label_{tag_tested_ld:x}:
    dsb sy
    isb
    mrs x6, cntvct_el0
    isb
    .rep 50
    mul x0, x0, x10
    .endr
    str x2, [x0]
    ldr x4, [x3]
    .rep 50
    mul x4, x4, x10
    .endr
    dsb ish
    isb 
    mrs x7, cntvct_el0
    isb
    sub x6, x7, x6
    str x6, [x2]
    add x2, x2, #8
    dsb sy
    isb
    sub x5, x5, #1
    cbnz x5, label_{tag_tested_ld:x}
'''
    add_snippet_loop = lambda snippet, loop_length, tag: f'''
    mov x5, #{loop_length}
label_{tag}:
{snippet}
    sub x5, x5, #1
    cbnz x5, label_{tag}
'''
    # Offset of a load in a snippet. nop is not counted
    ld_offset = 8 + 40  
    # Entry of a function
    code = func_head
    code_init = ''
    cur_addr = 8 
    random_ld_list = [i & (~3) for i in random_ld_list] # 4-byte aligned
    random_ld_list = list(set(random_ld_list))
    random_ld_list.sort()
    for i in range(len(random_ld_list)):
        j = 0
        while(j * (1 << 15) + random_ld_list[i] < cur_addr + ld_offset):
            j += 1
            continue
        n_nop = ((1 << 15) * j + random_ld_list[i] - cur_addr - ld_offset) // 4
        cur_addr += 4 + 4 * n_nop + 16 + 40
        code_init += add_snippet(n_nop, cur_addr - 12)
    code += add_snippet_loop(code_init, 20, cur_addr)
    cur_addr += 8

    code_test = ''
    tested_ld_list = [i & (~3) for i in tested_ld_list]
    for i in range(len(tested_ld_list)):
        j = 0
        while(j * (1 << 15) + tested_ld_list[i] < cur_addr + ld_offset):
            j += 1
            continue
        n_nop = ((1 << 15) * j + tested_ld_list[i] - cur_addr - ld_offset) // 4
        cur_addr += 4 + 4 * n_nop + 16 + 40
        code_test += add_snippet(n_nop, cur_addr - 12)
    
    code += code_test
    
    code_probe = ''
    prob_ld_offset = 228
    probe_ld_list = [i & (~3) for i in probe_ld_list]
    for i in range(len(probe_ld_list)):
        j = 0
        while(j * (1 << 15) + probe_ld_list[i] < cur_addr + prob_ld_offset):
            j += 1
            continue
        n_nop_prob = ((1 << 15) * j + probe_ld_list[i] - cur_addr - prob_ld_offset) // 4
        cur_addr += 4 * (119 + n_nop_prob)
        code_probe += add_probe(n_nop_prob, cur_addr - 248)
        
    code += code_probe

    # ret
    code += '''
    ret
'''
    # Write to asm.S
    with open("src/asm.S", "w") as f:
        f.write(code)
        
        
def build(num_samples = 1):
    e = os.popen(f"make table_org SAMPLED_LD_NUM={num_samples}").read()

def run(cpu = 1):
    e = os.popen(f"taskset -c {cpu} ./bin/table_org").read()
    upd_rate = eval(e.strip())
    return upd_rate

'''
Demonstrate that the table size of Neoverse-N2 MDP is 32
'''
def lab_size():
    random_ld_list = random.sample([60 * i for i in range(0, 540)], 100)
    tested_ld_list = []
    results = {}
    for size_tested in range(1, 40):
        while(len(tested_ld_list) < size_tested):
            idx = random.randint(0, (1 << 15)) & (~3)
            if idx not in random_ld_list and idx not in tested_ld_list:
                tested_ld_list.append(idx)
        probe_ld_list = [tested_ld_list[0]]
        generate_microbenchmark(random_ld_list, tested_ld_list, probe_ld_list)
        build(1)
        upd_rate = run()
        results[size_tested] = upd_rate
        print(f"size: {size_tested}, update rate: {upd_rate}")
    with open("data/table_size.json", "w") as f:
        json.dump(results, f)

def plot():
    data = {}
    with open("data/table_size.json", "r") as f:
        data = json.load(f)
    x = []
    y = []
    for k in data.keys():
        x.append(int(k))
        y.append(data[k])
    plt.figure(figsize=(9, 3))
    plt.plot(x, y, linestyle='-', linewidth=2, color='#f39c12')
    plt.xticks(range(0,40,8))
    plt.ylim([-0.2,1.2])
    plt.tick_params(labelsize=12)
    plt.savefig('data/table_size.png')
    plt.show()


if __name__ == "__main__":
    lab_size()
    plot()