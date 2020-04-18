#!/bin/sh -p

# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script will be called by the installer application to copy Google
# Chrome.app into the proper /Applications folder. This script may run as root.
#
# When running as root, this script will be invoked with the real user ID set
# to the user's ID, but the effective user ID set to 0 (root).  bash -p is
# used on the first line to prevent bash from setting the effective user ID to
# the real user ID (dropping root privileges).

# 'e': terminate if error arises
# 'u': raise an error if a variable isn't set
# 'o pipefail': set the return exit code to the last non-zero error code
set -euo pipefail

# Waits for the main app to pass the path to the app bundle inside the mounted
# disk image.
read -r SRC

DEST="${1}"
APPBUNDLENAME=$(basename "${SRC}")
FULL_DEST="${DEST}"/"${APPBUNDLENAME}"

# Starts the copy
# 'l': copy symlinks as symlinks
# 'r': recursive copy
# 'p': preserve permissions
# 't': preserve times
# 'q': quiet mode, so rynsc will only log to console if an error occurs
rsync -lrptq "${SRC}" "${DEST}"

# If this script is run as root, change ownership to root and set elevated
# permissions.
if [ "${EUID}" -eq 0 ] ; then
  chown -Rh root:admin "${FULL_DEST}"
  chmod -R a+rX,ug+w,o-w  "${FULL_DEST}"
fi

exit 0
