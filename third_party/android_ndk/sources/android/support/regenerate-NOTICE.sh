#!/bin/bash

# Change into the libandroid_support directory.
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR

# Pull the notices from the files in libandroid_support itself (via `.`),
# plus all the bionic files we pull in.
sed '/$(BIONIC_PATH).*\.c/ { s| *$(BIONIC_PATH)|../../../../bionic/| ; s| *\\$|| ; p } ; d' Android.mk | \
    xargs ../../../../bionic/libc/tools/generate-NOTICE.py . > NOTICE

# Show the caller what we've done.
git diff --exit-code HEAD ./NOTICE
exit $?
