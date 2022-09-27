# StoRM Tape
Diotalevi's branch #ciao

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

### Stage a request
To stage a request, using the dummy stage request JSON contained in this repo:

```shell
$ curl -i -d @stage_request.json http://localhost:8080/api/v1/stage
HTTP/1.1 201 Created
Location: https://localhost:8080/api/v1/stage/6aa34070-d82c-49c5-b4c1-f48046625d2f
Content-Type: application/json
Content-Length: 52
Server: Crow/1.0
Date: Mon, 22 Aug 2022 15:00:21 GMT
Connection: Keep-Alive

{"requestId":"6aa34070-d82c-49c5-b4c1-f48046625d2f"}
```

### Progress tracking
To see the progress tracking of one request: 

```shell
$ curl -i http://localhost:8080/api/v1/stage/6aa34070-d82c-49c5-b4c1-f48046625d2f
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 188
Server: Crow/1.0
Date: Mon, 22 Aug 2022 15:25:52 GMT
Connection: Keep-Alive

{"id":"6aa34070-d82c-49c5-b4c1-f48046625d2f","created_at":1661181803,"started_at":1661181803,"files":[{"path":"/data/pluto","state":"SUBMITTED"},{"path":"/tmp/pippo","state":"SUBMITTED"}]}
```

### Cancel a subset of files
To cancel a subset of files, listed in JSON format (a cancel request dummy JSON is provided in this repo), of a given stage request:

```shell
$ curl -i -d @cancel_request.json http://localhost:8080/api/v1/stage/6aa34070-d82c-49c5-b4c1-f48046625d2f/cancel
HTTP/1.1 200 OK
Content-Length: 0
Server: Crow/1.0
Date: Mon, 22 Aug 2022 15:26:45 GMT
Connection: Keep-Alive
```

Requesting again a progress track of the stage request, now gives the following output:

```shell
$ curl -i http://localhost:8080/api/v1/stage/6aa34070-d82c-49c5-b4c1-f48046625d2f
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 188
Server: Crow/1.0
Date: Mon, 22 Aug 2022 15:27:33 GMT
Connection: Keep-Alive

{"id":"6aa34070-d82c-49c5-b4c1-f48046625d2f","created_at":1661181803,"started_at":1661181803,"files":[{"path":"/data/pluto","state":"CANCELLED"},{"path":"/tmp/pippo","state":"SUBMITTED"}]}
```

### Archive information
To request information about the progress of writing files to tape, given a JSON with the list of requested files (a dummy archive info JSON is provided in this repo):

```shell
$ curl -i -d @archive_info.json http://localhost:8080/api/v1/archiveinfo
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 135
Server: Crow/1.0
Date: Mon, 22 Aug 2022 15:28:17 GMT
Connection: Keep-Alive

[{"error":"USER ERROR: file does not exist or is not accessible to you","path":"/local/minni"},{"locality":"TAPE","path":"/tmp/pippo"}]
```

### Delete a stage request
To delete a stage request, given its unique ID:

```shell
$ curl -X "DELETE" -i http://localhost:8080/api/v1/stage/6aa34070-d82c-49c5-b4c1-f48046625d2f
HTTP/1.1 200 OK
Content-Length: 0
Server: Crow/1.0
Date: Mon, 22 Aug 2022 15:29:21 GMT
Connection: Keep-Alive
```

And again, requesting the archive information about the same files, now gives the following output:

```shell
$ curl -i -d @archive_info.json http://localhost:8080/api/v1/archiveinfo
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 187
Server: Crow/1.0
Date: Mon, 22 Aug 2022 15:30:49 GMT
Connection: Keep-Alive

[{"error":"USER ERROR: file does not exist or is not accessible to you","path":"/local/minni"},{"error":"USER ERROR: file does not exist or is not accessible to you","path":"/tmp/pippo"}]
```