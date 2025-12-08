# Copyright (c) 2025 Chang Liu
# SPDX-License-Identifier: Apache-2.0

import matplotlib.pyplot as plt
import seaborn as sns
import os
import json

'''
Run bin/timing_difference to generate timing samples of BLK states and SSB states
'''
def run():
    e = os.popen(f"./bin/timing_difference")
    out = e.read().strip()
    data_v0 = out.split("\n")
    data_blk = []
    data_ssb = []
    for i in range(len(data_v0)):
        data_each_line = [int(j) for j in data_v0[i].strip().split(" ")]
        data_blk.extend(data_each_line[:15])
        data_ssb.extend(data_each_line[15:])
    with open("data/timing_difference.json", "w") as f:
        json.dump([data_blk, data_ssb], f)

def plot_distribution():
    data = []
    with open("data/timing_difference.json", "r") as f:
        data = json.load(f)
    distribute_0 = data[0]
    distribute_1 = data[1]

    plt.figure(figsize=(5, 2))
    sns.kdeplot(distribute_0, shade=True, color="#4682B4", label='BLK State')
    sns.kdeplot(distribute_1, shade=True, color="#DAA520", label='SSB State')
    plt.xlabel("Execution time (from CNTVCT)", fontsize=12)
    plt.ylabel("Density", fontsize=12)
    plt.legend(prop={'size': 10})
    plt.xticks(fontsize=12)
    plt.tight_layout()
    plt.savefig("data/timing_difference.png")

if __name__ == "__main__":
    run()
    plot_distribution()