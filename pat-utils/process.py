#!/usr/bin/env python3
import sys
import pandas as pd
import numpy as np

def pctdiff(value1, value2):
    """Calculate the percent difference between two values."""
    if value1 == 0 and value2 == 0:
        return 0.0  # Avoid division by zero if both values are zero
    elif value1 == 0 or value2 == 0:
        return float('inf')  # If one value is zero, percent difference is infinite

    # Calculate the absolute difference
    absolute_difference = abs(value1 - value2)

    # Calculate the average of the two values
    average = (value1 + value2) / 2

    # Calculate the percent difference
    percent_diff = (absolute_difference / average) * 100

    return percent_diff

if len(sys.argv) < 2:
    print("Usage: ./process.py <filename>")
    sys.exit(1)

with open(sys.argv[1]) as file:
    df = pd.read_csv(file)

cores = np.unique(df['core_id'])
functions = np.unique(df['function'])
nruns = np.max(df['run_id']) + 1

#print(cores)
#print(functions)
#print(nruns)

df = df[df['function'] == 'Copy']
df = df.drop('function', axis=1)
#print(df.columns)
df_mean = df.groupby('core_id').mean().reset_index()
df_std  = df.groupby('core_id').std().reset_index()
for i, (mean, std) in enumerate(zip(df_mean['max_mbytes_per_sec'], df_std['max_mbytes_per_sec'])):
    top = mean+std
    bottom = mean-std
    print(i, f'{bottom:.0f}-{top:.0f} ({pctdiff(top,bottom):.2f})')
    if i == 55:
        break


#print(df)
