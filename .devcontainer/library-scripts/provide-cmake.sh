#!/usr/bin/env bash

# Copyright 2023 Istituto Nazionale di Fisica Nucleare
# SPDX-License-Identifier: EUPL-1.2
set -ex

wget "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-${PLATFORM}-${ARCH}.sh" && \
    chmod +x "cmake-${CMAKE_VERSION}-${PLATFORM}-${ARCH}".sh && \
    ./"cmake-${CMAKE_VERSION}-${PLATFORM}-${ARCH}.sh" --prefix=/usr/local --skip-license && \
    rm -rf cmake-* && \
    cmake --version