ARG UBUNTU_VERSION=18.04
FROM ubuntu:$UBUNTU_VERSION

RUN set -x \
 && : Remove pre-bundled libunwind \
 && find /usr -name "*libunwind*" -delete \
 && apt update \
 && apt install -y --no-install-recommends \
            g++ openjdk-8-jdk-headless cmake make curl git \
            zlib1g-dev \
            libgc-dev libunwind8-dev libre2-dev \
 && rm -rf /var/lib/apt/lists/*

RUN set -x \
 && curl -Ls https://raw.githubusercontent.com/paulp/sbt-extras/master/sbt > /usr/bin/sbt \
 && chmod 0755 /usr/bin/sbt

ARG LLVM_VERSION=6.0
ENV LLVM_VERSION=$LLVM_VERSION
RUN set -x \
 && apt update \
 && apt install -y --no-install-recommends \
            clang-$LLVM_VERSION clang-format-$LLVM_VERSION \
            libclang-$LLVM_VERSION-dev llvm-$LLVM_VERSION-dev \
 && rm -rf /var/lib/apt/lists/*

ENV PATH=$PATH:/usr/lib/llvm-$LLVM_VERSION/bin
WORKDIR /src
