# Copyright (C) 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# A set of function shared by the 'build-host-xxxx.sh' scripts.
# They are mostly related to building host libraries.
#
# NOTE: This script uses various prefixes:
#
#    BH_       Used for public macros
#    bh_       Use for public functions
#
#    _BH_      Used for private macros
#    _bh_      Used for private functions
#
# Callers should only rely on the public macros and functions defined here.
#

# List of macros defined by the functions here:
#
#   defined by 'bh_set_build_tag'
#
#   BH_BUILD_CONFIG     Generic GNU config triplet for build system
#   BH_BUILD_OS         NDK system name
#   BH_BUILD_ARCH       NDK arch name
#   BH_BUILD_TAG        NDK system tag ($OS-$ARCH)
#   BH_BUILD_BITS       build system bitness (32 or 64)
#
#   defined by 'bh_set_host_tag'
#                          7
#   BH_HOST_CONFIG
#   BH_HOST_OS
#   BH_HOST_ARCH
#   BH_HOST_TAG
#   BH_HOST_BITS
#
#   defined by 'bh_set_target_tag'
#
#   BH_TARGET_CONFIG
#   BH_TARGET_OS
#   BH_TARGET_ARCH
#   BH_TARGET_TAG
#   BH_TARGET_BITS
#
#


# The values of HOST_OS/ARCH/TAG will be redefined during the build to
# match those of the system the generated compiler binaries will run on.
#
# Save the original ones into BUILD_XXX variants, corresponding to the
# machine where the build happens.
#
BH_BUILD_OS=$HOST_OS
BH_BUILD_ARCH=$HOST_ARCH
BH_BUILD_TAG=$HOST_TAG

# Map an NDK system tag to an OS name
# $1: system tag (e.g. linux-x86)
# Out: system name (e.g. linux)
bh_tag_to_os ()
{
    local RET
    case $1 in
        android-*) RET="android";;
        linux-*) RET="linux";;
        darwin-*) RET="darwin";;
        windows|windows-*) RET="windows";;
        *) echo "ERROR: Unknown tag $1" >&2; echo "INVALID"; exit 1;;
    esac
    echo $RET
}

# Map an NDK system tag to an architecture name
# $1: system tag (e.g. linux-x86)
# Out: arch name (e.g. x86)
bh_tag_to_arch ()
{
    local RET
    case $1 in
        *-arm) RET=arm;;
        *-arm64) RET=arm64;;
        *-mips) RET=mips;;
        *-mips64) RET=mips64;;
        windows|*-x86) RET=x86;;
        *-x86_64) RET=x86_64;;
        *) echo "ERROR: Unknown tag $1" >&2; echo "INVALID"; exit 1;;
    esac
    echo $RET
}

# Map an NDK system tag to a bit number
# $1: system tag (e.g. linux-x86)
# Out: bit number (32 or 64)
bh_tag_to_bits ()
{
    local RET
    case $1 in
        windows|*-x86|*-arm|*-mips) RET=32;;
        *-x86_64|*-arm64|*-mips64) RET=64;;
        *) echo "ERROR: Unknown tag $1" >&2; echo "INVALID"; exit 1;;
    esac
    echo $RET
}

# Map an NDK system tag to the corresponding GNU configuration triplet.
# $1: NDK system tag
# Out: GNU configuration triplet
bh_tag_to_config_triplet ()
{
    local RET
    case $1 in
        linux-x86) RET=i686-linux-gnu;;
        linux-x86_64) RET=x86_64-linux-gnu;;
        darwin-x86) RET=i686-apple-darwin;;
        darwin-x86_64) RET=x86_64-apple-darwin;;
        windows|windows-x86) RET=i586-pc-mingw32msvc;;
        windows-x86_64) RET=x86_64-w64-mingw32;;
        android-arm) RET=arm-linux-androideabi;;
        android-arm64) RET=aarch64-linux-android;;
        android-x86) RET=i686-linux-android;;
        android-x86_64) RET=x86_64-linux-android;;
        android-mips) RET=mipsel-linux-android;;
        android-mips64) RET=mips64el-linux-android;;
        *) echo "ERROR: Unknown tag $1" >&2; echo "INVALID"; exit 1;;
    esac
    echo "$RET"
}


