# SSBleed-v1 Proof-of-concept

## Introduction

SSBleed-v1 exploits two security issues of the MDP. First, the MDP is not isolated between processes running on the same core. Second, a load without preceding stores can still update the MDP counter.

In this PoC, we demonstrate how one process can leak the control flow of another. We create two processes, with `process-1` serving as the attacker and `process-2` as the victim. The victim process contains a secret-dependent branch as follows:

```c
if (secret_bit) {
    // load_1
}
else {
    // load_2, optional
}
```

If `secret_bit` equals 0, the first load is executed. Otherwise, the second load is executed.

Before each attack round, the attacker executes three data-dependent store–load pairs to initialize the MDP counter value to 15. The load used by the attacker has the same lowest 15 bits of its address with the victim’s load. Then, the victim executes the secret-dependent branch. Finally, the attacker probes whether the counter value has been updated to 14. If the counter has been updated, the attacker infers that `load_1` is executed and thus `secret_bit` equals 1. Otherwise, `secret_bit` equals 0.

For simplicity, semaphores are used to synchronize the two processes.

## Build

A C compiler is required. For example, we use gcc 13.3.0 with make 4.3. No specific kernel or package dependencies and installations are required. The executable files `process-1` and `process-2` can be built through a simple command:

```shell
make
```

## Run

There are two ways to run the SSBleed-v1 PoC. 

### 1. Execute processes directly

First, launch two terminals. In the first terminal, run the attacker process:

```shell
taskset -c 1 ./process-1
```

In the second terminal, run the victim process:

```shell
taskset -c 1 ./process-2
```

### 2. Use the script

We prepare a script to automatically start two processes. To use the script, simply run

```shell
./run.sh
```

## Expected Results

The attacker process `process-1` will infer the secrets in the victim process `process-2` by observing the target of the secret-dependent branch in the victim space through a cross-process MDP side-channel attack.

The leaked secret bytes are printed by `process-1`, and the accuracy and throughput will also be reported.

The expected results are as follows:

```shell
./run.sh 
S (53)
S (53)
B (42)
l (6c)
e (65)
e (65)
d (64)
- (2d)
V (56)
1 (31)
  (20)
P (50)
r (72)
o (6f)
o (6f)
f (66)
- (2d)
o (6f)
f (66)
- (2d)
c (63)
o (6f)
n (6e)
c (63)
e (65)
p (70)
t (74)
  (20)
o (6f)
n (6e)
  (20)
N (4e)
e (65)
o (6f)
v (76)
e (65)
r (72)
s (73)
e (65)
- (2d)
N (4e)
2 (32)
 (0)
accuracy: 1.0000, throughput: 55243.2953 bps
```