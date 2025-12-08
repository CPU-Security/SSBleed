# Copyright (c) 2025 Hongpei Zheng
# SPDX-License-Identifier: Apache-2.0

import matplotlib.pyplot as plt
import numpy as np
import sys

if len(sys.argv) > 1:
    filename = sys.argv[1]
else:
    print("Please provide a data file.")
    sys.exit(1)
title = 'Network Traffic'
fig, ax = plt.subplots(figsize=(8, 2.5))

data = np.loadtxt("data/" + filename)
x = data[:, 0]
timing = data[:, 1]
ground_truth = data[:, 2]
prediction = data[:, 3]

true_positive = np.where((ground_truth == 1) & (prediction == 1))[0]
false_positive = np.where((ground_truth == 0) & (prediction == 1))[0]
false_negative = np.where((ground_truth == 1) & (prediction == 0))[0]

ax.scatter(x, timing, color='gray', label='True Negative', s=15)
ax.scatter(x[true_positive], timing[true_positive], color='green', label='True Positive', s=15)
ax.scatter(x[false_positive], timing[false_positive], color='orange', label='False Positive', s=15)
ax.scatter(x[false_negative], timing[false_negative], color='red', label='False Negative', s=15)
ax.set_yticks([160, 180, 200, 220, 240])
ax.set_yticklabels([0, 20, 40, 60, 80])
ax.axhline(y=200, color='black', linestyle='--', clip_on=False)
textline = "\n↑\n|\n| not detected\n|\n↓\n-- Threshold\n↑\n|\n| detected\n|\n↓\n"
ax.text(1.01, 200, textline, color='black', va='center', ha='left', fontsize=11, fontfamily='monospace', transform=ax.get_yaxis_transform())
ax.legend(loc='center left')
plt.tight_layout()
plt.savefig('../interrupt.png')