bh_set_build_tag ()
{
  SAVED_OPTIONS=$(set +o)
  set -e
  BH_BUILD_OS=$(bh_tag_to_os $1)
  BH_BUILD_ARCH=$(bh_tag_to_arch $1)
  BH_BUILD_BITS=$(bh_tag_to_bits $1)
  BH_BUILD_TAG=$BH_BUILD_OS-$BH_BUILD_ARCH
  BH_BUILD_CONFIG=$(bh_tag_to_config_triplet $1)
  eval "$SAVED_OPTIONS"
}

# Set default BH_BUILD macros.
bh_set_build_tag $HOST_TAG

bh_set_host_tag ()
{
  SAVED_OPTIONS=$(set +o)
  set -e
  BH_HOST_OS=$(bh_tag_to_os $1)
  BH_HOST_ARCH=$(bh_tag_to_arch $1)
  BH_HOST_BITS=$(bh_tag_to_bits $1)
  BH_HOST_TAG=$BH_HOST_OS-$BH_HOST_ARCH
  BH_HOST_CONFIG=$(bh_tag_to_config_triplet $1)
  eval "$SAVED_OPTIONS"
}

bh_set_target_tag ()
{
  SAVED_OPTIONS=$(set +o)
  set -e
  BH_TARGET_OS=$(bh_tag_to_os $1)
  BH_TARGET_ARCH=$(bh_tag_to_arch $1)
  BH_TARGET_BITS=$(bh_tag_to_bits $1)
  BH_TARGET_TAG=$BH_TARGET_OS-$BH_TARGET_ARCH
  BH_TARGET_CONFIG=$(bh_tag_to_config_triplet $1)
  eval "$SAVED_OPTIONS"
}

bh_sort_systems_build_first ()
{
  local IN_SYSTEMS="$1"
  local OUT_SYSTEMS
  # Pull out the host if there
  for IN_SYSTEM in $IN_SYSTEMS; do
    if [ "$IN_SYSTEM" = "$BH_BUILD_TAG" ]; then
        OUT_SYSTEMS=$IN_SYSTEM
    fi
  done
  # Append the rest
  for IN_SYSTEM in $IN_SYSTEMS; do
    if [ ! "$IN_SYSTEM" = "$BH_BUILD_TAG" ]; then
        OUT_SYSTEMS=$OUT_SYSTEMS" $IN_SYSTEM"
    fi
  done
  echo $OUT_SYSTEMS
}

# $1 is the string to search for
# $2... is the list to search in
# Returns first, yes or no.
bh_list_contains ()
{
  local SEARCH="$1"
  shift
  # For dash, this has to be split over 2 lines.
  # Seems to be a bug with dash itself:
  # https://bugs.launchpad.net/ubuntu/+source/dash/+bug/141481
  local LIST
  LIST=$@
  local RESULT=first
  # Pull out the host if there
  for ELEMENT in $LIST; do
    if [ "$ELEMENT" = "$SEARCH" ]; then
      echo $RESULT
      return 0
    fi
    RESULT=yes
  done
  echo no
  return 1
}


# Use this function to enable/disable colored output
# $1: 'true' or 'false'
bh_set_color_mode ()
{
  local DO_COLOR=
  case $1 in
    on|enable|true) DO_COLOR=true
    ;;
  esac
  if [ "$DO_COLOR" ]; then
    _BH_COLOR_GREEN="\033[32m"
    _BH_COLOR_PURPLE="\033[35m"
    _BH_COLOR_CYAN="\033[36m"
    _BH_COLOR_END="\033[0m"
  else
    _BH_COLOR_GREEN=
    _BH_COLOR_PURPLE=
    _BH_COLOR_CYAN=
    _BH_COLOR_END=
  fi
}

# By default, enable color mode
bh_set_color_mode true

# Pretty printing with colors!
bh_host_text ()
{
    printf "[${_BH_COLOR_GREEN}${BH_HOST_TAG}${_BH_COLOR_END}]"
}

