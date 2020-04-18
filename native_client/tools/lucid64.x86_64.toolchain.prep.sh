#!/bin/bash
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

PATH=/usr/local/sbin:/usr/sbin:/sbin:/usr/local/bin:/usr/bin:/bin
umask 077

set -x
set -e
set -u

declare -A libraries=(
  [libppl0.10-dev]=p/ppl
  [libcloog-ppl-dev]=c/cloog-ppl
  [libgmp3-dev]=g/gmp
  [libgmp-dev]=g/gmp
  [libmpfr-dev]=m/mpfr4
  [libmpc-dev]=m/mpclib
)

declare -a mirrors=(
  http://mirrors.kernel.org/ubuntu/pool/main
  http://mirrors.kernel.org/debian/pool/main
  http://mirrors.kernel.org/ubuntu/pool/universe
)

working_directory="${TMPDIR:-/tmp}/nacl64.$$"

mkdir "$working_directory"

cd "$working_directory"

need_libstdcxx46=0

if dpkg -s libppl0.11-dev ; then
  libraries[libppl0.11-dev]=${libraries[libppl0.10-dev]}
  libraries[libpwl-dev]=p/ppl
  unset libraries[libppl0.10-dev]
  need_libstdcxx46=1
fi

if ((EUID==0)); then
  install=1
  apt-get install "${!libraries[@]}" ||
    unset libraries[libgmp-dev] && apt-get install "${!libraries[@]}"
else
  if ! dpkg -s libgmp-dev ; then
    unset libraries[libgmp-dev]
  fi
  echo "If libraries are not all installed please do:"
  echo "  sudo apt-get install ${!libraries[@]}"
  install=0
fi
if dpkg -s libgmp-dev ; then
  unset libraries[libgmp3-dev]
fi
for library in "${!libraries[@]}"; do
  dpkg -s "$library" | grep Version |
  {
    read skip version
    if [[ "$version" = *:* ]] ; then
      version="${version/*:/}"
      echo $version
    fi
    found=0
    for mirror in "${mirrors[@]}"; do
      if wget "$mirror/${libraries[$library]}/${library}_${version}_i386.deb"
      then
        found=1
        break
      fi
    done
    if ((!found)) && [[ "$library" = "libmpfr-dev" ]]; then
      for mirror in "${mirrors[@]}"; do
        if wget "$mirror/m/mpfr/${library}_${version}_i386.deb"; then
          found=1
          break
        fi
      done
    fi
    if ((!found)); then
      exit 1
    fi
    ar x "${library}_${version}_i386.deb" data.tar.gz
    tar xSvpf data.tar.gz --wildcards './usr/lib/*.*a'
    if [[ "$library" = libgmp*-dev ]]; then
      tar xSvpf data.tar.gz './usr/include/gmp-i386.h'
    fi
    rm data.tar.gz
  }
done
perl -pi -e s'|/usr/lib|/usr/lib32|' usr/lib/*.la
mv usr/lib/libgmpxx.a usr/lib/libgmpxx-orig.a
if ((need_libstdcxx46)); then
  wget "${mirrors[1]}"/g/gcc-4.6/libstdc++6-4.6-dev_4.6.0-2_i386.deb
  ar x libstdc++6-4.6-dev_4.6.0-2_i386.deb data.tar.gz
  tar xSvpf data.tar.gz ./usr/lib/gcc/i486-linux-gnu/4.6/libstdc++.a
  mv usr/lib/gcc/i486-linux-gnu/4.6/libstdc++.a usr/lib/libstdc++46.a
  rm -rf usr/lib/gcc
  rm data.tar.gz
  cat >usr/lib/libgmpxx.a <<END
OUTPUT_FORMAT(elf32-i386)
GROUP ( /usr/lib32/libgmpxx-orig.a
        AS_NEEDED ( /usr/lib32/libstdc++46.a
                    /usr/lib32/libpwl.a
                    /usr/lib32/libm.so ) )
END
else
  cat >usr/lib/libgmpxx.a <<END
OUTPUT_FORMAT(elf32-i386)
GROUP ( /usr/lib32/libgmpxx-orig.a
        AS_NEEDED ( /usr/lib32/libstdc++.so.6 /usr/lib32/libm.so ) )
END
fi
chmod a+r usr/lib/libgmpxx.a
if ((install)); then
  mv -f usr/lib/*.*a /usr/lib32
  mv -f usr/include/*.h /usr/include
  rm -rf "$PWD"
else
  echo "Move files from $PWD/usr/lib to /usr/lib32"
  echo "Move files from $PWD/usr/include to /usr/include"
fi
