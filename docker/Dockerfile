FROM centos:7

ARG USERNAME=storm
ARG USER_UID=1000
ARG USER_GID=${USER_UID}
ARG WORK_DIR=storm-tape
ARG STORM_BUILD_TYPE=debug

COPY ./library-scripts/*.sh /library-scripts/
COPY ./storm-tape-*.tar.gz /

RUN chmod +x library-scripts/*.sh && \
    library-scripts/provide-deps.sh ${STORM_BUILD_TYPE} && \
    library-scripts/provide-user.sh ${USERNAME} ${USER_UID} ${USER_GID} && \
    rm -rf library-scripts && \
    tar -xvf storm-tape-*.tar.gz && \
    mv storm-tape-*/sbin/storm-tape* /usr/bin/ && \
    rm -rf storm-tape-* && \
    mkdir ${WORK_DIR} && \
    chown -R ${USERNAME}:${USERNAME} ${WORK_DIR}

USER ${USERNAME}
WORKDIR ${WORK_DIR}
