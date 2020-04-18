#!/bin/bash
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Attaches GDB to x86-64 Chromium process running a NaCl module.
#
# Assumes:
#   * there is only one NaCl module running on the whole system
#   * the NaCl module is linked with the x86 glibc toolchain
#
# To use:
#   1. put your nacl64-gdb in PATH, see more details on nacl64-gdb at [1]
#   2. cd to where the x86-64 runnable-ld.so is to let nacl64-gdb find it
#   3. make a link named main.nexe that points to the x86_64-nacl version of
#      'main.nexe' as the .nmf manifest suggests (see [2])
#   4. change NACL_IRT below to point at nacl_irt_x86_64.nexe near chrome
#   5. for the best experience make all DSOs (libblah.so.* files) be found in
#      the current directory by:
#      * making links analogous to the one for the 'main.nexe' or
#      * changing your .nmf manifest
#
# [1] http://www.chromium.org/nativeclient/how-tos/debuggingtips/debugging-x86-64-native-client-modules-with-nacl64-gdb
# [2] http://www.chromium.org/nativeclient/design-documents/nacl-manifest-file-format-for-glibc
#
# Messy, heh? Hope this gets sorted out soon.

NACL_IRT=${NACL_IRT:-/absolute/path/to/nacl_irt_x86_64.nexe}

# Find a nacl_helper_bootstrap process that is a child of a
# nacl_helper_bootstrap process. It runs the NaCl module.
pid=$(
  ps -eo pid,cmd |
  grep -e '[/]nacl_helper_bootstrap' |
  while read pid rest ; do
    cat /proc/"$pid"/stat | (
      read pid name status ppid rest
      if [[ $(cat /proc/$ppid/stat) = *nacl_helper_boo* ]] ; then
        echo $pid
      fi
    )
  done
)

echo ===
echo attaching to pid=$pid
echo ===

readonly tmpfile=/tmp/t.gdb.$$

cat >$tmpfile<<END
nacl-irt $NACL_IRT
nacl-file ./runnable-ld.so
attach $pid
END

nacl64-gdb -x $tmpfile
rm $tmpfile
