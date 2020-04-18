#!/bin/bash -e

# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

ncpus=$(getconf _NPROCESSORS_ONLN)

export PATH="$(npm bin):${PATH}"

# In case tests fail without a JavaScript error take also a look at these files.
# NaCl module crash message is shown only in the JavaScript console within the
# browser, and PP_DCHECK and other NaCl errors will appear only here.
# Messages are appended to the logs and it's up to the tester to remove them.
export NACL_EXE_STDOUT=`pwd`/nacl.stdout
export NACL_EXE_STDERR=`pwd`/nacl.stderr

# user-data-dir-karma will cache extension files and might lead to test failures
# when changing branches, so we need to remove it before running the tests.
# Because Karma captures any changes to the JavaScript files as long as the
# tests run, this directory removal should not affect tests development speed.
# But the logs inside user-data-dir-karma can contain useful information in case
# the NaCl module crashes, so we need to keep it.
if [ -d user-data-dir-karma ]; then
  rm -r user-data-dir-karma
fi

cd ../unpacker/
# Test only with the Debug build.
make -j${ncpus} debug || { exit 1; }
cd ../unpacker-test
karma start
