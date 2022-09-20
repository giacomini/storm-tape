#!/usr/bin/env bash

#Configuring build folder
/usr/local/bin/cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE \
-DCMAKE_BUILD_TYPE:STRING=Debug -S /workspaces/storm-tape-poc \
-B /workspaces/storm-tape-poc/build -G Ninja

#Building
Executing command: /usr/local/bin/cmake --build /workspaces/storm-tape-poc/build \
--config Debug --target all