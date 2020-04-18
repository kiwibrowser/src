#!/bin/bash
#
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# TODO(michaelpg): Dedupe common functionality with the Chrome installer.

set -e
set -o pipefail
if [ "$VERBOSE" ]; then
  set -x
fi
set -u

# Create the Debian changelog file needed by dpkg-gencontrol. This just adds a
# placeholder change, indicating it is the result of an automatic build.
# TODO(mmoss) Release packages should create something meaningful for a
# changelog, but simply grabbing the actual 'svn log' is way too verbose. Do we
# have any type of "significant/visible changes" log that we could use for this?
gen_changelog() {
  rm -f "${DEB_CHANGELOG}"
  process_template "${SCRIPTDIR}/changelog.template" "${DEB_CHANGELOG}"
  debchange -a --nomultimaint -m --changelog "${DEB_CHANGELOG}" \
    "Release Notes: ${RELEASENOTES}"
  GZLOG="${STAGEDIR}/usr/share/doc/${PACKAGE}-${CHANNEL}/changelog.gz"
  mkdir -p "$(dirname "${GZLOG}")"
  gzip -9 -c "${DEB_CHANGELOG}" > "${GZLOG}"
  chmod 644 "${GZLOG}"
}

# Create the Debian control file needed by dpkg-deb.
gen_control() {
  dpkg-gencontrol -v"${VERSIONFULL}" -c"${DEB_CONTROL}" -l"${DEB_CHANGELOG}" \
  -f"${DEB_FILES}" -p"${PACKAGE}-${CHANNEL}" -P"${STAGEDIR}" \
  -O > "${STAGEDIR}/DEBIAN/control"
  rm -f "${DEB_CONTROL}"
}

# Setup the installation directory hierachy in the package staging area.
prep_staging_debian() {
  prep_staging_common
  install -m 755 -d "${STAGEDIR}/DEBIAN" \
    "${STAGEDIR}/usr/share/doc/${USR_BIN_SYMLINK_NAME}"
}

# Put the package contents in the staging area.
stage_install_debian() {
  # Always use a different name for /usr/bin symlink depending on channel to
  # avoid file collisions.
  local USR_BIN_SYMLINK_NAME="${PACKAGE}-${CHANNEL}"

  if [ "$CHANNEL" != "stable" ]; then
    # Avoid file collisions between channels.
    local INSTALLDIR="${INSTALLDIR}-${CHANNEL}"

    local PACKAGE="${PACKAGE}-${CHANNEL}"
  fi
  prep_staging_debian
  stage_install_common
}

# Actually generate the package file.
do_package() {
  echo "Packaging ${ARCHITECTURE}..."
  PREDEPENDS="$COMMON_PREDEPS"
  DEPENDS="${COMMON_DEPS}"
  RECOMMENDS="${COMMON_RECOMMENDS}"
  PROVIDES=""
  gen_changelog
  process_template "${SCRIPTDIR}/control.template" "${DEB_CONTROL}"
  export DEB_HOST_ARCH="${ARCHITECTURE}"
  if [ -f "${DEB_CONTROL}" ]; then
    gen_control
  fi
  if [ ${IS_OFFICIAL_BUILD} -ne 0 ]; then
    local COMPRESSION_OPTS="-Zxz -z9"
  else
    local COMPRESSION_OPTS="-Znone"
  fi
  fakeroot dpkg-deb ${COMPRESSION_OPTS} -b "${STAGEDIR}" .
}

verify_package() {
  DEPENDS="${COMMON_DEPS}"  # This needs to match do_package() above.
  echo ${DEPENDS} | sed 's/, /\n/g' | LANG=C sort > expected_deb_depends
  dpkg -I "${PACKAGE}-${CHANNEL}_${VERSIONFULL}_${ARCHITECTURE}.deb" | \
      grep '^ Depends: ' | sed 's/^ Depends: //' | sed 's/, /\n/g' | \
      LANG=C sort > actual_deb_depends
  BAD_DIFF=0
  diff -u expected_deb_depends actual_deb_depends || BAD_DIFF=1
  if [ $BAD_DIFF -ne 0 ]; then
    echo
    echo "ERROR: bad dpkg dependencies!"
    echo
    exit $BAD_DIFF
  fi
}

