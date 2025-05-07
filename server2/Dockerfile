# server2/Dockerfile
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    gcc \
    g++ \
    build-essential \
    libc6-dev \
    libx11-dev \
    libxkbcommon-dev \
    libxkbcommon-x11-dev \
    git \
    make \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/nonpop/xkblayout-state.git \
    && cd xkblayout-state \
    && make \
    && make install

WORKDIR /app
COPY server2.c .
RUN gcc server2.c -o server2 -lpthread -lX11 -lxkbcommon

CMD ["./server2"]