bh_toolchain_text ()
{
    printf "[${_BH_COLOR_PURPLE}${BH_TOOLCHAIN}${_BH_COLOR_END}]"
}

bh_target_text ()
{
    printf "[${_BH_COLOR_CYAN}${BH_TARGET_TAG}${_BH_COLOR_END}]"
}

bh_arch_text ()
{
    # Print arch name in cyan
    printf "[${_BH_COLOR_CYAN}${BH_TARGET_ARCH}${_BH_COLOR_END}]"
}

# Check that a given compiler generates code correctly
#
# This is to detect bad/broken toolchains, e.g. amd64-mingw32msvc
# is totally broken on Ubuntu 10.10 and 11.04
#
# $1: compiler
# $2: optional extra flags
#
bh_check_compiler ()
{
    local CC="$1"
    local TMPC=$TMPDIR/build-host-$USER-$$.c
    local TMPE=${TMPC%%.c}
    local TMPL=$TMPC.log
    local RET
    shift
    cat > $TMPC <<EOF
int main(void) { return 0; }
EOF
    log_n "Checking compiler code generation ($CC)... "
    $CC -o $TMPE $TMPC "$@" >$TMPL 2>&1
    RET=$?
    rm -f $TMPC $TMPE $TMPL
    if [ "$RET" = 0 ]; then
        log "yes"
    else
        log "no"
    fi
    return $RET
}


# $1: toolchain install dir
# $2: toolchain prefix, no trailing dash (e.g. arm-linux-androideabi)
# $3: optional -m32 or -m64.
_bh_try_host_fullprefix ()
{
    local PREFIX="$1/bin/$2"
    shift; shift;
    if [ -z "$HOST_FULLPREFIX" ]; then
        local GCC="$PREFIX-gcc"
        if [ -f "$GCC" ]; then
            if bh_check_compiler "$GCC" "$@"; then
                HOST_FULLPREFIX="${GCC%%gcc}"
                dump "$(bh_host_text) Using host gcc: $GCC $@"
            else
                dump "$(bh_host_text) Ignoring broken host gcc: $GCC $@"
            fi
        fi
    fi
}

# $1: host prefix, no trailing slash (e.g. i686-linux-android)
# $2: optional compiler args (should be empty, -m32 or -m64)
_bh_try_host_prefix ()
{
    local PREFIX="$1"
    shift
    if [ -z "$HOST_FULLPREFIX" ]; then
        local GCC="$(which $PREFIX-gcc 2>/dev/null)"
        if [ "$GCC" -a -e "$GCC" ]; then
            if bh_check_compiler "$GCC" "$@"; then
                HOST_FULLPREFIX=${GCC%%gcc}
                dump "$(bh_host_text) Using host gcc: ${HOST_FULLPREFIX}gcc $@"
            else
                dump "$(bh_host_text) Ignoring broken host gcc: $GCC $@"
            fi
        fi
    fi
}

# Used to determine the minimum possible Darwin version that a Darwin SDK
# can target. This actually depends from the host architecture.
# $1: Host architecture name
# out: SDK version number (e.g. 10.4 or 10.5)
_bh_darwin_arch_to_min_version ()
{
  if [ "$1" = "x86" ]; then
    echo "10.4"
  else
    echo "10.5"
  fi
}

# Use the check for the availability of a compatibility SDK in Darwin
# this can be used to generate binaries compatible with either Tiger or
# Leopard.
#
# $1: SDK root path
# $2: Darwin compatibility minimum version
_bh_check_darwin_sdk ()
{
    if [ -d "$1" -a -z "$HOST_CFLAGS" ] ; then
        HOST_CFLAGS="-isysroot $1 -mmacosx-version-min=$2 -DMAXOSX_DEPLOYEMENT_TARGET=$2"
        HOST_CXXFLAGS=$HOST_CFLAGS
        HOST_LDFLAGS="-syslibroot $1 -mmacosx-version-min=$2"
        dump "Generating $2-compatible binaries."
        return 0  # success
    fi
    return 1
}

