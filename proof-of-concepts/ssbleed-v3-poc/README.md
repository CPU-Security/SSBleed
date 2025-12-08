# SSBleed-v3 Proof-of-concept

## Introduction

SSBleed-v3 exploits two security issues of the MDP. First, the MDP can be updated by loads that are speculatively executed but not committed. Second, the MDP state update is independent of the cache and prefetcher states, enabling a more stable transient-execution attack.

In this proof-of-concept (PoC), we verify that the MDP can be updated during speculative execution. We train the conditional branch predictor (CBP) to trigger a misprediction (i.e., transient execution). During transient execution, we execute a data-dependent store–load pair. After transient execution ends, we probe the MDP state to verify that it has been updated by the speculatively executed but uncommitted store–load pair.

## Build

A C compiler is required. For example, we use gcc 13.3.0 with make 4.3. No specific kernel or package dependencies and installations are required. The executable files `process-1` and `process-2` can be built through a simple command:

```shell
make
```

## Run

To start the SSBleed-v3 PoC, simply run

```shell
./poc
```

## Expected Results

The expected results are as follows:

```shell
./poc 
S (53)
S (53)
B (42)
l (6c)
e (65)
e (65)
d (64)
- (2d)
V (56)
3 (33)
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
accuracy: 1.0000, throughput: 9202.4540 bps
```