#!/bin/bash

# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Test of the Mac Chrome installer.


# Where I am
DIR=$(dirname "${0}")

# My installer to test
INSTALLER="${DIR}"/keystone_install.sh
if [ ! -f "${INSTALLER}" ]; then
  echo "Can't find scripts." >& 2
  exit 1
fi

# What I test
PRODNAME="Google Chrome"
APPNAME="${PRODNAME}.app"
FWKNAME="${PRODNAME} Framework.framework"

# The version number for fake ksadmin to pretend to be
KSADMIN_VERSION_LIE="1.0.7.1306"

# Temp directory to be used as the disk image (source)
TEMPDIR=$(mktemp -d -t $(basename ${0}))
PATH=$PATH:"${TEMPDIR}"

# Clean up the temp directory
function cleanup_tempdir() {
  chmod u+w "${TEMPDIR}"
  rm -rf "${TEMPDIR}"
}

# Run the installer and make sure it fails.
# If it succeeds, we fail.
# Arg0: string to print
function fail_installer() {
  echo $1
  "${INSTALLER}" "${TEMPDIR}" >& /dev/null
  RETURN=$?
  if [ $RETURN -eq 0 ]; then
    echo "Did not fail (which is a failure)" >& 2
    cleanup_tempdir
    exit 1
  else
    echo "Returns $RETURN"
  fi
}

# Make sure installer works!
# Arg0: string to print
function pass_installer() {
  echo $1
  "${INSTALLER}" "${TEMPDIR}" >& /dev/null
  RETURN=$?
  if [ $RETURN -ne 0 ]; then
    echo "FAILED; returned $RETURN but should have worked" >& 2
    cleanup_tempdir
    exit 1
  else
    echo "worked"
  fi
}

# Make an old-style destination directory, to test updating from old-style
# versions to new-style versions.
function make_old_dest() {
  DEST="${TEMPDIR}"/Dest.app
  rm -rf "${DEST}"
  mkdir -p "${DEST}"/Contents
  defaults write "${DEST}/Contents/Info" KSVersion 0
  cat >"${TEMPDIR}"/ksadmin <<EOF
#!/bin/sh
if [ "\${1}" = "--ksadmin-version" ] ; then
  echo "${KSADMIN_VERSION_LIE}"
  exit 0
fi
if [ -z "\${FAKE_SYSTEM_TICKET}" ] && [ "\${1}" = "-S" ] ; then
  echo no system tix! >& 2
  exit 1
fi
echo " xc=<KSPathExistenceChecker:0x45 path=${DEST}>"
exit 0
EOF
  chmod u+x "${TEMPDIR}"/ksadmin
}

# Make a new-style destination directory, to test updating between new-style
# versions.
function make_new_dest() {
  DEST="${TEMPDIR}"/Dest.app
  rm -rf "${DEST}"
  defaults write "${DEST}/Contents/Info" CFBundleShortVersionString 0
  defaults write "${DEST}/Contents/Info" KSVersion 0
  cat >"${TEMPDIR}"/ksadmin <<EOF
#!/bin/sh
if [ "\${1}" = "--ksadmin-version" ] ; then
  echo "${KSADMIN_VERSION_LIE}"
  exit 0
fi
if [ -z "\${FAKE_SYSTEM_TICKET}" ] && [ "\${1}" = "-S" ] ; then
  echo no system tix! >& 2
  exit 1
fi
echo " xc=<KSPathExistenceChecker:0x45 path=${DEST}>"
exit 0
EOF
  chmod u+x "${TEMPDIR}"/ksadmin
}

# Make a simple source directory - the update that is to be applied
function make_src() {
  chmod ugo+w "${TEMPDIR}"
  rm -rf "${TEMPDIR}/${APPNAME}"
  RSRCDIR="${TEMPDIR}/${APPNAME}/Contents/Versions/1/${FWKNAME}/Resources"
  mkdir -p "${RSRCDIR}"
  defaults write "${TEMPDIR}/${APPNAME}/Contents/Info" \
      CFBundleShortVersionString "1"
  defaults write "${TEMPDIR}/${APPNAME}/Contents/Info" \
      KSProductID "com.google.Chrome"
  defaults write "${TEMPDIR}/${APPNAME}/Contents/Info" \
      KSVersion "2"
}

function make_basic_src_and_dest() {
  make_src
  make_new_dest
}

fail_installer "No source anything"

mkdir "${TEMPDIR}"/"${APPNAME}"
fail_installer "No source bundle"

make_basic_src_and_dest
chmod ugo-w "${TEMPDIR}"
fail_installer "Writable dest directory"

make_basic_src_and_dest
fail_installer "Was no KSUpdateURL in dest after copy"

make_basic_src_and_dest
defaults write "${TEMPDIR}/${APPNAME}/Contents/Info" KSUpdateURL "http://foobar"
export FAKE_SYSTEM_TICKET=1
fail_installer "User and system ticket both present"
export -n FAKE_SYSTEM_TICKET

make_src
make_old_dest
defaults write "${TEMPDIR}/${APPNAME}/Contents/Info" KSUpdateURL "http://foobar"
pass_installer "Old-style update"

make_basic_src_and_dest
defaults write "${TEMPDIR}/${APPNAME}/Contents/Info" KSUpdateURL "http://foobar"
pass_installer "ALL"

cleanup_tempdir
