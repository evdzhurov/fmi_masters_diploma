FROM ubuntu AS mpi-ubuntu

LABEL maintainer="edzhurov@uni-sofia.bg"

# Setup local time
ENV TZ=Europe/Sofia
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# Install packages
RUN apt-get update && \ 
    apt-get install -y autoconf automake gcc g++ make wget curl openssh-server

# Build MPI
RUN cd /usr/local/src/ && \
    wget http://www.mpich.org/static/downloads/3.2/mpich-3.2.tar.gz && \
    tar xf mpich-3.2.tar.gz && \
    rm mpich-3.2.tar.gz && \
    cd mpich-3.2 && \
    ./configure --disable-fortran && \
    make && make install
    #cd /usr/local/src && \
    #rm -rf mpich-3.2

# Setup ssh
RUN ssh-keygen -q -t rsa -N "" -f "/root/.ssh/id_rsa"
RUN cat /root/.ssh/id_rsa.pub >> /root/.ssh/authorized_keys
# Disable prompt for adding a new host
RUN /bin/bash -c 'echo -e "Host *\n\tStrictHostKeyChecking no" >> /root/.ssh/config' 

EXPOSE 22
# Required to start sshd
RUN mkdir /var/run/sshd

# --------------------------------------------------------------------------------------
FROM mpi-ubuntu AS mpi-dev-ubuntu

LABEL maintainer="edzhurov@uni-sofia.bg"

RUN apt-get update && \
    apt-get install -y inetutils-ping gdb

# --------------------------------------------------------------------------------------
FROM mpi-dev-ubuntu AS dist-apriori-miner

LABEL maintainer="edzhurov@uni-sofia.bg"

WORKDIR /usr/src/app

# Build app
COPY apriori_mpi.cpp .
COPY sample_tiny.csv .
RUN mpicxx apriori_mpi.cpp -o apriori_mpi