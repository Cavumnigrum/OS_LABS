# client/Dockerfile
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    gcc \
    libgtk-3-dev \
    libc6-dev \
    x11-apps\
    && rm -rf /var/lib/apt/lists/*

COPY client.c .
RUN gcc client.c -o client `pkg-config --cflags --libs gtk+-3.0` -lpthread

CMD ["./client"]
