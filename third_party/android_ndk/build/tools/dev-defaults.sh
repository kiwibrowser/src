# Default values used by several dev-scripts.
#

# This script is imported while building the NDK, while running the tests, and
# when running make-standalone-toolchain.sh. Check if we have our own platforms
# tree (as we would in an installed NDK) first, and fall back to prebuilts/ndk.
PLATFORMS_DIR=$ANDROID_NDK_ROOT/platforms
if [ ! -d "$PLATFORMS_DIR" ]; then
    PLATFORMS_DIR=$ANDROID_NDK_ROOT/../prebuilts/ndk/current/platforms
fi
API_LEVELS=$(ls $PLATFORMS_DIR | sed 's/android-//' | sort -n)

# The latest API level is the last one in the list.
LATEST_API_LEVEL=$(echo $API_LEVELS | awk '{ print $NF }')

FIRST_API64_LEVEL=21

# Default ABIs for the target prebuilt binaries.
PREBUILT_ABIS="armeabi armeabi-v7a x86 mips arm64-v8a x86_64 mips64"

# Location of the STLport sources, relative to the NDK root directory
STLPORT_SUBDIR=sources/cxx-stl/stlport

# Location of the GAbi++ sources, relative to the NDK root directory
GABIXX_SUBDIR=sources/cxx-stl/gabi++

# Location of the GNU libstdc++ headers and libraries, relative to the NDK
# root directory.
GNUSTL_SUBDIR=sources/cxx-stl/gnu-libstdc++

# Location of the LLVM libc++ headers and libraries, relative to the NDK
# root directory.
LIBCXX_SUBDIR=sources/cxx-stl/llvm-libc++

# Location of the LLVM libc++abi headers, relative to the NDK # root directory.
LIBCXXABI_SUBDIR=sources/cxx-stl/llvm-libc++abi/libcxxabi

# Location of the gccunwind sources, relative to the NDK root directory
GCCUNWIND_SUBDIR=sources/android/gccunwind

# Location of the support sources for libc++, relative to the NDK root directory
SUPPORT_SUBDIR=sources/android/support

# The date to use when downloading toolchain sources from AOSP servers
# Leave it empty for tip of tree.
TOOLCHAIN_GIT_DATE=now

# The space-separated list of all GCC versions we support in this NDK
DEFAULT_GCC_VERSION_LIST="4.9"

DEFAULT_GCC32_VERSION=4.9
DEFAULT_GCC64_VERSION=4.9
FIRST_GCC32_VERSION=4.9
FIRST_GCC64_VERSION=4.9
DEFAULT_LLVM_GCC32_VERSION=4.9
DEFAULT_LLVM_GCC64_VERSION=4.9

DEFAULT_BINUTILS_VERSION=2.27
DEFAULT_GDB_VERSION=7.11
DEFAULT_MPFR_VERSION=3.1.1
DEFAULT_GMP_VERSION=5.0.5
DEFAULT_MPC_VERSION=1.0.1
DEFAULT_CLOOG_VERSION=0.18.0
DEFAULT_ISL_VERSION=0.11.1
DEFAULT_PPL_VERSION=1.0
DEFAULT_PYTHON_VERSION=2.7.5
DEFAULT_PERL_VERSION=5.16.2

# Default platform to build target binaries against.
DEFAULT_PLATFORM=android-14

# The list of default CPU architectures we support
DEFAULT_ARCHS="arm x86 mips arm64 x86_64 mips64"

# Default toolchain names and prefix
#
# This is used by get_default_toolchain_name_for_arch and get_default_toolchain_prefix_for_arch
# defined below
DEFAULT_ARCH_TOOLCHAIN_NAME_arm=arm-linux-androideabi
DEFAULT_ARCH_TOOLCHAIN_PREFIX_arm=arm-linux-androideabi

