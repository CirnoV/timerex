# See here for image contents: https://github.com/microsoft/vscode-dev-containers/tree/v0.183.0/containers/cpp/.devcontainer/base.Dockerfile

# [Choice] Debian / Ubuntu version: debian-10, debian-9, ubuntu-20.04, ubuntu-18.04
ARG VARIANT="ubuntu-20.04"
FROM mcr.microsoft.com/vscode/devcontainers/cpp:0-${VARIANT}

# [Optional] Uncomment this section to install additional packages.
# RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
#     && apt-get -y install --no-install-recommends <your-package-list-here>
WORKDIR /root

# common packages
RUN apt-get update && \
  apt-get upgrade -y && \
  apt-get install --no-install-recommends -y \
  software-properties-common \
  ca-certificates curl file \
  build-essential \
  autoconf automake autotools-dev libtool xutils-dev && \
  rm -rf /var/lib/apt/lists/*

ENV SSL_VERSION=1.0.2u

RUN curl https://www.openssl.org/source/openssl-$SSL_VERSION.tar.gz -O && \
  tar -xzf openssl-$SSL_VERSION.tar.gz && \
  cd openssl-$SSL_VERSION && ./config && make depend && make install && \
  cd .. && rm -rf openssl-$SSL_VERSION*

ENV OPENSSL_LIB_DIR=/usr/local/ssl/lib \
  OPENSSL_INCLUDE_DIR=/usr/local/ssl/include \
  OPENSSL_STATIC=1

# install toolchain
RUN curl https://sh.rustup.rs -sSf | \
  sh -s -- --default-toolchain stable -y

ENV PATH=/root/.cargo/bin:$PATH

# install pip
RUN add-apt-repository universe
RUN apt-get update && apt-get install python3-pip -y

# install multilib
RUN apt-get install gcc-multilib g++-multilib -y

# install mmsource / sourcemod / ambuild
RUN mkdir -p /workspaces
WORKDIR /workspaces
RUN git clone https://github.com/alliedmodders/metamod-source.git mmsource && \
  git clone --recursive https://github.com/alliedmodders/sourcemod.git && \
  git clone https://github.com/alliedmodders/ambuild.git && \
  pip3 install ./ambuild
