#!/bin/bash
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

mirror_base_address="https://storage.googleapis.com/nativeclient-mirror/toolchain"

if (($#<1 || $#>2)); then
  cat <<END
Usage:
  $0 <revision> [SRC-directory name]

This script downloads pristine sources and applies patches found on
https://gsdview.appspot.com/nativeclient-archive2/x86_toolchain/r<revison>/
By default sources are put in SRC directory (which should be empty).
END
  exit 1
fi

dir="$(realpath "$(dirname "$0")")"
if (($#==2)); then
  if [[ "${2:0:1}" = "/" ]]; then
    target="$2"
  else
    target="$dir/$2"
  fi
else
  target="$dir/SRC"
fi
# If target exist but only contains files starting with dot we can use it.
if [[ -e "$target" ]] &&
   (($(find "$target" -maxdepth 1 ! -name ".*" | wc -l)>1)); then
  cat <<END
$target already exist and it's not empty. Not overwriting.
END
  exit 2
fi

tar=$(which gnutar || which tar)
uncompress=$(which xz)
suffix=xz
if [[ "$uncompress" = "" ]]; then
  uncompress=bzip2
  suffix=bz2
fi

if [[ "$(which wget)" = "" ]]; then
  wget() {
    if (($#==1)); then
      curl "$1" -o "$(basename "$1")"
    else
      curl "$1"
    fi
  }
fi

set -e
set -u
set -x

mkdir -p "$target"
( flock -x 3
  # We have a lock. Now check: maybe someone downloaded files already?
  if (($(find "$target" -maxdepth 1 ! -name ".*" | wc -l)<=1)); then
    wget \
      "https://gsdview.appspot.com/nativeclient-archive2/x86_toolchain/r$1/" -O- |
    grep patch.$suffix |
    while IFS='"' read -r -d $'\n' prefix patchname suffix; do
      basename="$(basename "$patchname")"
      version="${basename#*-}"
      version="${version%-*}"
      cd "$target"
      case "$basename" in
        *binutils*)
          wget "$mirror_base_address/binutils/binutils-$version.tar.bz2"
          "$tar" xjSvpf "binutils-$version.tar.bz2"
          mv "binutils-$version" "binutils"
          cd "binutils"
          ;;
        *gcc*)
          wget "$mirror_base_address/gcc/gcc-$version/gcc-core-$version.tar.bz2"
          "$tar" xjSvpf "gcc-core-$version.tar.bz2"
          wget "$mirror_base_address/gcc/gcc-$version/gcc-fortran-$version.tar.bz2"
          "$tar" xjSvpf "gcc-fortran-$version.tar.bz2"
          wget "$mirror_base_address/gcc/gcc-$version/gcc-g++-$version.tar.bz2"
          "$tar" xjSvpf "gcc-g++-$version.tar.bz2"
          wget "$mirror_base_address/gcc/gcc-$version/gcc-objc-$version.tar.bz2"
          "$tar" xjSvpf "gcc-objc-$version.tar.bz2"
          wget "$mirror_base_address/gcc/gcc-$version/gcc-testsuite-$version.tar.bz2"
          "$tar" xjSvpf "gcc-testsuite-$version.tar.bz2"
          mv "gcc-$version" "gcc"
          cd "gcc"
          ;;
        *glibc*)
          wget "$mirror_base_address/glibc/glibc-$version.tar.bz2"
          "$tar" xjSvpf "glibc-$version.tar.bz2"
          wget "$mirror_base_address/glibc/glibc-libidn-$version.tar.bz2"
          "$tar" xjSvpf "glibc-libidn-$version.tar.bz2"
          mv "glibc-$version" "glibc"
          mv "glibc-libidn-$version" "glibc/libidn"
          cd "glibc"
          ;;
        *newlib*)
          wget "$mirror_base_address/newlib/newlib-$version.tar.gz"
          "$tar" xzSvpf "newlib-$version.tar.gz"
          mv "newlib-$version" "newlib"
          cd "newlib"
          ;;
        *)
          echo "Unknown package: $basename"
          exit 3
          ;;
      esac
      wget "https://storage.googleapis.com$patchname" -O- |
      "$uncompress" -d |
      patch -p1
    done
    cd "$target"
    rm *.tar.*
  fi
) 3> "$target/.src-lock"
