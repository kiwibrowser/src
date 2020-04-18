#!/bin/bash
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

##################################################################
#  Description: tool to examine an x86_64 linux system to look for
#  missing packages needed to develop for Native Client and help the
#  user to install any that are missing.  This makes many assumptions
#  about the linux distribution (Ubuntu) and version (Trusty Tahr) and
#  might not work for other distributions/versions.
##################################################################

PATH=/usr/local/sbin:/usr/sbin:/sbin:/usr/local/bin:/usr/bin:/bin

umask=077
d=${TMPDIR:-/tmp}/nacl64.$$
if ! mkdir "$d" > /dev/null 2>&1
then
  cat >&2 << EOF
Could not create safe temporary directory "$d".

ABORTING.
EOF
  exit 1
fi
f="$d/x.c"
fout="$d/x"
trap 'rm -fr "$d"; exit' 0 1 2 3 15

function isRunningAsRoot() {
  whoami | grep -q 'root'
}


function ensure_installed {
  if ! [ -e "$1" ]
  then
    cat >&2 << EOF
... you do not have $2.  Installing...
EOF
    if apt-get -y install "$2" > /dev/null 2>&1
    then
      echo "... done" >&2
    else
      cat >&2 <<EOF
... failed to install $2.

ABORTING
EOF
      exit 1
    fi
  fi
}

if ! isRunningAsRoot
then
  cat >&2 << \EOF
Not running as root, so cannot install libraries/links.
Note: you probably will need to copy this script to the local file system
(and off of NFS) in order to run this script as root.

ABORTING.
EOF
  exit 1
fi

ensure_installed '/usr/lib/git-core/git-svn' 'git-svn'

if [ $(uname -m) != "x86_64" ]
then
  cat << \EOF
You do not appear to be using an x86_64 system.  This rest of this script
is not required.
EOF
  exit 0
fi

# libtinfo5 is needed by gdb
ensure_installed '/lib/i386-linux-gnu/libtinfo.so.5' 'libtinfo5:i386'
# glib is needed by qemu
ensure_installed '/lib/i386-linux-gnu/libglib-2.0.so.0' 'libglib2.0-0:i386'
# 32-bit libc headers and libraries
ensure_installed '/usr/include/i386-linux-gnu/asm/errno.h' 'linux-libc-dev:i386'
ensure_installed '/usr/share/doc/g++-4.8-multilib' 'g++-4.8-multilib'
