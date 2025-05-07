# server1/Dockerfile
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    gcc \
    libc6-dev \
    && rm -rf /var/lib/apt/lists/*

COPY server1.c .
RUN gcc server1.c -o server1 -lpthread

CMD ["./server1"]