DEFAULT_ARCH_TOOLCHAIN_NAME_arm64=aarch64-linux-android
DEFAULT_ARCH_TOOLCHAIN_PREFIX_arm64=aarch64-linux-android

DEFAULT_ARCH_TOOLCHAIN_NAME_x86=x86
DEFAULT_ARCH_TOOLCHAIN_PREFIX_x86=i686-linux-android

DEFAULT_ARCH_TOOLCHAIN_NAME_x86_64=x86_64
DEFAULT_ARCH_TOOLCHAIN_PREFIX_x86_64=x86_64-linux-android

DEFAULT_ARCH_TOOLCHAIN_NAME_mips=mips64el-linux-android
DEFAULT_ARCH_TOOLCHAIN_PREFIX_mips=mips64el-linux-android

DEFAULT_ARCH_TOOLCHAIN_NAME_mips64=mips64el-linux-android
DEFAULT_ARCH_TOOLCHAIN_PREFIX_mips64=mips64el-linux-android

# The build number of clang used to build pieces of the NDK (like platforms).
DEFAULT_LLVM_VERSION="2455903"

# The default URL to download the LLVM tar archive
DEFAULT_LLVM_URL="http://llvm.org/releases"

# The list of default host NDK systems we support
DEFAULT_SYSTEMS="linux-x86 windows darwin-x86"

# The default issue tracker URL
DEFAULT_ISSUE_TRACKER_URL="http://source.android.com/source/report-bugs.html"

# Return the default gcc version for a given architecture
# $1: Architecture name (e.g. 'arm')
# Out: default arch-specific gcc version
get_default_gcc_version_for_arch ()
{
    case $1 in
       *64) echo $DEFAULT_GCC64_VERSION ;;
       *) echo $DEFAULT_GCC32_VERSION ;;
    esac
}

# Return the first gcc version for a given architecture
# $1: Architecture name (e.g. 'arm')
# Out: default arch-specific gcc version
get_first_gcc_version_for_arch ()
{
    case $1 in
       *64) echo $FIRST_GCC64_VERSION ;;
       *) echo $FIRST_GCC32_VERSION ;;
    esac
}

# Return default NDK ABI for a given architecture name
# $1: Architecture name
# Out: ABI name
get_default_abi_for_arch ()
{
    local RET
    case $1 in
        arm)
            RET="armeabi"
            ;;
        arm64)
            RET="arm64-v8a"
            ;;
        x86|x86_64|mips|mips64)
            RET="$1"
            ;;
        mips32r6)
            RET="mips"
            ;;
        *)
            2> echo "ERROR: Unsupported architecture name: $1, use one of: arm arm64 x86 x86_64 mips mips64"
            exit 1
            ;;
    esac
    echo "$RET"
}


# Retrieve the list of default ABIs supported by a given architecture
# $1: Architecture name
# Out: space-separated list of ABI names
get_default_abis_for_arch ()
{
    local RET
    case $1 in
        arm)
            RET="armeabi armeabi-v7a"
            ;;
        arm64)
            RET="arm64-v8a"
            ;;
        x86|x86_64|mips|mips32r6|mips64)
            RET="$1"
            ;;
        *)
            2> echo "ERROR: Unsupported architecture name: $1, use one of: arm arm64 x86 x86_64 mips mips64"
            exit 1
            ;;
    esac
    echo "$RET"
}

# Return toolchain name for given architecture and GCC version
# $1: Architecture name (e.g. 'arm')
# $2: optional, GCC version (e.g. '4.8')
# Out: default arch-specific toolchain name (e.g. 'arm-linux-androideabi-$GCC_VERSION')
# Return empty for unknown arch
get_toolchain_name_for_arch ()
{
    if [ ! -z "$2" ] ; then
        eval echo \"\${DEFAULT_ARCH_TOOLCHAIN_NAME_$1}-$2\"
    else
        eval echo \"\${DEFAULT_ARCH_TOOLCHAIN_NAME_$1}\"
    fi
}

