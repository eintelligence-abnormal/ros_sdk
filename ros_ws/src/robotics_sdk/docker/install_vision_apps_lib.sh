#!/bin/bash

#  Copyright (C) 2024 Texas Instruments Incorporated - http://www.ti.com/
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#    Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
#    Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the
#    distribution.
#
#    Neither the name of Texas Instruments Incorporated nor the names of
#    its contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set -e

echo "$(basename "$0"): Processing..."

# Example BASE_URL="https://software-dl.ti.com/jacinto7/esd/robotics-sdk/10_00_00/deps"
if [ -z "$BASE_URL" ]; then
    echo "Error: BASE_URL is not defined."
    exit 1
else
    echo "BASE_URL=$BASE_URL"
fi

if [ -z "$SOC" ]; then
    echo "Error: SOC is not defined."
    exit 1
fi

if [ -z "$TIVA_LIB_VER" ]; then
    echo "Error: TIVA_LIB_VER is not defined."
    exit 1
fi

# package name
DEB_PKG="libti-vision-apps-${SOC}_${TIVA_LIB_VER}-ubuntu22.04.deb"

# source folder for lib file
LIB_DIR="$HOME/ubuntu22-deps"
DOWNLOAD_LIBS=true
if [ -d "$LIB_DIR" ]; then
    DOWNLOAD_LIBS=false
    echo "$(basename "$0"): to use the library files under $LIB_DIR"
fi

# download the lib file
if [ "$DOWNLOAD_LIBS" = true ]; then
    mkdir -p "$LIB_DIR"
    wget -q --no-proxy "${BASE_URL}/${DEB_PKG}" -O "${LIB_DIR}/${DEB_PKG}"
    if [ $? -ne 0 ]; then
        echo "Error: Failed to download $DEB_PKG"
        exit 1
    fi
fi

# install vision-apps.deb
if [ -f "${LIB_DIR}/${DEB_PKG}" ]; then
    dpkg -i "${LIB_DIR}/${DEB_PKG}"
else
    echo "Error: ${DEB_PKG} does not exist."
    exit 1
fi

# clean up
if [ "$DOWNLOAD_LIBS" = true ]; then
    rm -rf "${LIB_DIR}"
fi

echo "$(basename "$0"): Completed!"
