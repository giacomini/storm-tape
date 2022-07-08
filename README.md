# StoRM Tape
Diotalevi's branch

## Introduction

This repository contains a proof-of-concept implementation in C++ of the Tape REST API defined in
https://docs.google.com/document/d/1Zx_H5dRkQRfju3xIYZ2WgjKoOvmLtsafP2pKGpHqcfY/edit?usp=sharing.

## How to build

The dependencies (Crow, Soci, Boost, ...) are managed with vcpkg, which needs to be installed first.

```shell
cd somewhere
git clone https://github.com/Microsoft/vcpkg.git
vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT=$(pwd)/vcpkg
```

The dependencies are specified in a manifest file.

To build:

```shell
git clone https://baltig.infn.it/giaco/storm-tape-poc.git
cd storm-tape-poc
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

## How to run

To run the server

```shell
$ build/storm-tape
(2022-04-28 15:47:59) [INFO    ] Crow/1.0 server is running at http://0.0.0.0:8080 using 2 threads
(2022-04-28 15:47:59) [INFO    ] Call `app.loglevel(crow::LogLevel::Warning)` to hide Info level logs.
...
```

The repo contains a simple stage request and a simple cancel request.

```shell
$ curl -i -d @stage_request.json http://localhost:8080/api/v1/stage
HTTP/1.1 201 Created
Location: http://localhost:8080/api/v1/stage/c5a71127-9745-48e6-bd94-245b48309c05
Content-Type: application/json
Content-Length: 52
Server: Crow/1.0
Date: Thu, 28 Apr 2022 15:49:35 GMT
Connection: Keep-Alive

{"requestId":"c5a71127-9745-48e6-bd94-245b48309c05"}
```

```shell
$ curl -i -d @cancel_request.json http://localhost:8080/api/v1/stage/c5a71127-9745-48e6-bd94-245b48309c05/cancel
HTTP/1.1 200 OK
Content-Length: 0
Server: Crow/1.0
Date: Thu, 28 Apr 2022 15:52:16 GMT
Connection: Keep-Alive

```

```shell
$curl -i http://localhost:8080/api/v1/stage/617f5a60-329a-435a-a2be-94e590daa41f
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 188
Server: Crow/1.0
Date: Thu, 28 Apr 2022 15:53:54 GMT
Connection: Keep-Alive

{"id":"617f5a60-329a-435a-a2be-94e590daa41f","created_at":1651161090,"started_at":1651161090,"files":[{"path":"/data/pluto","state":"SUBMITTED"},{"path":"/tmp/pippo","state":"SUBMITTED"}]}
```
