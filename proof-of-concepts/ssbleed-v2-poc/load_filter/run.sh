#!/bin/bash
make clean -s
make
n=$1

for ((i=1; i<=n; i++))
do
    ./load_generator $2
    echo "Iteration $i completed."
done