# Remove temporary files and unwanted packaging output.
cleanup() {
  echo "Cleaning..."
  rm -rf "${STAGEDIR}"
  rm -rf "${TMPFILEDIR}"
}

usage() {
  echo "usage: $(basename $0) [-a target_arch] [-b 'dir'] -c channel"
  echo "                      -d branding [-f] [-o 'dir'] -s 'dir'"
  echo "-a arch     package architecture (ia32 or x64)"
  echo "-b dir      build input directory    [${BUILDDIR}]"
  echo "-c channel  the package channel (unstable, beta, stable)"
  echo "-d brand    either chromium or google_chrome"
  echo "-f          indicates that this is an official build"
  echo "-h          this help message"
  echo "-o dir      package output directory [${OUTPUTDIR}]"
  echo "-s dir      /path/to/sysroot"
}

# Check that the channel name is one of the allowable ones.
verify_channel() {
  case $CHANNEL in
    stable )
      CHANNEL=stable
      RELEASENOTES="http://googlechromereleases.blogspot.com/search/label/Stable%20updates"
      ;;
    unstable|dev|alpha )
      CHANNEL=unstable
      RELEASENOTES="http://googlechromereleases.blogspot.com/search/label/Dev%20updates"
      ;;
    testing|beta )
      CHANNEL=beta
      RELEASENOTES="http://googlechromereleases.blogspot.com/search/label/Beta%20updates"
      ;;
    * )
      echo
      echo "ERROR: '$CHANNEL' is not a valid channel type."
      echo
      exit 1
      ;;
  esac
}

process_opts() {
  while getopts ":a:b:c:d:fho:s:" OPTNAME
  do
    case $OPTNAME in
      a )
        TARGETARCH="$OPTARG"
        ;;
      b )
        BUILDDIR=$(readlink -f "${OPTARG}")
        ;;
      c )
        CHANNEL="$OPTARG"
        ;;
      d )
        BRANDING="$OPTARG"
        ;;
      f )
        IS_OFFICIAL_BUILD=1
        ;;
      h )
        usage
        exit 0
        ;;
      o )
        OUTPUTDIR=$(readlink -f "${OPTARG}")
        mkdir -p "${OUTPUTDIR}"
        ;;
      s )
        SYSROOT="$OPTARG"
        ;;
      \: )
        echo "'-$OPTARG' needs an argument."
        usage
        exit 1
        ;;
      * )
        echo "invalid command-line option: $OPTARG"
        usage
        exit 1
        ;;
    esac
  done
}

#=========
# MAIN
#=========

SCRIPTDIR=$(readlink -f "$(dirname "$0")")
OUTPUTDIR="${PWD}"
# Default target architecture to same as build host.
if [ "$(uname -m)" = "x86_64" ]; then
  TARGETARCH="x64"
else
  TARGETARCH="ia32"
fi

# call cleanup() on exit
trap cleanup 0
process_opts "$@"
BUILDDIR=${BUILDDIR:=$(readlink -f "${SCRIPTDIR}/../../../../out/Release")}
IS_OFFICIAL_BUILD=${IS_OFFICIAL_BUILD:=0}

STAGEDIR="${BUILDDIR}/app-shell-deb-staging-${CHANNEL}"
mkdir -p "${STAGEDIR}"
TMPFILEDIR="${BUILDDIR}/app-shell-deb-tmp-${CHANNEL}"
mkdir -p "${TMPFILEDIR}"
DEB_CHANGELOG="${TMPFILEDIR}/changelog"
DEB_FILES="${TMPFILEDIR}/files"
DEB_CONTROL="${TMPFILEDIR}/control"

source ${BUILDDIR}/app_shell_installer/common/installer.include

get_version_info
VERSIONFULL="${VERSION}-${PACKAGE_RELEASE}"

if [ "$BRANDING" = "google_chrome" ]; then
  source "${BUILDDIR}/app_shell_installer/common/google-app-shell.info"
else
  source "${BUILDDIR}/app_shell_installer/common/chromium-app-shell.info"
