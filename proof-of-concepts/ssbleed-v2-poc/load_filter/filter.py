# Copyright (c) 2025 Hongpei Zheng
# SPDX-License-Identifier: Apache-2.0

from collections import defaultdict
import numpy as np
import re
import sys

address_counter = defaultdict(int)

def first_iter(raw_runs):
    for run in raw_runs:
        for entry in run:
            address = entry["address"]
            address_counter[address] += 1

    threshold = 0.5 * len(raw_runs)
    stable_addresses = [addr for addr, cnt in address_counter.items() if cnt >= threshold]
    return stable_addresses

def second_iter(address_data):
    p_primary = []
    p_counter = []
    n_primary = []
    n_counter = []
    for entry in address_data:
        p_primary.append(entry["P"][0])
        p_counter.append(entry["P"][1])
        n_primary.append(entry["N"][0])
        n_counter.append(entry["N"][1])
    
    p_var = np.var(p_primary)
    p_counter_var = np.std(p_counter)
    n_var = np.var(n_primary)
    n_counter_var = np.std(n_counter)
    # print(address_data[0]["address"])
    # print(p_var, n_var)
    # print(p_counter_var + n_counter_var)
    return (p_var == 0) and (n_var == 0) and (p_counter_var + n_counter_var < 100)


def loaddata(input_path=None):
    with open("data/" + input_path, "r") as f:
        raw_runs = []
        current_run = []
        for line in f:
            if line.strip() == "":
                if current_run:
                    raw_runs.append(current_run)
                    current_run = []
            else:
                entry = {}
                parts = line.strip().split(":")
                entry["address"] = hex(int(parts[0], 16) & 0x7fff)
                data = parts[1]
                p_match = re.search(r'P \[(\d+)\]\((\d+)\)/\[(\d+)\]\((\d+)\)', data)
                n_match = re.search(r'N \[(\d+)\]\((\d+)\)/\[(\d+)\]\((\d+)\)', data)
                if p_match:
                    entry["P"] = [
                        int(p_match.group(1)), int(p_match.group(2))
                    ]
                if n_match:
                    entry["N"] = [
                        int(n_match.group(1)), int(n_match.group(2))
                    ]
                current_run.append(entry)
        if current_run:
            raw_runs.append(current_run)
    # print(len(raw_runs))
    return raw_runs
    
if __name__ == "__main__":
    input_path = sys.argv[1] if len(sys.argv) > 1 else None
    raw_runs = loaddata(input_path)
    stable_addresses = first_iter(raw_runs)
    print("Stable addresses:", stable_addresses)
    final_addresses = []
    for addr in stable_addresses:
        addr_data = [entry for run in raw_runs for entry in run if entry["address"] == addr]
        # print(addr_data)
        if second_iter(addr_data):
            final_addresses.append(addr)
            
    if len(stable_addresses) == 0:
        print("No stable addresses found. Please check the data or try to increase the number of runs.")
        exit(1)
    if len(final_addresses) == 0:
        print("No final addresses found. You can use stable address as the final address, but it may lead to bad results.")
    else:
        print("Final addresses:", final_addresses)