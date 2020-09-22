FROM ubuntu

LABEL maintainer="edj@fastmail.fm"

# Setup local time
ENV TZ=Europe/Sofia
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# Install packages
RUN apt-get update && \ 
    apt-get install -y autoconf automake gcc g++ make wget curl openssh-server inetutils-ping

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
COPY .ssh /root/.ssh
RUN chmod 600 /root/.ssh/id_rsa

EXPOSE 22
CMD ["service", "ssh", "start"]