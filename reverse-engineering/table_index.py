# Copyright (c) 2025 Chang Liu
# SPDX-License-Identifier: Apache-2.0

import json
import os
import random
import matplotlib.pyplot as plt

def run(cpu = 1):
    e = os.popen(f"taskset -c {cpu} ./bin/table_index").read()
    data = e.strip().split("\n")
    print(data)
    addr_collided = [int(i,16) for i in data]
    with open("data/table_index.json", "w") as f:
        data = json.dump(addr_collided[1:], f)

def plot():
    data = []
    with open("data/table_index.json", "r") as f:
        data = json.load(f)
    x_tick = []
    print(data)
    print(data[-1] - data[0])
    y = [0 for i in range(data[-1] - data[0] + 100)]
    for i in range(0, len(data)):
        y[data[i] - data[0]] = 1
        x_tick.append(data[i] - data[0])
    plt.figure(figsize=(9, 3))
    plt.plot(range(len(y)), y, linestyle='-', linewidth=2, color='#f39c12')
    plt.xlabel("Instruction address offset", fontsize=12)
    plt.ylabel("Collided rate", fontsize=12)
    plt.xticks(x_tick, ['$0$', '$2^{15}$', '$2 \\times 2^{15}$', '$3 \\times 2^{15}$', '$4 \\times 2^{15}$'])
    plt.ylim([-0.2,1.2])
    plt.tick_params(labelsize=12)
    plt.tight_layout()
    plt.savefig('data/table_index.png')
    plt.show()


if __name__ == "__main__":
    run()
    plot()