# Return the default toolchain name for a given architecture
# $1: Architecture name (e.g. 'arm')
# Out: default arch-specific toolchain name (e.g. 'arm-linux-androideabi-$GCCVER')
# Return empty for unknown arch
get_default_toolchain_name_for_arch ()
{
    local GCCVER=$(get_default_gcc_version_for_arch $1)
    eval echo \"\${DEFAULT_ARCH_TOOLCHAIN_NAME_$1}-$GCCVER\"
}

# Return the default toolchain program prefix for a given architecture
# $1: Architecture name
# Out: default arch-specific toolchain prefix (e.g. arm-linux-androideabi)
# Return empty for unknown arch
get_default_toolchain_prefix_for_arch ()
{
    eval echo "\$DEFAULT_ARCH_TOOLCHAIN_PREFIX_$1"
}

# Get the list of all toolchain names for a given architecture
# $1: architecture (e.g. 'arm')
# $2: comma separated versions (optional)
# Out: list of toolchain names for this arch (e.g. arm-linux-androideabi-4.8 arm-linux-androideabi-4.9)
# Return empty for unknown arch
get_toolchain_name_list_for_arch ()
{
    local PREFIX VERSION RET ADD FIRST_GCC_VERSION VERSIONS
    PREFIX=$(eval echo \"\$DEFAULT_ARCH_TOOLCHAIN_NAME_$1\")
    if [ -z "$PREFIX" ]; then
        return 0
    fi
    RET=""
    FIRST_GCC_VERSION=$(get_first_gcc_version_for_arch $1)
    ADD=""
    VERSIONS=$(commas_to_spaces $2)
    if [ -z "$VERSIONS" ]; then
        VERSIONS=$DEFAULT_GCC_VERSION_LIST
    else
        ADD="yes" # include everything we passed explicitly
    fi
    for VERSION in $VERSIONS; do
        if [ -z "$ADD" -a "$VERSION" = "$FIRST_GCC_VERSION" ]; then
            ADD="yes"
        fi
        if [ -z "$ADD" ]; then
            continue
        fi
        RET=$RET" $PREFIX-$VERSION"
    done
    RET=${RET## }
    echo "$RET"
}

# Return the binutils version to be used by default when
# building a given version of GCC. This is needed to ensure
# we use binutils-2.19 when building gcc-4.4.3 for ARM and x86,
# and later binutils in other cases (mips, or gcc-4.6+).
#
# Note that technically, we could use latest binutils for all versions of
# GCC, however, in NDK r7, we did build GCC 4.4.3 with binutils-2.20.1
# and this resulted in weird C++ debugging bugs. For NDK r7b and higher,
# binutils was reverted to 2.19, to ensure at least
# feature/bug compatibility.
#
# $1: toolchain with version number (e.g. 'arm-linux-androideabi-4.8')
#
get_default_binutils_version_for_gcc ()
{
    echo "$DEFAULT_BINUTILS_VERSION"
}

# Return the binutils version to be used by default when
# building a given version of llvm. For llvm-3.4 or later,
# we use binutils-2.23+ to ensure the LLVMgold.so could be
# built properly. For llvm-3.3, we use binutils-2.21 as default.
#
# $1: toolchain with version numer (e.g. 'llvm-3.3')
#
get_default_binutils_version_for_llvm ()
{
    echo "$DEFAULT_BINUTILS_VERSION"
}

# Return the gdb version to be used by default when building a given
# version of GCC.
#
# $1: toolchain with version number (e.g. 'arm-linux-androideabi-4.8')
#
get_default_gdb_version_for_gcc ()
{
    echo "$DEFAULT_GDB_VERSION"
}

# Return the gdbserver version to be used by default when building a given
# version of GCC.
#
# $1: toolchain with version number (e.g. 'arm-linux-androideabi-4.8')
#
get_default_gdbserver_version ()
{
    echo "$DEFAULT_GDB_VERSION"
}
