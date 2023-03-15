#!/usr/bin/env bash

# Copyright 2023 Istituto Nazionale di Fisica Nucleare
# SPDX-License-Identifier: EUPL-1.2

# Adapted from https://github.com/devcontainers/images/blob/main/src/cpp/.devcontainer/base-scripts/install-vcpkg.sh


set -e

USERNAME=${1:-"vscode"}

# Add to bashrc/zshrc files for all users.
updaterc()  {
    echo "Updating /etc/bash.bashrc and /etc/zsh/zshrc..."
    if [[ "$(cat /etc/bash.bashrc)" != *"$1"* ]]; then
        echo -e "$1" >> /etc/bash.bashrc
    fi
    if [ -f "/etc/zsh/zshrc" ] && [[ "$(cat /etc/zsh/zshrc)" != *"$1"* ]]; then
        echo -e "$1" >> /etc/zsh/zshrc
    fi
}

# Setup group and add user
umask 0002
if ! cat /etc/group | grep -e "^vcpkg:" > /dev/null 2>&1; then
    groupadd -r "vcpkg"
fi
usermod -a -G "vcpkg" "${USERNAME}"

# Start Installation
# Clone repository with ports and installer
mkdir -p "${VCPKG_ROOT}"
mkdir -p "${VCPKG_DOWNLOADS}"
git clone \
    -c core.eol=lf \
    -c core.autocrlf=false \
    -c fsck.zeroPaddedFilemode=ignore \
    -c fetch.fsck.zeroPaddedFilemode=ignore \
    -c receive.fsck.zeroPaddedFilemode=ignore \
    https://github.com/microsoft/vcpkg "${VCPKG_ROOT}"

## Run installer to get latest stable vcpkg binary
"${VCPKG_ROOT}"/bootstrap-vcpkg.sh

# Add vcpkg to PATH
updaterc "$(cat << EOF
export VCPKG_ROOT="${VCPKG_ROOT}"
if [[ "\${PATH}" != *"\${VCPKG_ROOT}"* ]]; then export PATH="\${PATH}:\${VCPKG_ROOT}"; fi
EOF
)"

# Give read/write permissions to the user group.
chown -R ":vcpkg" "${VCPKG_ROOT}" "${VCPKG_DOWNLOADS}"
chmod g+r+w+s "${VCPKG_ROOT}" "${VCPKG_DOWNLOADS}"
chmod -R g+r+w "${VCPKG_ROOT}" "${VCPKG_DOWNLOADS}"