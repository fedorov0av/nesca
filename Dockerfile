# syntax=docker/dockerfile:1

FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    pkg-config \
    qtbase5-dev \
    qtbase5-dev-tools \
    qt5-qmake \
    qtchooser \
    qtmultimedia5-dev \
    libqt5multimedia5-plugins \
    libssh-dev \
    libssl-dev \
    libcurl4-openssl-dev \
    libgl1-mesa-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/nesca
COPY src/ ./src/

WORKDIR /opt/nesca/src
RUN qmake -v && qmake && make -j"$(nproc)"

ENV QT_QPA_PLATFORM=offscreen
CMD ["./nesca"]
