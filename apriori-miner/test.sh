#!/bin/bash

JOB_ID="test-job"
N_PROC=2

DATA_PATH="/var/dist-apriori-data"
JOB_PATH="${DATA_PATH}/${JOB_ID}"
MPI_LOG_FILE_PATH="${JOB_PATH}/miner-%r.log"
MPI_ERR_FILE_PATH="${JOB_PATH}/miner-%r.err"

echo "Running ${JOB_ID} with ${N_PROC} processes..."

rm -rf ${JOB_PATH}
mkdir ${JOB_PATH}

mpiexec -n ${N_PROC} -outfile-pattern=${MPI_LOG_FILE_PATH} -errfile-pattern=${MPI_ERR_FILE_PATH} ./apriori_mpi --input sample_tiny.csv