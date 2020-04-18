#!/bin/sh

# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# ChromeOS version information
#
# This file is usually sourced by other build scripts, but can be run
# directly to see what it would do.
#
# Version numbering scheme is much like Chrome's, with the addition of
# double-incrementing branch number so trunk is always odd.

#############################################################################
# SET VERSION NUMBERS
#############################################################################
# Major/minor versions.
# Primarily for product marketing.
export CHROMEOS_VERSION_MAJOR=0
export CHROMEOS_VERSION_MINOR=13

# Branch number.
# Increment by 1 in a new release branch.
# Increment by 2 in trunk after making a release branch.
# Does not reset on a major/minor change (always increases).
# (Trunk is always odd; branches are always even).
export CHROMEOS_VERSION_BRANCH=507

# Patch number.
# Increment by 1 each release on a branch.
# Reset to 0 when increasing branch number.
export CHROMEOS_VERSION_PATCH=87

# Official builds must set CHROMEOS_OFFICIAL=1.
if [ ${CHROMEOS_OFFICIAL:-0} -ne 1 ] && [ "${USER}" != "chrome-bot" ]; then
  # For developer builds, overwrite CHROMEOS_VERSION_PATCH with a date string
  # for use by auto-updater.
  export CHROMEOS_VERSION_PATCH=$(date +%Y_%m_%d_%H%M)
fi

# Version string. Not indentied to appease bash.
export CHROMEOS_VERSION_STRING=\
"${CHROMEOS_VERSION_MAJOR}.${CHROMEOS_VERSION_MINOR}"\
".${CHROMEOS_VERSION_BRANCH}.${CHROMEOS_VERSION_PATCH}"

# Set CHROME values (Used for releases) to pass to chromeos-chrome-bin ebuild
# URL to chrome archive
export CHROME_BASE=
# export CHROME_VERSION from incoming value or NULL and let ebuild default
export CHROME_VERSION="$CHROME_VERSION"

# Print (and remember) version info.
echo "ChromeOS version information:"
env | egrep '^CHROMEOS_VERSION|CHROME_' | sed 's/^/    /'
