#!/bin/bash
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
#@ This script for up/downloading native client toolchains, etc..
#@ To manually inspect what is on the store servers point your
#@ browser at:
#@ http://gsdview.appspot.com/nativeclient-archive2/

#set -o xtrace
set -o nounset
set -o errexit

######################################################################
# Helper
######################################################################

Banner() {
  echo "######################################################################"
  echo $*
  echo "######################################################################"
}


Usage() {
  egrep "^#@" $0 | cut --bytes=3-
}

SanityCheck() {
  Banner "Sanity Checks"
  if [[ $(basename $(pwd)) != "native_client" ]] ; then
    echo "ERROR: run this script from the native_client/ dir"
    exit -1
  fi
}

######################################################################
# Config
######################################################################

readonly GS_UTIL=${GS_UTIL:-buildbot/gsutil.sh}

readonly DIR_ARCHIVE=nativeclient-archive2
readonly DIR_TRYBOT=nativeclient-trybot

readonly GS_PREFIX_ARCHIVE="gs://${DIR_ARCHIVE}"
readonly GS_PREFIX_TRYBOT="gs://${DIR_TRYBOT}"

readonly URL_PREFIX_UI="http://gsdview.appspot.com"
######################################################################
# UTIL
######################################################################
GetFileSizeK() {
  # Note: this is tricky to make work on win/linux/mac
  du -k $1 | egrep -o "^[0-9]+"
}


Upload() {
  local size_kb=$(GetFileSizeK $1)
  echo "uploading: $2 (${size_kb}kB)"
  local path=${2:5}
  echo "@@@STEP_LINK@download (${size_kb}kB)@${URL_PREFIX_UI}/${path}@@@"
  ${GS_UTIL} cp -a public-read $1 $2
}

