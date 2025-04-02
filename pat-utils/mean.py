#!/usr/bin/env python3

import sys
import statistics
import numpy as np

agg = 1
if len(sys.argv) > 1:
    agg = int(sys.argv[1])

nums = []
for line in sys.stdin:
    nums.append(int(line))

if agg > 1:
    #print(np.array(nums).reshape(-1,agg).sum(axis=1).tolist())
    nums = np.array(nums).reshape(-1, agg).sum(axis=1).tolist()

min_   = min(nums)
max_   = max(nums)
mean   = statistics.mean(nums)
median = int(statistics.median(nums))
stdev  = statistics.stdev(nums)

print(f"Min:    {min_}")
print(f"Max:    {max_}")
print(f"Median: {median}")
print(f"Mean:   {mean:.2f}")
print(f"StdDev: {stdev:.2f}")
