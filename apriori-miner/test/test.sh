#!/bin/bash

JOB_ID="test-job"
N_PROC=2
CSV_FILE=sample_tiny.csv
MAX_K=1
MIN_SUPPORT=0.05
MIN_CONFIDENCE=1.0

DATA_PATH="/var/dist-apriori-data"
if [ ! -d ${DATA_PATH} ]; then
    DATA_PATH="dist-apriori-data"
fi

JOB_PATH="${DATA_PATH}/${JOB_ID}"
MPI_LOG_FILE_PATH="${JOB_PATH}/miner-%r.log"
MPI_ERR_FILE_PATH="${JOB_PATH}/miner-%r.err"

echo "Running ${JOB_ID} with ${N_PROC} processes..."

rm -rf ${JOB_PATH}
mkdir -p ${JOB_PATH}

mpiexec -n ${N_PROC} -outfile-pattern=${MPI_LOG_FILE_PATH} -errfile-pattern=${MPI_ERR_FILE_PATH} ./apriori_mpi --input ${CSV_FILE} --max_k ${MAX_K} --min_sup ${MIN_SUPPORT} --min_conf ${MIN_CONFIDENCE}