# Check that a given compiler generates 32 or 64 bit code.
# $1: compiler full path (.e.g  /path/to/fullprefix-gcc)
# $2: 32 or 64
# $3: extract compiler flags
# Return: success iff the compiler generates $2-bits code
_bh_check_compiler_bitness ()
{
    local CC="$1"
    local BITS="$2"
    local TMPC=$TMPDIR/build-host-gcc-bits-$USER-$$.c
    local TMPL=$TMPC.log
    local RET
    shift; shift;
    cat > $TMPC <<EOF
/* this program will fail to compile if the compiler doesn't generate BITS-bits code */
int tab[1-2*(sizeof(void*)*8 != BITS)];
EOF
    dump_n "$(bh_host_text) Checking that the compiler generates $BITS-bits code ($@)... "
    $CC -c -DBITS=$BITS -o /dev/null $TMPC $HOST_CFLAGS "$@" > $TMPL 2>&1
    RET=$?
    rm -f $TMPC $TMPL
    if [ "$RET" = 0 ]; then
        dump "yes"
    else
        dump "no"
    fi
    return $RET
}

# This function probes the system to find the best toolchain or cross-toolchain
# to build binaries that run on a given host system. After that, it generates
# a wrapper toolchain under $2 with a prefix of ${BH_HOST_CONFIG}-
# where $BH_HOST_CONFIG is a GNU configuration name.
#
# Important: this script might redefine $BH_HOST_CONFIG to a different value!
#
# $1: NDK system tag (e.g. linux-x86)
#
# The following can be defined, otherwise they'll be auto-detected and set.
#
#  DARWIN_MIN_VERSION   -> Darwmin minimum compatibility version
#  DARWIN_SDK_VERSION   -> Darwin SDK version
#
# The following can be defined for extra features:
#
#  DARWIN_TOOLCHAIN     -> Path to Darwin cross-toolchain (cross-compile only).
#  DARWIN_SYSROOT       -> Path to Darwin SDK sysroot (cross-compile only).
#  NDK_CCACHE           -> Ccache binary to use to speed up rebuilds.
#  ANDROID_NDK_ROOT     -> Top-level NDK directory, for automatic probing
#                          of prebuilt platform toolchains.
#
_bh_select_toolchain_for_host ()
{
    local HOST_CFLAGS HOST_CXXFLAGS HOST_LDFLAGS HOST_FULLPREFIX DARWIN_ARCH
    local DARWIN_ARCH DARWIN_SDK_SUBDIR

    # We do all the complex auto-detection magic in the setup phase,
    # then save the result in host-specific global variables.
    #
    # In the build phase, we will simply restore the values into the
    # global HOST_FULLPREFIX / HOST_BUILD_DIR
    # variables.
    #

    # Try to find the best toolchain to do that job, assuming we are in
    # a full Android platform source checkout, we can look at the prebuilts/
    # directory.
    case $1 in
        linux-x86)
            # If possible, automatically use our custom toolchain to generate
            # 32-bit executables that work on Ubuntu 8.04 and higher.
            _bh_try_host_fullprefix "$(dirname $ANDROID_NDK_ROOT)/prebuilts/gcc/linux-x86/host/i686-linux-glibc2.7-4.6" i686-linux
            _bh_try_host_fullprefix "$(dirname $ANDROID_NDK_ROOT)/prebuilts/gcc/linux-x86/host/i686-linux-glibc2.7-4.4.3" i686-linux
            _bh_try_host_fullprefix "$(dirname $ANDROID_NDK_ROOT)/prebuilt/linux-x86/toolchain/i686-linux-glibc2.7-4.4.3" i686-linux
            _bh_try_host_prefix i686-linux-gnu
            _bh_try_host_prefix i686-linux
            _bh_try_host_prefix x86_64-linux-gnu -m32
            _bh_try_host_prefix x86_64-linux -m32
            ;;

        linux-x86_64)
            # If possible, automaticaly use our custom toolchain to generate
            # 64-bit executables that work on Ubuntu 8.04 and higher.
            _bh_try_host_fullprefix "$(dirname $ANDROID_NDK_ROOT)/prebuilts/gcc/linux-x86/host/x86_64-linux-glibc2.7-4.6" x86_64-linux
            _bh_try_host_prefix x86_64-linux-gnu
            _bh_try_host_prefix x84_64-linux
            _bh_try_host_prefix i686-linux-gnu -m64
            _bh_try_host_prefix i686-linux -m64
            ;;

        darwin-*)
            DARWIN_ARCH=$(bh_tag_to_arch $1)
            if [ -z "$DARWIN_MIN_VERSION" ]; then
                DARWIN_MIN_VERSION=$(_bh_darwin_arch_to_min_version $DARWIN_ARCH)
            fi
            case $BH_BUILD_OS in
                darwin)
                    if [ "$DARWIN_SDK_VERSION" ]; then
                        # Compute SDK subdirectory name
                        case $DARWIN_SDK_VERSION in
                            10.4) DARWIN_SDK_SUBDIR=$DARWIN_SDK.sdku;;
                            *) DARWIN_SDK_SUBDIR=$DARWIN_SDK.sdk;;
                        esac
                        # Since xCode moved to the App Store the SDKs have been 'sandboxed' into the Xcode.app folder.
                        _bh_check_darwin_sdk /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX$DARWIN_SDK_SUBDIR $DARWIN_MIN_VERSION
                        _bh_check_darwin_sdk /Developer/SDKs/MacOSX$DARWIN_SDK_SUBDIR $DARWIN_MIN_VERSION
                    else
                        # Since xCode moved to the App Store the SDKs have been 'sandboxed' into the Xcode.app folder.
                        _bh_check_darwin_sdk /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.7.sdk $DARWIN_MIN_VERSION
                        _bh_check_darwin_sdk /Developer/SDKs/MacOSX10.7.sdk  $DARWIN_MIN_VERSION
                        _bh_check_darwin_sdk /Developer/SDKs/MacOSX10.6.sdk  $DARWIN_MIN_VERSION
                        # NOTE: The 10.5.sdk on Lion is buggy and cannot build basic C++ programs
                        #_bh_check_darwin_sdk /Developer/SDKs/MacOSX10.5.sdk  $DARWIN_ARCH
                        # NOTE: The 10.4.sdku is not available anymore and could not be tested.
                        #_bh_check_darwin_sdk /Developer/SDKs/MacOSX10.4.sdku $DARWIN_ARCH
                    fi
                    if [ -z "$HOST_CFLAGS" ]; then
                        local version="$(sw_vers -productVersion)"
                        log "Generating $version-compatible binaries!"
                    fi
                    ;;
                *)
                    if [ -z "$DARWIN_TOOLCHAIN" -o -z "$DARWIN_SYSROOT" ]; then
                        dump "If you want to build Darwin binaries on a non-Darwin machine,"
                        dump "Please define DARWIN_TOOLCHAIN to name it, and DARWIN_SYSROOT to point"
                        dump "to the SDK. For example:"
                        dump ""
                        dump "   DARWIN_TOOLCHAIN=\"i686-apple-darwin11\""
                        dump "   DARWIN_SYSROOT=\"~/darwin-cross/MacOSX10.7.sdk\""
                        dump "   export DARWIN_TOOLCHAIN DARWIN_SYSROOT"
                        dump ""
                        exit 1
                    fi
                    _bh_check_darwin_sdk $DARWIN_SYSROOT $DARWIN_MIN_VERSION
                    _bh_try_host_prefix "$DARWIN_TOOLCHAIN" -m$(bh_tag_to_bits $1) --sysroot "$DARWIN_SYSROOT"
                    if [ -z "$HOST_FULLPREFIX" ]; then
                        dump "It looks like $DARWIN_TOOLCHAIN-gcc is not in your path, or does not work correctly!"
                        exit 1
                    fi
                    dump "Using darwin cross-toolchain: ${HOST_FULLPREFIX}gcc"
                    ;;
            esac
            ;;

        windows|windows-x86)
            case $BH_BUILD_OS in
                linux)
                    # We favor these because they are more recent, and because
                    # we have a script to rebuild them from scratch. See
                    # build-mingw64-toolchain.sh.
                    _bh_try_host_prefix x86_64-w64-mingw32 -m32
                    _bh_try_host_prefix i686-w64-mingw32
                    # Typically provided by the 'mingw32' package on Debian
                    # and Ubuntu systems.
                    _bh_try_host_prefix i586-mingw32msvc
                    # Special note for Fedora: this distribution used
                    # to have a mingw32-gcc package that provided a 32-bit
                    # only cross-toolchain named i686-pc-mingw32.
                    # Later versions of the distro now provide a new package
                    # named mingw-gcc which provides i686-w64-mingw32 and
                    # x86_64-w64-mingw32 instead.
                    _bh_try_host_prefix i686-pc-mingw32
                    if [ -z "$HOST_FULLPREFIX" ]; then
                        dump "There is no Windows cross-compiler. Ensure that you"
                        dump "have one of these installed and in your path:"
                        dump "   x86_64-w64-mingw32-gcc  (see build-mingw64-toolchain.sh)"
                        dump "   i686-w64-mingw32-gcc    (see build-mingw64-toolchain.sh)"
                        dump "   i586-mingw32msvc-gcc    ('mingw32' Debian/Ubuntu package)"
                        dump "   i686-pc-mingw32         (on Fedora)"
                        dump ""
                        exit 1
                    fi
                    # Adjust $HOST to match the toolchain to ensure proper builds.
                    # I.e. chose configuration triplets that are known to work
                    # with the gmp/mpfr/mpc/binutils/gcc configure scripts.
                    case $HOST_FULLPREFIX in
                        *-mingw32msvc-*|i686-pc-mingw32)
                            BH_HOST_CONFIG=i586-pc-mingw32msvc
                            ;;
                        *)
                            BH_HOST_CONFIG=i686-w64-mingw32msvc
                            ;;
                    esac
                    ;;
                *) panic "Sorry, this script only supports building windows binaries on Linux."
                ;;
            esac
            HOST_CFLAGS=$HOST_CFLAGS" -D__USE_MINGW_ANSI_STDIO=1"
            HOST_CXXFLAGS=$HOST_CXXFLAGS" -D__USE_MINGW_ANSI_STDIO=1"
            ;;

        windows-x86_64)
            case $BH_BUILD_OS in
                linux)
                    # See comments above for windows-x86
                    _bh_try_host_prefix x86_64-w64-mingw32
                    _bh_try_host_prefix i686-w64-mingw32 -m64
                    # Beware that this package is completely broken on many
                    # versions of no vinegar Ubuntu (i.e. it fails at building trivial
                    # programs).
                    _bh_try_host_prefix amd64-mingw32msvc
                    # There is no x86_64-pc-mingw32 toolchain on Fedora.
                    if [ -z "$HOST_FULLPREFIX" ]; then
                        dump "There is no Windows cross-compiler in your path. Ensure you"
                        dump "have one of these installed and in your path:"
                        dump "   x86_64-w64-mingw32-gcc  (see build-mingw64-toolchain.sh)"
                        dump "   i686-w64-mingw32-gcc    (see build-mingw64-toolchain.sh)"
                        dump "   amd64-mingw32msvc-gcc   (Debian/Ubuntu - broken until Ubuntu 11.10)"
                        dump ""
                        exit 1
                    fi
                    # See comment above for windows-x86
                    case $HOST_FULLPREFIX in
                        *-mingw32msvc*)
                            # Actually, this has never been tested.
                            BH_HOST=amd64-pc-mingw32msvc
                            ;;
                        *)
                            BH_HOST=x86_64-w64-mingw32
                            ;;
                    esac
                    ;;

                *) panic "Sorry, this script only supports building windows binaries on Linux."
                ;;
            esac
            HOST_CFLAGS=$HOST_CFLAGS" -D__USE_MINGW_ANSI_STDIO=1"
            HOST_CXXFLAGS=$HOST_CXXFLAGS" -D__USE_MINGW_ANSI_STDIO=1"
            ;;
    esac

    # Determine the default bitness of our compiler. It it doesn't match
    # HOST_BITS, tries to see if it supports -m32 or -m64 to change it.
    if ! _bh_check_compiler_bitness ${HOST_FULLPREFIX}gcc $BH_HOST_BITS; then
        local TRY_CFLAGS
        case $BH_HOST_BITS in
            32) TRY_CFLAGS=-m32;;
            64) TRY_CFLAGS=-m64;;
        esac
        if ! _bh_check_compiler_bitness ${HOST_FULLPREFIX}gcc $BH_HOST_BITS $TRY_CFLAGS; then
            panic "Can't find a way to generate $BH_HOST_BITS binaries with this compiler: ${HOST_FULLPREFIX}gcc"
        fi
        HOST_CFLAGS=$HOST_CFLAGS" "$TRY_CFLAGS
        HOST_CXXFLAGS=$HOST_CXXFLAGS" "$TRY_CFLAGS
    fi

    # Support for ccache, to speed up rebuilds.
    DST_PREFIX=$HOST_FULLPREFIX
    local CCACHE=
    if [ "$NDK_CCACHE" ]; then
        CCACHE="--ccache=$NDK_CCACHE"
    fi

    # We're going to generate a wrapper toolchain with the $HOST prefix
    # i.e. if $HOST is 'i686-linux-gnu', then we're going to generate a
    # wrapper toolchain named 'i686-linux-gnu-gcc' that will redirect
    # to whatever HOST_FULLPREFIX points to, with appropriate modifier
    # compiler/linker flags.
    #
    # This helps tremendously getting stuff to compile with the GCC
    # configure scripts.
    #
    run mkdir -p "$BH_WRAPPERS_DIR" &&
    run $NDK_BUILDTOOLS_PATH/gen-toolchain-wrapper.sh "$BH_WRAPPERS_DIR" \
        --src-prefix="$BH_HOST_CONFIG-" \
        --dst-prefix="$DST_PREFIX" \
        --cflags="$HOST_CFLAGS" \
        --cxxflags="$HOST_CXXFLAGS" \
        --ldflags="$HOST_LDFLAGS" \
        $CCACHE
}


