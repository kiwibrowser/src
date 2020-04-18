#!/bin/bash

# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script is used on GS purge servers to update and run the purge
# scripts repeatedly.  These GCE servers are expected to set custom
# metadata (via the GCE web UI) with the key "purge_target" and a
# value of "chromeos-release" or "chromeos-image-archive" based on
# which bucket a given server is supposed to clean.

LOG=~/purge.log
ATTR_URL="http://metadata.google.internal/computeMetadata/v1/instance/attributes/purge_target"
CHROMITE_BIN="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd "${CHROMITE_BIN}"

TARGET=`curl -H "Metadata-Flavor: Google" ${ATTR_URL}`
mv -f "${LOG}" "${LOG}.previous"
(date && \
 git pull && \
 ./purge_builds --debug "--${TARGET}" && \
 date) >> "${LOG}" 2>&1

# Wait a while.
echo "Sleeping for 24 hours."
sleep 24h
