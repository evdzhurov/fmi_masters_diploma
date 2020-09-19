FROM ubuntu_mpi_test

LABEL maintainer="edj@fastmail.fm"

WORKDIR /usr/src/app

COPY apriori_mpi.c .

RUN mpicc apriori_mpi.c -o apriori_mpi

ENTRYPOINT [ "mpirun", "-n", "2", "./apriori_mpi" ]