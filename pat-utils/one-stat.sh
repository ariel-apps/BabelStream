#!/usr/bin/env bash
set -euo pipefail

sz=$((2**21))
if [[ $# -eq 1 ]];
then
    sz=$1
fi

#echo "nthreads size stat"

for n in `seq 1 20`;
do
    echo -n "$n $sz "
    OMP_NUM_THREADS=$n ../build/install/bin/omp-stream -n 100 -s $sz | grep "L1D" | cut -d' ' -f 4 | ./mean.py $n | grep -i "median" | cut -d' ' -f 2
done
