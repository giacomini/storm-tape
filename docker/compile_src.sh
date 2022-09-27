#!/usr/bin/env bash

#Configuring build folder
#/usr/local/bin/cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE \
#-DCMAKE_BUILD_TYPE:STRING=Debug -S /storm-tape-poc \
#-B /storm-tape-poc/build -G Ninja

#Building
#/usr/local/bin/cmake --build /storm-tape-poc/build \
#--config Debug --target all