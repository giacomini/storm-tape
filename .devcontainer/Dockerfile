FROM --platform=amd64 centos:7

ARG DEVTOOLSET_VERSION=11
ARG CMAKE_VERSION=3.25.2
ARG PLATFORM=linux
ARG ARCH=x86_64

ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=${USER_UID}

COPY base-scripts/*.sh /base-scripts/
COPY library-scripts/*.sh /library-scripts/
COPY library-scripts/scl_enable /opt/app-root/etc/

ENV VCPKG_ROOT=/vcpkg
ENV VCPKG_DOWNLOADS=/vcpkg_downloads
ENV APP_ROOT=/opt/app-root
ENV BASH_ENV=${APP_ROOT}/etc/scl_enable \
    ENV=${APP_ROOT}/etc/scl_enable \
    PROMPT_COMMAND=". ${APP_ROOT}/etc/scl_enable"

RUN sed -i "s/#DEVTOOLSET_VERSION#/${DEVTOOLSET_VERSION}/g" /opt/app-root/etc/scl_enable && \
    chmod +x library-scripts/*.sh base-scripts/*.sh && \
    library-scripts/provide-deps.sh && \
    library-scripts/provide-cmake.sh && \
    library-scripts/provide-user.sh ${USERNAME} ${USER_UID} ${USER_GID} && \
    base-scripts/install-vcpkg.sh ${USERNAME} && \
    rm -rf library-scripts base-scripts

USER ${USERNAME}
