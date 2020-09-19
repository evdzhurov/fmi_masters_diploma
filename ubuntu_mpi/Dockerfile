FROM ubuntu

LABEL maintainer="edj@fastmail.fm"

RUN apt-get update && apt-get install -y autoconf automake gcc g++ make wget curl

RUN cd /usr/local/src/ && \
    wget http://www.mpich.org/static/downloads/3.2/mpich-3.2.tar.gz && \
    tar xf mpich-3.2.tar.gz && \
    rm mpich-3.2.tar.gz && \
    cd mpich-3.2 && \
    ./configure --disable-fortran && \
    make && make install
    #cd /usr/local/src && \
    #rm -rf mpich-3.2

#ENV PATH=/usr/bin:/usr/local/bin:/bin:/app