fi
eval $(sed -e "s/^\([^=]\+\)=\(.*\)$/export \1='\2'/" \
  "${BUILDDIR}/app_shell_installer/theme/BRANDING")

verify_channel

# Some Debian packaging tools want these set.
export DEBFULLNAME="${MAINTNAME}"
export DEBEMAIL="${MAINTMAIL}"

# We'd like to eliminate more of these deps by relying on the 'lsb' package, but
# that brings in tons of unnecessary stuff, like an mta and rpm. Until that full
# 'lsb' package is installed by default on DEB distros, we'll have to stick with
# the LSB sub-packages, to avoid pulling in all that stuff that's not installed
# by default.

# Generate the dependencies,
# TODO(mmoss): This is a workaround for a problem where dpkg-shlibdeps was
# resolving deps using some of our build output shlibs (i.e.
# out/Release/lib.target/libfreetype.so.6), and was then failing with:
#   dpkg-shlibdeps: error: no dependency information found for ...
# It's not clear if we ever want to look in LD_LIBRARY_PATH to resolve deps,
# but it seems that we don't currently, so this is the most expediant fix.
SAVE_LDLP=${LD_LIBRARY_PATH:-}
unset LD_LIBRARY_PATH
if [ ${TARGETARCH} = "x64" ]; then
  SHLIB_ARGS="-l${SYSROOT}/usr/lib/x86_64-linux-gnu"
  SHLIB_ARGS="${SHLIB_ARGS} -l${SYSROOT}/lib/x86_64-linux-gnu"
else
  SHLIB_ARGS="-l${SYSROOT}/usr/lib/i386-linux-gnu"
  SHLIB_ARGS="${SHLIB_ARGS} -l${SYSROOT}/lib/i386-linux-gnu"
fi
SHLIB_ARGS="${SHLIB_ARGS} -l${SYSROOT}/usr/lib"
DPKG_SHLIB_DEPS=$(cd ${SYSROOT} && dpkg-shlibdeps ${SHLIB_ARGS:-} -O \
                  -e"$BUILDDIR/app_shell" | sed 's/^shlibs:Depends=//')
if [ -n "$SAVE_LDLP" ]; then
  LD_LIBRARY_PATH=$SAVE_LDLP
fi

# Format it nicely and save it for comparison.
echo "$DPKG_SHLIB_DEPS" | sed 's/, /\n/g' | LANG=C sort > actual

# Additional dependencies not in the dpkg-shlibdeps output.
# ca-certificates: Make sure users have SSL certificates.
# libnss3: Pull a more recent version of NSS than required by runtime linking,
#          for security and stability updates in NSS.
# lsb-release: For lsb-release.
# wget: For uploading crash reports with Breakpad.
ADDITIONAL_DEPS="ca-certificates, libnss3 (>= 3.26), lsb-release, wget"

# Fix-up libnspr dependency due to renaming in Ubuntu (the old package still
# exists, but it was moved to "universe" repository, which isn't installed by
# default).
DPKG_SHLIB_DEPS=$(sed \
    's/\(libnspr4-0d ([^)]*)\), /\1 | libnspr4 (>= 4.9.5-0ubuntu0), /g' \
    <<< $DPKG_SHLIB_DEPS)

# Remove libnss dependency so the one in $ADDITIONAL_DEPS can supercede it.
DPKG_SHLIB_DEPS=$(sed 's/\(libnss3 ([^)]*)\), //g' <<< $DPKG_SHLIB_DEPS)

COMMON_DEPS="${DPKG_SHLIB_DEPS}, ${ADDITIONAL_DEPS}"
COMMON_PREDEPS="dpkg (>= 1.14.0)"
COMMON_RECOMMENDS="libu2f-udev"


# Make everything happen in the OUTPUTDIR.
cd "${OUTPUTDIR}"

case "$TARGETARCH" in
  ia32 )
    export ARCHITECTURE="i386"
    ;;
  x64 )
    export ARCHITECTURE="amd64"
    ;;
  * )
    echo
    echo "ERROR: Don't know how to build DEBs for '$TARGETARCH'."
    echo
    exit 1
    ;;
esac
stage_install_debian

do_package
verify_package
