# Copyright (c) 2025 Chang Liu
# SPDX-License-Identifier: Apache-2.0

import json
import os
import time
import matplotlib.pyplot as plt

def run(input_seq):
    # Generate an input file
    f_input = "data/input.txt"
    seqs = [int(d) for d in input_seq.split("_")]
    seq_in_txt = [0]
    for i in range(0, len(seqs), 2):
        for j in range(seqs[i]):
            if (seqs[i + 1] == 0):
                seq_in_txt.append(0)
            else:
                seq_in_txt.append(1)
    seq_in_txt[0] = len(seq_in_txt) - 1
    with open(f_input, "w") as f:
        for i in range(len(seq_in_txt)):
            f.write(f"{seq_in_txt[i]} ")
        f.write("\n")

    # Run bin/existence with different inputs
    e = os.popen(f"./bin/state_machine {f_input}")
    return eval(e.read().strip())

def print_state_sm():
    # Print experimental results of state machine test
    with open("data/state_machine.json", "r") as f:
        data = json.load(f)
    for key in data.keys():
        print(f"{key}: {'SSB' if data[key] > 50 else 'BLK'}")

def experiment():
    results = {}
    results["15 DA"] = run("15_0")
    results["15 DA, 1 SA"] = run("15_0_1_1")
    results["1 SA, 13 DA"] = run("1_1_13_0")
    results["1 SA, 14 DA"] = run("1_1_14_0")
    results["10 SA, 14 DA"] = run("10_1_14_0")
    results["10 SA, 15 DA"] = run("10_1_15_0")
    results["1 SA, 10 DA, 1 SA, 4 DA"] = run("1_1_10_0_1_1_4_0")
    results["1 SA, 10 DA, 1 SA, 5 DA"] = run("1_1_10_0_1_1_5_0")

    with open("data/state_machine.json", "w") as f:
        json.dump(results, f)

if __name__ == "__main__":
    experiment()
    print_state_sm()