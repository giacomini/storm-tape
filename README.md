# StoRM Tape REST API

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
git clone https://baltig.infn.it/cnafsd/storm-tape.git
cd storm-tape
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

To build with presets:

```shell
cmake --preset <preset>
cmake --build build/<preset>
```

For example, to build the `debug` configuration:
```shell
cmake --preset debug
cmake --build build/debug
```

To list all available presets:

```shell
cmake --list-presets

Available configure presets:

  "clang-tidy"
  "debug"
  "release"
  "test"
```

To perform linting (`clang-format`):

```shell
cmake -S . -B build [options]
cmake --build build --target format-check 
cmake --build build --target format-fix
```

## How to run

To run the server

```shell
$ build/storm-tape
(2022-04-28 15:47:59) [INFO    ] Crow/1.0 server is running at http://0.0.0.0:8080 using 2 threads
(2022-04-28 15:47:59) [INFO    ] Call `app.loglevel(crow::LogLevel::Warning)` to hide Info level logs.
...
```

## Install via Docker
As an alternative, it is possible to run the REST API server via Docker, using the following command:
```
docker run -it -p 8080:8080 -v <path/to/dir>:/storm-tape baltig.infn.it:4567/cnafsd/storm-tape
```
NOTE:
Before running the image, make sure to be correctly logged in on the Baltig Docker registry,
via the docker [login](https://docs.docker.com/engine/reference/commandline/login/) command,
using INFN-AAI credentials.

### Stage a request
To stage a request, use the dummy stage request JSON contained in this repo:

```shell
$ curl -i -d @example/stage_request.json http://localhost:8080/api/v1/stage
HTTP/1.1 201 Created
Location: https://localhost:8080/api/v1/stage/318640a8-424e-4071-adb8-abefad1bdbb3
Content-Type: application/json
Content-Length: 52
Server: Crow/1.0
Date: Thu, 10 Nov 2022 17:12:47 GMT
Connection: Keep-Alive

{"requestId":"318640a8-424e-4071-adb8-abefad1bdbb3"}
```
NOTE: Using the provided Docker container, the dummy files listed in the stage_request JSON have been created. 
In alternative, make sure to provide existing filename(s).
If a file is not present on the filesystem, or an invalid file is prompted, a stage request will still be created,
but the files will be subsequently listed with "FAILED" state.

### Progress tracking
To see the progress tracking of one request: 

```shell
$ curl -i http://localhost:8080/api/v1/stage/318640a8-424e-4071-adb8-abefad1bdbb3
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 200
Server: Crow/1.0
Date: Thu, 10 Nov 2022 17:14:15 GMT
Connection: Keep-Alive

{"id":"318640a8-424e-4071-adb8-abefad1bdbb3","created_at":1668100367,"started_at":1668100367,"files":[{"path":"/tmp/example.txt","state":"SUBMITTED"},{"path":"/tmp/example2.txt","state":"SUBMITTED"}]}
```

### Cancel a subset of files
To cancel a subset of files, listed in JSON format (a cancel request dummy JSON is provided in this repo), of a given stage request:

```shell
$ curl -i -d @example/cancel_request.json http://localhost:8080/api/v1/stage/6aa34070-d82c-49c5-b4c1-f48046625d2f/cancel
HTTP/1.1 200 OK
Content-Length: 0
Server: Crow/1.0
Date: Thu, 10 Nov 2022 17:22:37 GMT
Connection: Keep-Alive
```

Requesting again a progress track of the stage request, now gives the following output:

```shell
$ curl -i http://localhost:8080/api/v1/stage/318640a8-424e-4071-adb8-abefad1bdbb3
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 200
Server: Crow/1.0
Date: Thu, 10 Nov 2022 17:22:57 GMT
Connection: Keep-Alive

{"id":"318640a8-424e-4071-adb8-abefad1bdbb3","created_at":1668100367,"started_at":1668100367,"files":[{"path":"/tmp/example.txt","state":"CANCELLED"},{"path":"/tmp/example2.txt","state":"SUBMITTED"}]}
```

### Archive information
To request information about the progress of writing files to tape, given a JSON with the list of requested files (a dummy archive info JSON is provided in this repo):

```shell
$ curl -i -d @example/archive_info.json http://localhost:8080/api/v1/archiveinfo
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 147
Server: Crow/1.0
Date: Thu, 10 Nov 2022 17:24:10 GMT
Connection: Keep-Alive

[{"error":"USER ERROR: file does not exist or is not accessible to you","path":"/tmp/example3.txt"},{"locality":"TAPE","path":"/tmp/example2.txt"}]
```

### Delete a stage request
To delete a stage request, given its unique ID:

```shell
$ curl -X "DELETE" -i http://localhost:8080/api/v1/stage/318640a8-424e-4071-adb8-abefad1bdbb3
HTTP/1.1 200 OK
Content-Length: 0
Server: Crow/1.0
Date: Thu, 10 Nov 2022 17:24:56 GMT
Connection: Keep-Alive
```

And again, requesting the archive information about the same files, now gives the following output:

```shell
$ curl -i -d @example/archive_info.json http://localhost:8080/api/v1/archiveinfo
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 199
Server: Crow/1.0
Date: Thu, 10 Nov 2022 17:25:23 GMT
Connection: Keep-Alive

[{"error":"USER ERROR: file does not exist or is not accessible to you","path":"/tmp/example2.txt"},{"error":"USER ERROR: file does not exist or is not accessible to you","path":"/tmp/example3.txt"}]
```

As a cross-check, progress tracking now the previous stage ID (now deleted) will result in a 404 response:

```shell
$ curl -i http://localhost:8080/api/v1/stage/318640a8-424e-4071-adb8-abefad1bdbb3
HTTP/1.1 404 Not Found
Content-Length: 15
Server: Crow/1.0
Date: Thu, 10 Nov 2022 17:33:39 GMT
Connection: Keep-Alive

404 Not Found
```