CheckPath() {
  if [[ $1 != toolchain/* &&
       $1 != between_builders/* &&
       $1 != canned_nexe/* ]] ; then
      echo "ERROR: Bad component name: $1"
      exit -1
  fi
}

UploadArchive() {
  local path=$1
  local tarball=$2

  CheckPath ${path}
  Upload ${tarball} ${GS_PREFIX_ARCHIVE}/${path}
}

DownloadArchive() {
  local path=$1
  local tarball=$2

  echo "@@@STEP_LINK@download@${URL_PREFIX_UI}/${path}@@@"
  ${GS_UTIL} cp ${GS_PREFIX_ARCHIVE}/${path} ${tarball}
}

UploadTrybot() {
  local path=$1
  local tarball=$2

  CheckPath ${path}
  Upload ${tarball} ${GS_PREFIX_TRYBOT}/${path}
}

DownloadTrybot() {
  local path=$1
  local tarball=$2

  echo "@@@STEP_LINK@download@${URL_PREFIX_UI}/${path}@@@"
  ${GS_UTIL} cp ${GS_PREFIX_TRYBOT}/${path} ${tarball}
}

ComputeSha1() {
  # on mac we do not have sha1sum so we fall back to openssl
  if which sha1sum >/dev/null ; then
    echo "$(SHA1=$(sha1sum -b $1) ; echo ${SHA1:0:40})"
  elif which openssl >/dev/null ; then
    echo "$(SHA1=$(openssl sha1 $1) ; echo ${SHA1/* /})"

  else
    echo "ERROR: do not know how to compute SHA1"
    exit 1
  fi
}

######################################################################
# ARM TRUSTED
######################################################################

UploadArmTrustedToolchain() {
  local rev=$1
  local tarball=$2

  UploadArchive toolchain/${rev}/naclsdk_linux_arm-trusted.tgz ${tarball}
}

DownloadArmTrustedToolchain() {
  local rev=$1
  local tarball=$2
  DownloadArchive toolchain/${rev}/naclsdk_linux_arm-trusted.tgz ${tarball}
}

ShowRecentArmTrustedToolchains() {
   local url=${GS_PREFIX_ARCHIVE}/toolchain/*/naclsdk_linux_arm-trusted.tgz
   local recent=$(${GS_UTIL} ls ${url} | tail -5)
   for url in ${recent} ; do
     if ${GS_UTIL} ls -L "${url}" ; then
       echo "====="
     fi
   done
}

######################################################################
# ARM UN-TRUSTED
######################################################################

#@ label should be in :
#@
#@ pnacl_linux_x86
#@ pnacl_mac_x86
#@ pnacl_win_x86

UploadToolchainTarball() {
  local rev=$1
  local label=$2
  local tarball=$3

  ComputeSha1 ${tarball} > ${tarball}.sha1hash
  UploadArchive toolchain/${rev}/naclsdk_${label}.tgz.sha1hash ${tarball}.sha1hash

  # NOTE: only the last link is shown on the waterfall so this should come last
  UploadArchive toolchain/${rev}/naclsdk_${label}.tgz ${tarball}
}

DownloadPnaclToolchains() {
  local rev=$1
  local label=$2
  local tarball=$3

  DownloadArchive toolchain/${rev}/naclsdk_${label}.tgz ${tarball}
}

ShowRecentPnaclToolchains() {
  local label=$1
  local url="${GS_PREFIX_ARCHIVE}/toolchain/*/naclsdk_${label}.tgz"

  local recent=$(${GS_UTIL} ls ${url} | tail -5)
  for url in ${recent} ; do
    if ${GS_UTIL} ls -L "${url}" ; then
      echo "====="
    fi
  done
}

######################################################################
# Nexes for regression/speed tests
######################################################################

UploadArchivedNexes() {
  local rev=$1
  local label="archived_nexes_$2.tar.bz2"
  local tarball=$3

  # TODO(robertm,bradn): find another place to store this and
  #                      negotiate long term storage guarantees
  UploadArchive canned_nexe/${rev}/${label} ${tarball}
}

DownloadArchivedNexes() {
  local rev=$1
  local label="archived_nexes_$2.tar.bz2"
  local tarball=$3

  DownloadArchive canned_nexe/${rev}/${label} ${tarball}
}

######################################################################
# Pexes for bitcode stability testing
######################################################################

UploadArchivedPexes() {
  local rev=$1
  local label="archived_pexes_$2.tar.bz2"
  local tarball=$3

  # TODO(robertm,bradn): find another place to store this and
  #                      negotiate long term storage guarantees
  # Note, we store the pexes with the toolchain rev for now
  UploadArchive toolchain/${rev}/${label} ${tarball}
}

DownloadArchivedPexes() {
  local rev=$1
  local label="archived_pexes_$2.tar.bz2"
  local tarball=$3
  DownloadArchive toolchain/${rev}/${label} ${tarball}
}

UploadArchivedPexesSpec2k() {
    UploadArchivedPexes $1 "spec2k" $2
}

DownloadArchivedPexesSpec2k() {
    DownloadArchivedPexes $1 "spec2k" $2
}
######################################################################
# ARM BETWEEN BOTS
######################################################################

UploadArmBinariesForHWBots() {
  local name=$1
  local tarball=$2
  UploadArchive between_builders/${name}/$(basename ${tarball}) ${tarball}
}


DownloadArmBinariesForHWBots() {
  local name=$1
  local tarball=$2
  DownloadArchive between_builders/${name}/$(basename ${tarball}) ${tarball}
}

######################################################################
# ARM BETWEEN BOTS TRY
######################################################################

UploadArmBinariesForHWBotsTry() {
  local name=$1
  local tarball=$2
  UploadTrybot between_builders/${name}/$(basename ${tarball}) ${tarball}
}


DownloadArmBinariesForHWBotsTry() {
  local name=$1
  local tarball=$2
  DownloadTrybot between_builders/${name}/$(basename ${tarball}) ${tarball}
}

######################################################################
# DISPATCH
######################################################################
SanityCheck

if [[ $# -eq 0 ]] ; then
  echo "you must specify a mode on the commandline:"
  echo
  Usage
  exit -1
elif [[ "$(type -t $1)" != "function" ]]; then
  echo "ERROR: unknown function '$1'." >&2
  echo "For help, try:"
  echo "    $0 help"
  exit 1
else
  "$@"
fi
