#!/bin/sh
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script to build binutils for both i386 and AMD64 Linux architectures.
# Must be run on an AMD64 supporting machine which has debootstrap and sudo
# installed.
# Uses Ubuntu Xenial chroots as build environment.

set -e
set -u

if [ x"$(whoami)" = x"root" ]; then
  echo "Script must not be run as root."
  exit 1
fi
sudo -v

OUTPUTDIR="${1:-$PWD/output-$(date +%Y%m%d-%H%M%S)}"
if [ ! -d "$OUTPUTDIR" ]; then
  mkdir -p "$OUTPUTDIR"
fi

# Download the source
VERSION=2.29.1
wget -c http://ftp.gnu.org/gnu/binutils/binutils-$VERSION.tar.bz2

# Verify the signature
wget -c -q http://ftp.gnu.org/gnu/binutils/binutils-$VERSION.tar.bz2.sig
if ! gpg --verify binutils-$VERSION.tar.bz2.sig; then
  echo "GPG Signature failed to verify."
  echo ""
  echo "You may need to import the vendor GPG key with:"
  echo "# gpg --keyserver hkp://pgp.mit.edu:80 --recv-key 4AE55E93 DD9E3C4F"
  exit 1
fi

# Extract the source
rm -rf binutils-$VERSION
tar jxf binutils-$VERSION.tar.bz2

for ARCH in i386 amd64; do
  CHROOT_DIR="xenial-chroot-$ARCH"
  if [ ! -d ${CHROOT_DIR} ]; then
    # Refresh sudo credentials
    sudo -v

    CHROOT_TEMPDIR=$(mktemp -d ${CHROOT_DIR}.XXXXXX)

    # Create the chroot
    echo ""
    echo "Building chroot for $ARCH"
    echo "============================="
    sudo debootstrap \
        --arch=$ARCH \
        --include=build-essential,flex,bison \
        xenial ${CHROOT_TEMPDIR}
    echo "============================="
    mv ${CHROOT_TEMPDIR} ${CHROOT_DIR}
  fi

  BUILDDIR=${CHROOT_DIR}/build

  # Clean up any previous failed build attempts inside chroot
  if [ -d "$BUILDDIR" ]; then
    sudo rm -rf "$BUILDDIR"
  fi

  # Copy data into the chroot
  sudo mkdir -p "$BUILDDIR"
  sudo cp -a binutils-$VERSION "$BUILDDIR"
  sudo cp -a build-one.sh "$BUILDDIR"

  # Do the build
  PREFIX=
  case $ARCH in
   i386)
     PREFIX="setarch linux32"
     ARCHNAME=i686-pc-linux-gnu
   ;;
   amd64)
     PREFIX="setarch linux64"
     ARCHNAME=x86_64-pc-linux-gnu
   ;;
  esac
  echo ""
  echo "Building binutils for $ARCH"
  LOGFILE="$OUTPUTDIR/build-$ARCH.log"
  if ! sudo $PREFIX chroot ${CHROOT_DIR} /build/build-one.sh \
    /build/binutils-$VERSION > $LOGFILE 2>&1; then
    echo "Build failed! See $LOGFILE for details."
    exit 1
  fi

  # Copy data out of the chroot
  sudo chown -R $(whoami) "$BUILDDIR/output/"

  # Strip the output binaries
  strip "$BUILDDIR/output/$ARCHNAME/bin/"*

  # Copy them out of the chroot
  cp -a "$BUILDDIR/output/$ARCHNAME" "$OUTPUTDIR"

  # Clean up chroot
  sudo rm -rf "$BUILDDIR"
done

echo "Check you are happy with the binaries in"
echo "  $OUTPUTDIR"
echo "Then"
echo " * upload to Google Storage using the upload.sh script"
echo " * roll dependencies"
