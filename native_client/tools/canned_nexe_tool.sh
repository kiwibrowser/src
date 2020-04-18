#!/bin/bash
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

#@ This script is used to update the archive of canned nexes
#@ Note, it does not recreate every nexe from scratch but will
#@ update those it can (currently: spec and translator nexes).
set -o nounset
set -o errexit

readonly UP_DOWN_LOAD_SCRIPT=buildbot/file_up_down_load.sh

readonly CANNED_DIR=CannedNexes

######################################################################
# Helpers
######################################################################

Banner() {
  echo "######################################################################"
  echo $*
  echo "######################################################################"
}

help() {
  egrep "^#@" $0 | cut --bytes=3-
}

DownloadCannedNexes() {
  local arch=$1
  local rev=$2
  Banner "Downloading rev: ${rev} arch: ${arch}"
  ${UP_DOWN_LOAD_SCRIPT} DownloadArchivedNexes \
      ${rev} "${arch}_giant" giant_nexe.tar.bz2
  # Untaring the tarball will generate "${CANNED_DIR}/" in the current directory
  rm -rf ${CANNED_DIR}
  tar jxf giant_nexe.tar.bz2
}

UploadCannedNexes() {
  local arch=$1
  local rev=$2
  Banner "Uploading rev: ${rev} arch: ${arch}"
  rm giant_nexe.tar.bz2
  tar jcf giant_nexe.tar.bz2 ${CANNED_DIR}

  ${UP_DOWN_LOAD_SCRIPT} UploadArchivedNexes \
      ${rev} "${arch}_giant" giant_nexe.tar.bz2
}

AddTranslatorNexes() {
  local arch=$1
  local dir="toolchain/linux_x86/pnacl_translator/translator/${arch}/bin"
  Banner "Updating Translator Nexes arch: ${arch}"
  # llc.nexe was renamed to pnacl-llc.nexe. Copy it keeping the old name
  # for continuity of historical data.
  # Also omit ld.nexe, since that is too small.
  cp ${dir}/pnacl-llc.nexe ${CANNED_DIR}/llc.nexe
}

Update() {
  local arch=$1
  local rev_in=$2
  local rev_out=$3
  DownloadCannedNexes ${arch} ${rev_in}
  AddTranslatorNexes ${arch}
  UploadCannedNexes ${arch} ${rev_out}
}

######################################################################
# "main"
######################################################################

if [ "$(type -t $1)" != "function" ]; then
  echo "ERROR: unknown function '$1'." >&2
  echo "For help, try:"
  echo "    $0 help"
  exit 1
fi

"$@"
