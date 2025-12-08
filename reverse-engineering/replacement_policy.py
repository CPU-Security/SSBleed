# Copyright (c) 2025 Chang Liu
# SPDX-License-Identifier: Apache-2.0

import json
import os
import re
import random

mdp_alignment = (1 << 15)

'''
Generate stld functions with several store-load pairs.
The generated assembly function will be written to `data/asm.S`.
'''
def generate_microbenchmark(random_ld_list, tested_ld_list, probe_ld):
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
label_{tag:x}:
{snippet}
    sub x5, x5, #1
    cbnz x5, label_{tag:x}
'''

    add_single_load_loop = lambda nop_pading, loop_length, tag: f'''
    mov x5, #{loop_length}
    b label_{tag:x}
    .rept {nop_pading}
    nop
    .endr
label_{tag:x}:
    ldr w4, [x2]
    isb
    sub x5, x5, #1
    cbnz x5, label_{tag:x}
'''
    ld_offset = 8 + 40 
    code = func_head
    code_init = ''
    cur_addr = 8
    random_ld_list = [i & (~3) for i in random_ld_list]
    random_ld_list = list(set(random_ld_list))
    random_ld_list.sort()
    for i in range(len(random_ld_list)):
        j = 0
        while(j * mdp_alignment + random_ld_list[i] < cur_addr + ld_offset):
            j += 1
            continue
        n_nop = (mdp_alignment * j + random_ld_list[i] - cur_addr - ld_offset) // 4
        cur_addr += 4 + 4 * n_nop + 16 + 40
        code_init += add_snippet(n_nop, cur_addr - 12)
    code += add_snippet_loop(code_init, 20, cur_addr)
    cur_addr += 8

    code_test = ''
    single_ld_offset = 8
    ldr_addr_offset = []
    for item in tested_ld_list:
        addr = item["addr"] & (~3)
        if not item["single"]:
            j = 0
            while(j * mdp_alignment +addr < cur_addr + ld_offset):
                j += 1
                continue
            n_nop = (mdp_alignment * j + addr - cur_addr - ld_offset) // 4
            cur_addr += 4 + 4 * n_nop + 16 + 40
            code_test += add_snippet(n_nop, cur_addr - 12)
            ldr_addr_offset.append(cur_addr - 12)
        else:
            rep_times = item["repeat"]
            j = 0
            while(j * mdp_alignment + addr < cur_addr + single_ld_offset):
                j += 1
                continue
            n_nop_single = (mdp_alignment * j + addr - cur_addr - single_ld_offset) // 4
            cur_addr += 4 * (6 + n_nop_single)
            code_test += add_single_load_loop(n_nop_single, rep_times, cur_addr - 16)
            ldr_addr_offset.append(cur_addr - 16)

    code += code_test
        
    code_probe = ''
    prob_ld_offset = 228
    probe_ld = probe_ld & (~3)
    j = 0
    while(j * mdp_alignment + probe_ld < cur_addr + prob_ld_offset):
        j += 1
        continue
    n_nop_prob = (mdp_alignment * j + probe_ld - cur_addr - prob_ld_offset) // 4
    cur_addr += 4 * (119 + n_nop_prob)
    code_probe += add_probe(n_nop_prob, cur_addr - 248)
    code += code_probe
    # ret
    code += '''
    ret
