# See here for image contents: https://github.com/microsoft/vscode-dev-containers/tree/v0.231.6/containers/cpp/.devcontainer/base.Dockerfile

# [Choice] Debian / Ubuntu version (use Debian 11, Ubuntu 18.04/21.04 on local arm64/Apple Silicon): debian-11, debian-10, ubuntu-21.04, ubuntu-20.04, ubuntu-18.04
ARG VARIANT="bullseye"
FROM mcr.microsoft.com/vscode/devcontainers/base:0-${VARIANT}

# [Optional] Install CMake version different from what base image has already installed. 
# CMake reinstall choices: none, 3.21.5, 3.22.2, or versions from https://cmake.org/download/
ARG REINSTALL_CMAKE_VERSION_FROM_SOURCE="3.24.0"

# Optionally install the cmake for vcpkg
COPY docker/base-scripts/reinstall-cmake.sh /tmp/
RUN if [ "${REINSTALL_CMAKE_VERSION_FROM_SOURCE}" != "none" ]; then \
        apt-get update \
        && chmod +x /tmp/reinstall-cmake.sh && /tmp/reinstall-cmake.sh ${REINSTALL_CMAKE_VERSION_FROM_SOURCE}; \
    fi \
    && rm -f /tmp/reinstall-cmake.sh

# Install needed packages. Use a separate RUN statement to add your own dependencies.
COPY docker/library-scripts/meta.env /usr/local/etc/vscode-dev-containers

# Setup ENV vars for vcpkg
ENV VCPKG_ROOT=/usr/local/vcpkg \
    VCPKG_DOWNLOADS=/usr/local/vcpkg-downloads
ENV PATH="${PATH}:${VCPKG_ROOT}"

# Install vcpkg itself: https://github.com/microsoft/vcpkg/blob/master/README.md#quick-start-unix
COPY docker/base-scripts/install-vcpkg.sh /tmp/
RUN /tmp/install-vcpkg.sh ${USERNAME} \
    && rm -f /tmp/install-vcpkg.sh

# Compile storm-tape-poc
RUN mkdir -p /storm-tape-poc/build/
COPY *.cpp *.hpp *.json CMakeLists.txt /storm-tape-poc/
COPY docker/compile_src.sh /tmp/
COPY docker/test_script.sh /test/
RUN chmod +x /tmp/compile_src.sh && chmod +x /test/test_script.sh && \
/tmp/compile_src.sh && rm -f /tmp/compile_src.sh