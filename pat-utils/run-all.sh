#!/usr/bin/env bash

#for cpu in `seq 0 95`;
#do
#    OMP_NUM_THREADS=1 numactl --physcpubind=$cpu ./omp-stream -n 5 -s 10000000 > /dev/null
#    echo -n "$cpu "
#    OMP_NUM_THREADS=1 numactl --physcpubind=$cpu ./omp-stream -n 100 -s 10000000 | tail -n 5 | head -n 1
#done

export OMP_NUM_THREADS=1

# Print header
echo -n "core_id,run_id,nthreads,"
../build/omp-stream --csv -n 3 -s 100 | head -n 4 | tail -n 1

start_time=$(date +%s)
count=0
nruns=25
size=10000000
for run_id in `seq 0 $((nruns-1))`;
do
    for core_id in `seq 0 95`;
    do
        prefix="$core_id,$run_id,1,"
        numactl --physcpubind=$core_id ./omp-stream --csv -n 100 -s $size | tail -n 5 | while read line; do echo "$prefix$line"; done
        count=$((count+1))
    done
done
end_time=$(date +%s)
elapsed_time=$(( end_time - start_time ))
mean_time=$(echo "scale=2;$elapsed_time/$count" | bc -l)
echo "$count runs in $elapsed_time seconds (mean $mean_time seconds)" 1>&2


#for size in 10000000 12000000 14000000 16000000;
#do
#    for i in `seq 1 5`;
#    do
#        OMP_NUM_THREADS=1 numactl --physcpubind=14 ./omp-stream -n 25 -s $size | tail -n 5 | head -n 1
#    done
#done
