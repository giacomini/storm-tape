#!/bin/bash

STORM_BUILD_TYPE=${1}

if [[ $STORM_BUILD_TYPE == "debug" ]] 
then 
    echo "Installing depencencies for debug"
    yum install -y centos-release-scl
    yum install -y \
        libubsan1 \
        libasan6
else
    echo "Skipping debug dependencies"
fi