'''

    with open("src/asm.S", "w") as f:
        f.write(code)
    
    return ldr_addr_offset
        
def build():
    e = os.popen(f"make replacement_policy").read()

def run(cpu = 1):
    e = os.popen(f"taskset -c {cpu} ./bin/replacement_policy").read()
    data = e.strip().split(" ")
    addr = int(data[0], 16)
    upd_rate = [int(i) for i in data[1:]]
    return addr, upd_rate

'''
Demonstrate the following permutation of eviction priority when self-eviction occurs.
0 [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31]
1 [1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14, 17, 16, 19, 18, 21, 20, 23, 22, 25, 24, 27, 26, 29, 28, 31, 30]
2 [2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15, 18, 17, 16, 19, 22, 21, 20, 23, 26, 25, 24, 27, 30, 29, 28, 31]
3 [3, 0, 1, 2, 7, 4, 5, 6, 11, 8, 9, 10, 15, 12, 13, 14, 19, 16, 17, 18, 23, 20, 21, 22, 27, 24, 25, 26, 31, 28, 29, 30]
4 [4, 1, 2, 3, 0, 5, 6, 7, 12, 9, 10, 11, 8, 13, 14, 15, 20, 17, 18, 19, 16, 21, 22, 23, 28, 25, 26, 27, 24, 29, 30, 31]
5 [5, 0, 3, 2, 1, 4, 7, 6, 13, 8, 11, 10, 9, 12, 15, 14, 21, 16, 19, 18, 17, 20, 23, 22, 29, 24, 27, 26, 25, 28, 31, 30]
6 [6, 1, 0, 3, 2, 5, 4, 7, 14, 9, 8, 11, 10, 13, 12, 15, 22, 17, 16, 19, 18, 21, 20, 23, 30, 25, 24, 27, 26, 29, 28, 31]
7 [7, 0, 1, 2, 3, 4, 5, 6, 15, 8, 9, 10, 11, 12, 13, 14, 23, 16, 17, 18, 19, 20, 21, 22, 31, 24, 25, 26, 27, 28, 29, 30]
8 [8, 1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 24, 17, 18, 19, 20, 21, 22, 23, 16, 25, 26, 27, 28, 29, 30, 31]
9 [9, 0, 3, 2, 5, 4, 7, 6, 1, 8, 11, 10, 13, 12, 15, 14, 25, 16, 19, 18, 21, 20, 23, 22, 17, 24, 27, 26, 29, 28, 31, 30]
10 [10, 1, 0, 3, 6, 5, 4, 7, 2, 9, 8, 11, 14, 13, 12, 15, 26, 17, 16, 19, 22, 21, 20, 23, 18, 25, 24, 27, 30, 29, 28, 31]
11 [11, 0, 1, 2, 7, 4, 5, 6, 3, 8, 9, 10, 15, 12, 13, 14, 27, 16, 17, 18, 23, 20, 21, 22, 19, 24, 25, 26, 31, 28, 29, 30]
12 [12, 1, 2, 3, 0, 5, 6, 7, 4, 9, 10, 11, 8, 13, 14, 15, 28, 17, 18, 19, 16, 21, 22, 23, 20, 25, 26, 27, 24, 29, 30, 31]
13 [13, 0, 3, 2, 1, 4, 7, 6, 5, 8, 11, 10, 9, 12, 15, 14, 29, 16, 19, 18, 17, 20, 23, 22, 21, 24, 27, 26, 25, 28, 31, 30]
14 [14, 1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 30, 17, 16, 19, 18, 21, 20, 23, 22, 25, 24, 27, 26, 29, 28, 31]
15 [15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 31, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30]
16 [16, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31]
17 [17, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14, 1, 16, 19, 18, 21, 20, 23, 22, 25, 24, 27, 26, 29, 28, 31, 30]
18 [18, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15, 2, 17, 16, 19, 22, 21, 20, 23, 26, 25, 24, 27, 30, 29, 28, 31]
19 [19, 0, 1, 2, 7, 4, 5, 6, 11, 8, 9, 10, 15, 12, 13, 14, 3, 16, 17, 18, 23, 20, 21, 22, 27, 24, 25, 26, 31, 28, 29, 30]
20 [20, 1, 2, 3, 0, 5, 6, 7, 12, 9, 10, 11, 8, 13, 14, 15, 4, 17, 18, 19, 16, 21, 22, 23, 28, 25, 26, 27, 24, 29, 30, 31]
21 [21, 0, 3, 2, 1, 4, 7, 6, 13, 8, 11, 10, 9, 12, 15, 14, 5, 16, 19, 18, 17, 20, 23, 22, 29, 24, 27, 26, 25, 28, 31, 30]
22 [22, 1, 0, 3, 2, 5, 4, 7, 14, 9, 8, 11, 10, 13, 12, 15, 6, 17, 16, 19, 18, 21, 20, 23, 30, 25, 24, 27, 26, 29, 28, 31]
23 [23, 0, 1, 2, 3, 4, 5, 6, 15, 8, 9, 10, 11, 12, 13, 14, 7, 16, 17, 18, 19, 20, 21, 22, 31, 24, 25, 26, 27, 28, 29, 30]
24 [24, 1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 8, 17, 18, 19, 20, 21, 22, 23, 16, 25, 26, 27, 28, 29, 30, 31]
25 [25, 0, 3, 2, 5, 4, 7, 6, 1, 8, 11, 10, 13, 12, 15, 14, 9, 16, 19, 18, 21, 20, 23, 22, 17, 24, 27, 26, 29, 28, 31, 30]
26 [26, 1, 0, 3, 6, 5, 4, 7, 2, 9, 8, 11, 14, 13, 12, 15, 10, 17, 16, 19, 22, 21, 20, 23, 18, 25, 24, 27, 30, 29, 28, 31]
27 [27, 0, 1, 2, 7, 4, 5, 6, 3, 8, 9, 10, 15, 12, 13, 14, 11, 16, 17, 18, 23, 20, 21, 22, 19, 24, 25, 26, 31, 28, 29, 30]
28 [28, 1, 2, 3, 0, 5, 6, 7, 4, 9, 10, 11, 8, 13, 14, 15, 12, 17, 18, 19, 16, 21, 22, 23, 20, 25, 26, 27, 24, 29, 30, 31]
29 [29, 0, 3, 2, 1, 4, 7, 6, 5, 8, 11, 10, 9, 12, 15, 14, 13, 16, 19, 18, 17, 20, 23, 22, 21, 24, 27, 26, 25, 28, 31, 30]
30 [30, 1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14, 17, 16, 19, 18, 21, 20, 23, 22, 25, 24, 27, 26, 29, 28, 31]
31 [31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30]
'''
def lab():
    results = {}
    for try_cleared_entry in [0, 1, 2, 3, 15, 16, 30, 31]: 
        results[try_cleared_entry] = []
        for sz in range(31, 63):
            tested_ld_list = random.sample([{"single": False, "addr": i} for i in range(0, 11000, 8)], sz)
            tested_ld_list.insert(0, {"single": False, "addr": random.randint(24000, 30000) & (~3)})
            tested_ld_list.insert(1, {"single": False, "addr": random.randint(24000, 30000) & (~3)})
            # assert(len(tested_ld_list) >= 32)
            tested_ld_list.insert(32, {"single": True, "addr": -1, "repeat": 15})
            tested_ld_list[32]["addr"] = tested_ld_list[try_cleared_entry]["addr"]
            tested_ldaddr_list = [i["addr"] for i in tested_ld_list]
            # print(tested_ld_list[32]["addr"], len(tested_ld_list), tested_ldaddr_list)
            evicted_entry = []
            for probe_ld in tested_ldaddr_list:
                random_ld_list = random.sample([60 * i for i in range(200, 400)], 35)
                generate_microbenchmark(random_ld_list, tested_ld_list, probe_ld)
                build()
                base_addr, upd_rate = run()
                if (upd_rate[0] == 0 and probe_ld not in evicted_entry):
                    evicted_entry.append(probe_ld)
            print(f"Cleared Entry {try_cleared_entry}", f": evict entry {[tested_ldaddr_list.index(i) for i in evicted_entry]}")
            results[try_cleared_entry].append([tested_ldaddr_list.index(i) for i in evicted_entry])
    with open("data/replacement_policy.json", 'w') as f:
        json.dump(results, f)


'''
Parse the outputs to and print the permutation
'''
def permutation_analysis():
    evict_order_record = {}
    evict_order_analysis = {}
    data = []
    with open("data/replacement_policy.json", 'r') as f:
        data = json.load(f)
        
    for key in data.keys():
        try:
            for i in range(-1, len(data[key]) - 1):
                list_1 = data[key][i] if i > -1 else []
                list_2 = data[key][i + 1]
                new_item_list =  [item for item in list_2 if item not in list_1]
                assert(len(new_item_list) == 1)
                new_item = new_item_list[0]
                if (new_item >= 0 and new_item <= 31):
                    if key not in evict_order_analysis.keys():
                        evict_order_analysis[key] = []
                    assert(new_item not in evict_order_analysis[key])
                    evict_order_analysis[key].append(new_item)
                else:
                    break
        except:
            evict_order_analysis.pop(key)
            continue
    for key in evict_order_analysis.keys():
        to_be_analized = evict_order_analysis[key][1:]
    
    for key in evict_order_analysis.keys():
        print(f"PI_{key} = {evict_order_analysis[key][1:]}")

if __name__ == "__main__":
    lab()
    permutation_analysis()
