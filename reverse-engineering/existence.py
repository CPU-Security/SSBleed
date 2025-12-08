# Copyright (c) 2025 Chang Liu
# SPDX-License-Identifier: Apache-2.0

import json
import os
import matplotlib.pyplot as plt

def run(num_alias, num_nonalis):
    # Run bin/existence with different inputs
    e = os.popen(f"./bin/existence {num_alias} {num_nonalis}")
    return eval(e.read().strip())

def plot():
    # Fetch data
    with open("data/existence.json", "r") as f:
        data = json.load(f)
    data_1 = [i / 1000 for i in data[0][:30]]
    data_2 = [i / 1000 for i in data[1][:30]]
    x = range(len(data_1))
    # Plot data/existence.png
    plt.figure(figsize=(6, 4))
    plt.plot(x, data_1, linestyle='--', marker='o', color='#f39c12', label='100SA-$k$DA')
    plt.plot(x, data_2, linestyle='-', linewidth=3, color='#5dade2', label='$k$SA-100DA')
    plt.xlabel("Number of trained store-load pairs ($k$)", fontsize=14)
    plt.ylabel("Mispredicted SSB rate", fontsize=14)
    plt.tick_params(axis='both', labelsize=14)
    plt.legend(fontsize=14, loc='center left')
    plt.tight_layout()
    plt.savefig('data/existence.png', dpi=600)
    plt.show()

def experiment():
    result_alias_100 = []
    result_nonalias_100 = []
    for i in range(100):
        result_alias_100.append(run(100, i))
    for i in range(100):
        result_nonalias_100.append(run(i, 100))
    with open("data/existence.json", "w") as f:
        json.dump([result_alias_100, result_nonalias_100], f)

if __name__ == "__main__":
    experiment()
    plot()