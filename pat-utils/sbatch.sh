#!/bin/bash
#SBATCH --job-name=bs-omp    # Job name
#SBATCH --output=bs-omp.out  # Standard output and error log
#SBATCH --error=bs-omp.err   # Error log
#SBATCH --nodes=1            # Number of nodes
#SBATCH --ntasks=1           # Total number of MPI tasks
#SBATCH --time=08:00:00      # Time limit hrs:min:sec
#SBATCH --partition=PV       # Partition name
#SBATCH --nodelist=blake16

./run-all.sh > omp-core-data-transpose-blake16.txt
