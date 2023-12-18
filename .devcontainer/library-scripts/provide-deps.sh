#!/usr/bin/env bash

# Copyright 2023 Istituto Nazionale di Fisica Nucleare
# SPDX-License-Identifier: EUPL-1.2

set -ex

# Add repo for devtools, ninja and git 2.x
yum install -y \
    centos-release-scl \
    epel-release \
    https://packages.endpointdev.com/rhel/7/os/${ARCH}/endpoint-repo.${ARCH}.rpm

yum update -y

yum install -y --setopt=tsflags=nodocs \
    sudo \
    git \
    wget \
    curl \
    zip \
    unzip \
    tar \
    ninja-build \
    rpm-build \
    devtoolset-"${DEVTOOLSET_VERSION}" \
    devtoolset-"${DEVTOOLSET_VERSION}"-libubsan-devel \
    devtoolset-"${DEVTOOLSET_VERSION}"-libasan-devel

yum clean all
rm -rf /var/cache/yum