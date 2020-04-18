# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

function cipd_bin_setup {
    local MYPATH=$(dirname "${BASH_SOURCE[0]}")
    local ENSURE="$MYPATH/cipd_manifest.txt"
    local ROOT="$MYPATH/.cipd_bin"

    UNAME=`uname -s | tr '[:upper:]' '[:lower:]'`
    case $UNAME in
      cygwin*)
        ENSURE="$(cygpath -w $ENSURE)"
        ROOT="$(cygpath -w $ROOT)"
        ;;
    esac

    "$MYPATH/cipd" ensure \
        -log-level warning \
        -ensure-file "$ENSURE" \
        -root "$ROOT"
}