# Setup the build directory, i.e. a directory where all intermediate
# files will be placed.
#
# $1: Build directory. Required.
#
# $2: Either 'preserve' or 'remove'. Indicates what to do of
#     existing files in the build directory, if any.
#
# $3: Either 'release' or 'debug'. Compilation mode.
#
bh_setup_build_dir ()
{
    BH_BUILD_DIR="$1"
    if [ -z "$BH_BUILD_DIR" ]; then
        panic "bh_setup_build_dir received no build directory"
    fi
    mkdir -p "$BH_BUILD_DIR"
    fail_panic "Could not create build directory: $BH_BUILD_DIR"

    if [ "$_BH_OPTION_FORCE" ]; then
        rm -rf "$BH_BUILD_DIR"/*
    fi

    if [ "$_BH_OPTION_NO_STRIP" ]; then
        BH_BUILD_MODE=debug
    else
        BH_BUILD_MODE=release
    fi

    # The directory that will contain our toolchain wrappers
    BH_WRAPPERS_DIR=$BH_BUILD_DIR/toolchain-wrappers
    rm -rf "$BH_WRAPPERS_DIR" && mkdir "$BH_WRAPPERS_DIR"
    fail_panic "Could not create wrappers dir: $BH_WRAPPERS_DIR"

    # The directory that will contain our timestamps
    BH_STAMPS_DIR=$BH_BUILD_DIR/timestamps
    mkdir -p "$BH_STAMPS_DIR"
    fail_panic "Could not create timestamps dir"
}

# Call this before anything else to setup a few important variables that are
# used consistently to build any host-specific binaries.
#
# $1: Host system name (e.g. linux-x86), this is the name of the host system
#     where the generated GCC binaries will run, not the current machine's
#     type (this one is in $ORIGINAL_HOST_TAG instead).
#
bh_setup_build_for_host ()
{
    local HOST_VARNAME=$(dashes_to_underscores $1)
    local HOST_VAR=_BH_HOST_${HOST_VARNAME}

    # Determine the host configuration triplet in $HOST
    bh_set_host_tag $1

    # Note: since _bh_select_toolchain_for_host can change the value of
    # $BH_HOST_CONFIG, we need to save it in a variable to later get the
    # correct one when this function is called again.
    if [ -z "$(var_value ${HOST_VAR}_SETUP)" ]; then
        _bh_select_toolchain_for_host $1
        var_assign ${HOST_VAR}_CONFIG $BH_HOST_CONFIG
        var_assign ${HOST_VAR}_SETUP true
    else
        BH_HOST_CONFIG=$(var_value ${HOST_VAR}_CONFIG)
    fi
}

# This function is used to setup the build environment whenever we
# generate host-specific binaries. You should call it before invoking
# a configure script or make.
#
# It assume sthat bh_setup_build_for_host was called with the right
# host system tag and wrappers directory.
#
bh_setup_host_env ()
{
    CC=$BH_HOST_CONFIG-gcc
    CXX=$BH_HOST_CONFIG-g++
    LD=$BH_HOST_CONFIG-ld
    AR=$BH_HOST_CONFIG-ar
    AS=$BH_HOST_CONFIG-as
    RANLIB=$BH_HOST_CONFIG-ranlib
    NM=$BH_HOST_CONFIG-nm
    STRIP=$BH_HOST_CONFIG-strip
    STRINGS=$BH_HOST_CONFIG-strings
    export CC CXX LD AR AS RANLIB NM STRIP STRINGS

    CFLAGS=
    CXXFLAGS=
    LDFLAGS=
    case $BH_BUILD_MODE in
        release)
            CFLAGS="-O2 -Os -fomit-frame-pointer -s"
            CXXFLAGS=$CFLAGS
            ;;
        debug)
            CFLAGS="-O0 -g"
            CXXFLAGS=$CFLAGS
            ;;
    esac
    export CFLAGS CXXFLAGS LDFLAGS

    export PATH=$BH_WRAPPERS_DIR:$PATH
}

_bh_option_no_color ()
{
    bh_set_color_mode off
}

# This function is used to register a few command-line options that
# impact the build of host binaries. Call it before invoking
# extract_parameters to add them automatically.
#
bh_register_options ()
{
    BH_HOST_SYSTEMS="$BH_BUILD_TAG"
    register_var_option "--systems=<list>" BH_HOST_SYSTEMS "Build binaries that run on these systems."

    _BH_OPTION_FORCE=
    register_var_option "--force" _BH_OPTION_FORCE "Force rebuild."

    _BH_OPTION_NO_STRIP=
    register_var_option "--no-strip" _BH_OPTION_NO_STRIP "Don't strip generated binaries."

    register_option "--no-color" _bh_option_no_color "Don't output colored text."

    if [ "$HOST_OS" = darwin ]; then
        DARWIN_SDK_VERSION=
        register_var_option "--darwin-sdk-version=<version>" DARWIN_SDK "Select Darwin SDK version."

        DARWIN_MIN_VERSION=
        register_var_option "--darwin-min-version=<version>" DARWIN_MIN_VERSION "Select minimum OS X version of generated host toolchains."
    fi
}

# Execute a given command.
#
# NOTE: The command is run in its own sub-shell to avoid environment
#        contamination.
#
# $@: command
bh_do ()
{
    ("$@")
    fail_panic
}

# Return the build install directory of a given Python version
#
# $1: host system tag
# $2: python version
# The suffix of this has to match python_ndk_install_dir
#  as I package them from the build folder, substituting
#  the end part of python_build_install_dir matching
#  python_ndk_install_dir with nothing.
python_build_install_dir ()
{
    echo "$BH_BUILD_DIR/$1/install/host-tools"
}

# Same as python_build_install_dir, but for the final NDK installation
# directory. Relative to $NDK_DIR.
#
# $1: host system tag
python_ndk_install_dir ()
{
    echo "host-tools"
}
