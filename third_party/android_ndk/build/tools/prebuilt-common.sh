# Common functions for all prebuilt-related scripts
# This is included/sourced by other scripts
#

# ensure stable sort order
export LC_ALL=C

# NDK_BUILDTOOLS_PATH should point to the directory containing
# this script. If it is not defined, assume that this is one of
# the scripts in the same directory that sourced this file.
#
if [ -z "$NDK_BUILDTOOLS_PATH" ]; then
    NDK_BUILDTOOLS_PATH=$(dirname $0)
    if [ ! -f "$NDK_BUILDTOOLS_PATH/prebuilt-common.sh" ]; then
        echo "INTERNAL ERROR: Please define NDK_BUILDTOOLS_PATH to point to \$NDK/build/tools"
        exit 1
    fi
fi

# Warn if /bin/sh isn't bash.
if [ -z "$BASH_VERSION" ] ; then
    echo "WARNING: The shell running this script isn't bash.  Although we try to avoid bashism in scripts, things can happen."
fi

NDK_BUILDTOOLS_ABSPATH=$(cd $NDK_BUILDTOOLS_PATH && pwd)

. $NDK_BUILDTOOLS_PATH/ndk-common.sh
. $NDK_BUILDTOOLS_PATH/dev-defaults.sh


# Given an input string of the form <foo>-<bar>-<version>, where
# <version> can be <major>.<minor>, extract <major>
extract_version ()
{
    echo $1 | tr '-' '\n' | tail -1
}

# $1: versioned name (e.g. arm-linux-androideabi-4.8)
# Out: major version (e.g. 4)
#
# Examples:  arm-linux-androideabi-4.4.3 -> 4
#            gmp-0.81 -> 0
#
extract_major_version ()
{
    local RET=$(extract_version $1 | cut -d . -f 1)
    RET=${RET:-0}
    echo $RET
}

# Same as extract_major_version, but for the minor version number
# $1: versioned named
# Out: minor version
#
extract_minor_version ()
{
    local RET=$(extract_version $1 | cut -d . -f 2)
    RET=${RET:-0}
    echo $RET
}

# Compare two version numbers and only succeeds if the first one is
# greater than or equal to the second one.
#
# $1: first version (e.g. 4.9)
# $2: second version (e.g. 4.8)
#
# Example: version_is_at_least 4.9 4.8 --> success
#
version_is_at_least ()
{
    local A_MAJOR A_MINOR B_MAJOR B_MINOR
    A_MAJOR=$(extract_major_version $1)
    B_MAJOR=$(extract_major_version $2)

    if [ $A_MAJOR -lt $B_MAJOR ]; then
        return 1
    elif [ $A_MAJOR -gt $B_MAJOR ]; then
        return 0
    fi

    # We have A_MAJOR == B_MAJOR here

    A_MINOR=$(extract_minor_version $1)
    B_MINOR=$(extract_minor_version $2)

    if [ $A_MINOR -lt $B_MINOR ]; then
        return 1
    else
        return 0
    fi
}

#====================================================
#
#  UTILITY FUNCTIONS
#
#====================================================

# Return the maximum length of a series of strings
#
# Usage:  len=`max_length <string1> <string2> ...`
#
max_length ()
{
    echo "$@" | tr ' ' '\n' | awk 'BEGIN {max=0} {len=length($1); if (len > max) max=len} END {print max}'
}

# Translate dashes to underscores
# Usage:  str=`dashes_to_underscores <values>`
dashes_to_underscores ()
{
    echo "$@" | tr '-' '_'
}

# Translate underscores to dashes
# Usage: str=`underscores_to_dashes <values>`
underscores_to_dashes ()
{
    echo "$@" | tr '_' '-'
}

# Translate commas to spaces
# Usage: str=`commas_to_spaces <list>`
commas_to_spaces ()
{
    echo "$@" | tr ',' ' '
}

# Translate spaces to commas
# Usage: list=`spaces_to_commas <string>`
spaces_to_commas ()
{
    echo "$@" | tr ' ' ','
}

# Remove trailing path of a path
# $1: path
remove_trailing_slash () {
    echo ${1%%/}
}

# Reverse a file path directory
# foo -> .
# foo/bar -> ..
# foo/bar/zoo -> ../..
reverse_path ()
{
    local path cur item
    path=${1%%/} # remove trailing slash
    cur="."
    if [ "$path" != "." ] ; then
        for item in $(echo "$path" | tr '/' ' '); do
            cur="../$cur"
        done
    fi
    echo ${cur%%/.}
}

# test_reverse_path ()
# {
#     rr=`reverse_path $1`
#     if [ "$rr" != "$2" ] ; then
#         echo "ERROR: reverse_path '$1' -> '$rr' (expected '$2')"
#     fi
# }
#
# test_reverse_path . .
# test_reverse_path ./ .
# test_reverse_path foo ..
# test_reverse_path foo/ ..
# test_reverse_path foo/bar ../..
# test_reverse_path foo/bar/ ../..
# test_reverse_path foo/bar/zoo ../../..
# test_reverse_path foo/bar/zoo/ ../../..

# Sort a space-separated list and remove duplicates
# $1+: slist
# Output: new slist
sort_uniq ()
{
    local RET
    RET=$(echo "$@" | tr ' ' '\n' | sort -u)
    echo $RET
}

# Return the list of all regular files under a given directory
# $1: Directory path
# Output: list of files, relative to $1
list_files_under ()
{
    if [ -d "$1" ]; then
        (cd $1 && find . -type f | sed -e "s!./!!" | sort -u)
    else
        echo ""
    fi
}

# Returns all words in text that do not match any of the pattern
# $1: pattern
# $2: text
filter_out ()
{
    local PATTERN="$1"
    local TEXT="$2"
    for pat in $PATTERN; do
        pat=$"${pat//\//\\/}"
        TEXT=$(echo $TEXT | sed -e 's/'$pat' //g' -e 's/'$pat'$//g')
    done
    echo $TEXT
}

# Assign a value to a variable
# $1: Variable name
# $2: Value
var_assign ()
{
    eval $1=\"$2\"
}

#====================================================
#
#  OPTION PROCESSING
#
#====================================================

# We recognize the following option formats:
#
#  -f
#  --flag
#
#  -s<value>
#  --setting=<value>
#

# NOTE: We translate '-' into '_' when storing the options in global variables
#

OPTIONS=""
OPTION_FLAGS=""
OPTION_SETTINGS=""

# Set a given option attribute
# $1: option name
# $2: option attribute
# $3: attribute value
#
option_set_attr ()
{
    eval OPTIONS_$1_$2=\"$3\"
}

# Get a given option attribute
# $1: option name
# $2: option attribute
#
option_get_attr ()
{
    echo `var_value OPTIONS_$1_$2`
}

# Register a new option
# $1: option
# $2: small abstract for the option
# $3: optional. default value
#
register_option_internal ()
{
    optlabel=
    optname=
    optvalue=
    opttype=
    while [ -n "1" ] ; do
        # Check for something like --setting=<value>
        echo "$1" | grep -q -E -e '^--[^=]+=<.+>$'
        if [ $? = 0 ] ; then
            optlabel=`expr -- "$1" : '\(--[^=]*\)=.*'`
            optvalue=`expr -- "$1" : '--[^=]*=\(<.*>\)'`
            opttype="long_setting"
            break
        fi

        # Check for something like --flag
        echo "$1" | grep -q -E -e '^--[^=]+$'
        if [ $? = 0 ] ; then
            optlabel="$1"
            opttype="long_flag"
            break
        fi

        # Check for something like -f<value>
        echo "$1" | grep -q -E -e '^-[A-Za-z0-9]<.+>$'
        if [ $? = 0 ] ; then
            optlabel=`expr -- "$1" : '\(-.\).*'`
            optvalue=`expr -- "$1" : '-.\(<.+>\)'`
            opttype="short_setting"
            break
        fi

        # Check for something like -f
        echo "$1" | grep -q -E -e '^-.$'
        if [ $? = 0 ] ; then
            optlabel="$1"
            opttype="short_flag"
            break
        fi

        echo "ERROR: Invalid option format: $1"
        echo "       Check register_option call"
        exit 1
    done

    log "new option: type='$opttype' name='$optlabel' value='$optvalue'"

    optname=`dashes_to_underscores $optlabel`
    OPTIONS="$OPTIONS $optname"
    OPTIONS_TEXT="$OPTIONS_TEXT $1"
    option_set_attr $optname label "$optlabel"
    option_set_attr $optname otype "$opttype"
    option_set_attr $optname value "$optvalue"
    option_set_attr $optname text "$1"
    option_set_attr $optname abstract "$2"
    option_set_attr $optname default "$3"
}

# Register a new option with a function callback.
#
# $1: option
# $2: name of function that will be called when the option is parsed
# $3: small abstract for the option
# $4: optional. default value
#
register_option ()
{
    local optname optvalue opttype optlabel
    register_option_internal "$1" "$3" "$4"
    option_set_attr $optname funcname "$2"
}

# Register a new option with a variable store
#
# $1: option
# $2: name of variable that will be set by this option
# $3: small abstract for the option
#
# NOTE: The current value of $2 is used as the default
#
register_var_option ()
{
    local optname optvalue opttype optlabel
    register_option_internal "$1" "$3" "`var_value $2`"
    option_set_attr $optname varname "$2"
}


MINGW=no
DARWIN=no
do_mingw_option ()
{
    if [ "$DARWIN" = "yes" ]; then
        echo "Can not have both --mingw and --darwin"
        exit 1
    fi
    MINGW=yes;
}
do_darwin_option ()
{
    if [ "$MINGW" = "yes" ]; then
        echo "Can not have both --mingw and --darwin"
        exit 1
    fi
    DARWIN=yes; 
}

register_canadian_option ()
{
    if [ "$HOST_OS" = "linux" ] ; then
        register_option "--mingw" do_mingw_option "Generate windows binaries on Linux."
        register_option "--darwin" do_darwin_option "Generate darwin binaries on Linux."
    fi
}

TRY64=no
do_try64_option () { TRY64=yes; }

register_try64_option ()
{
    register_option "--try-64" do_try64_option "Generate 64-bit only binaries."
}


register_jobs_option ()
{
    NUM_JOBS=$BUILD_NUM_CPUS
    register_var_option "-j<number>" NUM_JOBS "Use <number> parallel build jobs"
}

# Print the help, including a list of registered options for this program
# Note: Assumes PROGRAM_PARAMETERS and PROGRAM_DESCRIPTION exist and
#       correspond to the parameters list and the program description
#
print_help ()
{
    local opt text abstract default

    echo "Usage: $PROGNAME [options] $PROGRAM_PARAMETERS"
    echo ""
    if [ -n "$PROGRAM_DESCRIPTION" ] ; then
        echo "$PROGRAM_DESCRIPTION"
        echo ""
    fi
    echo "Valid options (defaults are in brackets):"
    echo ""

    maxw=`max_length "$OPTIONS_TEXT"`
    AWK_SCRIPT=`echo "{ printf \"%-${maxw}s\", \\$1 }"`
    for opt in $OPTIONS; do
        text=`option_get_attr $opt text | awk "$AWK_SCRIPT"`
        abstract=`option_get_attr $opt abstract`
        default=`option_get_attr $opt default`
        if [ -n "$default" ] ; then
            echo "  $text     $abstract [$default]"
        else
            echo "  $text     $abstract"
        fi
    done
    echo ""
}

option_panic_no_args ()
{
    echo "ERROR: Option '$1' does not take arguments. See --help for usage."
    exit 1
}

option_panic_missing_arg ()
{
    echo "ERROR: Option '$1' requires an argument. See --help for usage."
    exit 1
}

extract_parameters ()
{
    local opt optname otype value name fin funcname
    PARAMETERS=""
    while [ -n "$1" ] ; do
        # If the parameter does not begin with a dash
        # it is not an option.
        param=`expr -- "$1" : '^\([^\-].*\)$'`
        if [ -n "$param" ] ; then
            if [ -z "$PARAMETERS" ] ; then
                PARAMETERS="$1"
            else
                PARAMETERS="$PARAMETERS $1"
            fi
            shift
            continue
        fi

        while [ -n "1" ] ; do
            # Try to match a long setting, i.e. --option=value
            opt=`expr -- "$1" : '^\(--[^=]*\)=.*$'`
            if [ -n "$opt" ] ; then
                otype="long_setting"
                value=`expr -- "$1" : '^--[^=]*=\(.*\)$'`
                break
            fi

            # Try to match a long flag, i.e. --option
            opt=`expr -- "$1" : '^\(--.*\)$'`
            if [ -n "$opt" ] ; then
                otype="long_flag"
                value="yes"
                break
            fi

            # Try to match a short setting, i.e. -o<value>
            opt=`expr -- "$1" : '^\(-[A-Za-z0-9]\)..*$'`
            if [ -n "$opt" ] ; then
                otype="short_setting"
                value=`expr -- "$1" : '^-.\(.*\)$'`
                break
            fi

            # Try to match a short flag, i.e. -o
            opt=`expr -- "$1" : '^\(-.\)$'`
            if [ -n "$opt" ] ; then
                otype="short_flag"
                value="yes"
                break
            fi

            echo "ERROR: Unknown option '$1'. Use --help for list of valid values."
            exit 1
        done

        #echo "Found opt='$opt' otype='$otype' value='$value'"

        name=`dashes_to_underscores $opt`
        found=0
        for xopt in $OPTIONS; do
            if [ "$name" != "$xopt" ] ; then
                continue
            fi
            # Check that the type is correct here
            #
            # This also allows us to handle -o <value> as -o<value>
            #
            xotype=`option_get_attr $name otype`
            if [ "$otype" != "$xotype" ] ; then
                case "$xotype" in
                "short_flag")
                    option_panic_no_args $opt
                    ;;
                "short_setting")
                    if [ -z "$2" ] ; then
                        option_panic_missing_arg $opt
                    fi
                    value="$2"
                    shift
                    ;;
                "long_flag")
                    option_panic_no_args $opt
                    ;;
                "long_setting")
                    option_panic_missing_arg $opt
                    ;;
                esac
            fi
            found=1
            break
            break
        done
        if [ "$found" = "0" ] ; then
            echo "ERROR: Unknown option '$opt'. See --help for usage."
            exit 1
        fi
        # Set variable or launch option-specific function.
        varname=`option_get_attr $name varname`
        if [ -n "$varname" ] ; then
            eval ${varname}=\"$value\"
        else
            eval `option_get_attr $name funcname` \"$value\"
        fi
        shift
    done
}

do_option_help ()
{
    print_help
    exit 0
}

VERBOSE=no
do_option_verbose ()
{
    VERBOSE=yes
}

DRYRUN=no
do_option_dryrun ()
{
    DRYRUN=yes
}

register_option "--help"          do_option_help     "Print this help."
register_option "--verbose"       do_option_verbose  "Enable verbose mode."
register_option "--dryrun"        do_option_dryrun   "Set to dryrun mode."

#====================================================
#
#  TOOLCHAIN AND ABI PROCESSING
#
#====================================================

# Determine optional variable value
# $1: final variable name
# $2: option variable name
# $3: small description for the option
fix_option ()
{
    if [ -n "$2" ] ; then
        eval $1="$2"
        log "Using specific $3: $2"
    else
        log "Using default $3: `var_value $1`"
    fi
}


# If SYSROOT is empty, check that $1/$2 contains a sysroot
# and set the variable to it.
#
# $1: sysroot path
# $2: platform/arch suffix
check_sysroot ()
{
    if [ -z "$SYSROOT" ] ; then
        log "Probing directory for sysroot: $1/$2"
        if [ -d $1/$2 ] ; then
            SYSROOT=$1/$2
        fi
    fi
}

# Determine sysroot
# $1: Option value (or empty)
#
fix_sysroot ()
{
    if [ -n "$1" ] ; then
        eval SYSROOT="$1"
        log "Using specified sysroot: $1"
    else
        SYSROOT_SUFFIX=$PLATFORM/arch-$ARCH
        SYSROOT=
        check_sysroot $ANDROID_BUILD_TOP/prebuilts/ndk/current/platforms $SYSROOT_SUFFIX
        check_sysroot $ANDROID_NDK_ROOT/platforms $SYSROOT_SUFFIX
        check_sysroot `dirname $ANDROID_NDK_ROOT`/development/ndk/platforms $SYSROOT_SUFFIX

        if [ -z "$SYSROOT" ] ; then
            echo "ERROR: Could not find NDK sysroot path for $SYSROOT_SUFFIX."
            echo "       Use --sysroot=<path> to specify one."
            exit 1
        fi
    fi

    if [ ! -f $SYSROOT/usr/include/stdlib.h ] ; then
        echo "ERROR: Invalid sysroot path: $SYSROOT"
        echo "       Use --sysroot=<path> to indicate a valid one."
        exit 1
    fi
}

# Check for the availability of a compatibility SDK in Darwin
# this can be used to generate binaries compatible with either Tiger or
# Leopard.
#
# $1: SDK root path
# $2: Optional MacOS X minimum version (e.g. 10.5)
DARWIN_MINVER=10.6
check_darwin_sdk ()
{
    local MACSDK="$1"
    local MINVER=$2

    if [ -z "$MINVER" ] ; then
        # expect SDK root path ended up with either MacOSX##.#.sdk or MacOSX##.#u.sdk
        MINVER=${MACSDK##*MacOSX}
        MINVER=${MINVER%%.sdk*}
        if [ "$MINVER" = "10.4u" ]; then
            MINVER=10.4
        fi
    fi
    if [ -d "$MACSDK" ] ; then
        HOST_CFLAGS=$HOST_CFLAGS" -isysroot $MACSDK -mmacosx-version-min=$MINVER -DMAXOSX_DEPLOYEMENT_TARGET=$MINVER"
        HOST_LDFLAGS=$HOST_LDFLAGS" -Wl,-syslibroot,$MACSDK -mmacosx-version-min=$MINVER"
        DARWIN_MINVER=$MINVER
        return 0  # success
    fi
    return 1
}

# Probe Darwin SDK in specified diectory $DARWIN_SYSROOT, or
# /Developer/SDKs/MacOSX10.6.sdk
#
probe_darwin_sdk ()
{
    if [ -n "$DARWIN_SYSROOT" ]; then
        if check_darwin_sdk "$DARWIN_SYSROOT"; then
            log "Use darwin sysroot $DARWIN_SYSROOT"
        else
            echo "darwin sysroot $DARWIN_SYSROOT is not valid"
            exit 1
        fi
    elif check_darwin_sdk /Developer/SDKs/MacOSX10.6.sdk 10.6; then
        log "Generating Snow Leopard-compatible binaries!"
    else
        local version=`sw_vers -productVersion`
        log "Generating $version-compatible binaries!"
    fi
}

handle_canadian_build ()
{
    HOST_EXE=
    if [ "$MINGW" = "yes" -o "$DARWIN" = "yes" ] ; then
        case $HOST_TAG in
            linux-*)
                ;;
            *)
                echo "ERROR: Can only enable --mingw or --darwin on Linux platforms !"
                exit 1
                ;;
        esac
        if [ "$MINGW" = "yes" ] ; then
            # NOTE: Use x86_64-pc-mingw32msvc or i586-pc-mingw32msvc because wrappers are generated
            #       using these names
            if [ "$TRY64" = "yes" ]; then
                ABI_CONFIGURE_HOST=x86_64-pc-mingw32msvc
                HOST_TAG=windows-x86_64
            else
                ABI_CONFIGURE_HOST=i586-pc-mingw32msvc
                HOST_TAG=windows
            fi
            HOST_OS=windows
            HOST_EXE=.exe
        else
            if [ "$TRY64" = "yes" ]; then
                ABI_CONFIGURE_HOST=x86_64-apple-darwin
                HOST_TAG=darwin-x86_64
            else
                ABI_CONFIGURE_HOST=i686-apple-darwin
                HOST_TAG=darwin-x86
            fi
            HOST_OS=darwin
        fi
    fi
}

# Find mingw toolchain
#
# Set MINGW_GCC to the found mingw toolchain
#
find_mingw_toolchain ()
{
    if [ "$DEBIAN_NAME" -a "$BINPREFIX" -a "$MINGW_GCC" ]; then
        return
    fi
    # IMPORTANT NOTE: binutils 2.21 requires a cross toolchain named
    # i585-pc-mingw32msvc-gcc, or it will fail its configure step late
    # in the toolchain build. Note that binutils 2.19 can build properly
    # with i585-mingw32mvsc-gcc, which is the name used by the 'mingw32'
    # toolchain install on Debian/Ubuntu.
    #
    # To solve this dilemma, we create a wrapper toolchain named
    # i586-pc-mingw32msvc-gcc that really calls i586-mingw32msvc-gcc,
    # this works with all versions of binutils.
    #
    # We apply the same logic to the 64-bit Windows cross-toolchain
    #
    # Fedora note: On Fedora it's x86_64-w64-mingw32- or i686-w64-mingw32-
    # On older Fedora it's 32-bit only and called i686-pc-mingw32-
    # so we just add more prefixes to the list to check.
    if [ "$HOST_ARCH" = "x86_64" -a "$TRY64" = "yes" ]; then
        BINPREFIX=x86_64-pc-mingw32msvc-
        BINPREFIXLST="x86_64-pc-mingw32msvc- x86_64-w64-mingw32- amd64-mingw32msvc-"
        DEBIAN_NAME=mingw-w64
    else
        # we are trying 32 bit anyway, so forcing it to avoid build issues
        force_32bit_binaries
        BINPREFIX=i586-pc-mingw32msvc-
        BINPREFIXLST="i586-pc-mingw32msvc- i686-pc-mingw32- i586-mingw32msvc- i686-w64-mingw32-"
        DEBIAN_NAME=mingw-w64
    fi

    # Scan $BINPREFIXLST list to find installed mingw toolchain. It will be
    # wrapped later with $BINPREFIX.
    for i in $BINPREFIXLST; do
        find_program MINGW_GCC ${i}gcc
        if [ -n "$MINGW_GCC" ]; then
            dump "Found mingw toolchain: $MINGW_GCC"
            break
        fi
    done
}

# Check there is a working cross-toolchain installed.
#
# $1: install directory for mingw/darwin wrapper toolchain
#
prepare_canadian_toolchain ()
{
    if [ "$MINGW" != "yes" -a "$DARWIN" != "yes" ]; then
        return
    fi
    CROSS_GCC=
    if [ "$MINGW" = "yes" ]; then
        find_mingw_toolchain
        if [ -z "$MINGW_GCC" ]; then
            echo "ERROR: Could not find in your PATH any of:"
            for i in $BINPREFIXLST; do echo "   ${i}gcc"; done
            echo "Please install the corresponding cross-toolchain and re-run this script"
            echo "TIP: On Debian or Ubuntu, try: sudo apt-get install $DEBIAN_NAME"
            exit 1
        fi
        CROSS_GCC=$MINGW_GCC
    else
        if [ -z "$DARWIN_TOOLCHAIN" ]; then
            echo "Please set DARWIN_TOOLCHAIN to darwin cross-toolchain"
            exit 1
        fi
        if [ ! -f "${DARWIN_TOOLCHAIN}-gcc" ]; then
            echo "darwin cross-toolchain $DARWIN_TOOLCHAIN-gcc doesn't exist"
            exit 1
        fi
        if [ "$HOST_ARCH" = "x86_64" -a "$TRY64" = "yes" ]; then
            BINPREFIX=x86_64-apple-darwin-
            DEBIAN_NAME=darwin64
            HOST_CFLAGS=$HOST_CFLAGS" -m64"
        else
            force_32bit_binaries
            BINPREFIX=i686-apple-darwin-
            DEBIAN_NAME=darwin32
            HOST_CFLAGS=$HOST_CFLAGS" -m32"
        fi
        CROSS_GCC=${DARWIN_TOOLCHAIN}-gcc
        probe_darwin_sdk
    fi

    # Create a wrapper toolchain, and prepend its dir to our PATH
    CROSS_WRAP_DIR="$1"/$DEBIAN_NAME-wrapper
    rm -rf "$CROSS_WRAP_DIR"
    mkdir -p "$CROSS_WRAP_DIR"

    if [ "$DARWIN" = "yes" ] ; then
        cat > "$CROSS_WRAP_DIR/sw_vers" <<EOF
#!/bin/sh
# Tiny utility for the real sw_vers some Makefiles need
case \$1 in
    -productVersion)
        echo $DARWIN_MINVER
        ;;
    *)
        echo "ERROR: Unknown switch \$1"
        exit 1
esac
EOF
    chmod 0755 "$CROSS_WRAP_DIR/sw_vers"
    fi

    DST_PREFIX=${CROSS_GCC%gcc}
    if [ "$NDK_CCACHE" ]; then
        DST_PREFIX="$NDK_CCACHE $DST_PREFIX"
    fi
    $NDK_BUILDTOOLS_PATH/gen-toolchain-wrapper.sh --src-prefix=$BINPREFIX --dst-prefix="$DST_PREFIX" "$CROSS_WRAP_DIR" \
        --cflags="$HOST_CFLAGS" --cxxflags="$HOST_CFLAGS" --ldflags="$HOST_LDFLAGS"
    # generate wrappers for BUILD toolchain
    # this is required for mingw/darwin build to avoid tools canadian cross configuration issues
    # 32-bit BUILD toolchain
    LEGACY_TOOLCHAIN_DIR="$ANDROID_BUILD_TOP/prebuilts/gcc/linux-x86/host/x86_64-linux-glibc2.11-4.8"
    $NDK_BUILDTOOLS_PATH/gen-toolchain-wrapper.sh --src-prefix=i386-linux-gnu- \
            --cflags="-m32" --cxxflags="-m32" --ldflags="-m elf_i386" --asflags="--32" \
            --dst-prefix="$LEGACY_TOOLCHAIN_DIR/bin/x86_64-linux-" "$CROSS_WRAP_DIR"
    $NDK_BUILDTOOLS_PATH/gen-toolchain-wrapper.sh --src-prefix=i386-pc-linux-gnu- \
            --cflags="-m32" --cxxflags="-m32" --ldflags="-m elf_i386" --asflags="--32" \
            --dst-prefix="$LEGACY_TOOLCHAIN_DIR/bin/x86_64-linux-" "$CROSS_WRAP_DIR"
    # 64-bit BUILD toolchain.  libbfd is still built in 32-bit.
    $NDK_BUILDTOOLS_PATH/gen-toolchain-wrapper.sh --src-prefix=x86_64-linux-gnu- \
            --dst-prefix="$LEGACY_TOOLCHAIN_DIR/bin/x86_64-linux-" "$CROSS_WRAP_DIR"
    $NDK_BUILDTOOLS_PATH/gen-toolchain-wrapper.sh --src-prefix=x86_64-pc-linux-gnu- \
            --dst-prefix="$LEGACY_TOOLCHAIN_DIR/bin/x86_64-linux-" "$CROSS_WRAP_DIR"
    fail_panic "Could not create $DEBIAN_NAME wrapper toolchain in $CROSS_WRAP_DIR"

    export PATH=$CROSS_WRAP_DIR:$PATH
    dump "Using $DEBIAN_NAME wrapper: $CROSS_WRAP_DIR/${BINPREFIX}gcc"
}

handle_host ()
{
    if [ "$TRY64" != "yes" ]; then
        force_32bit_binaries  # to modify HOST_TAG and others
        HOST_BITS=32
    fi
    handle_canadian_build
}

setup_ccache ()
{
    # Support for ccache compilation
    # We can't use this here when building Windows/darwin binaries on Linux with
    # binutils 2.21, because defining CC/CXX in the environment makes the
    # configure script fail later
    #
    if [ "$NDK_CCACHE" -a "$MINGW" != "yes" -a "$DARWIN" != "yes" ]; then
        NDK_CCACHE_CC=$CC
        NDK_CCACHE_CXX=$CXX
        # Unfortunately, we can just do CC="$NDK_CCACHE $CC" because some
        # configure scripts are not capable of dealing with this properly
        # E.g. the ones used to rebuild the GCC toolchain from scratch.
        # So instead, use a wrapper script
        CC=$NDK_BUILDTOOLS_ABSPATH/ndk-ccache-gcc.sh
        CXX=$NDK_BUILDTOOLS_ABSPATH/ndk-ccache-g++.sh
        export NDK_CCACHE_CC NDK_CCACHE_CXX
        log "Using ccache compilation"
        log "NDK_CCACHE_CC=$NDK_CCACHE_CC"
        log "NDK_CCACHE_CXX=$NDK_CCACHE_CXX"
    fi
}

prepare_common_build ()
{
    if [ "$MINGW" = "yes" -o "$DARWIN" = "yes" ]; then
        if [ "$TRY64" = "yes" ]; then
            HOST_BITS=64
        else
            HOST_BITS=32
        fi
        if [ "$MINGW" = "yes" ]; then
            log "Generating $HOST_BITS-bit Windows binaries"
        else
            log "Generating $HOST_BITS-bit Darwin binaries"
        fi
        # Do *not* set CC and CXX when building the Windows/Darwin binaries in canadian build.
        # Otherwise, the GCC configure/build script will mess that Canadian cross
        # build in weird ways. Instead we rely on the toolchain detected or generated
        # previously in prepare_canadian_toolchain.
        unset CC CXX
        return
    fi

    # On Linux, detect our legacy-compatible toolchain when in the Android
    # source tree, and use it to force the generation of glibc-2.7 compatible
    # binaries.
    #
    # We only do this if the CC variable is not defined to a given value
    if [ -z "$CC" ]; then
        LEGACY_TOOLCHAIN_DIR=
        if [ "$HOST_OS" = "linux" ]; then
            LEGACY_TOOLCHAIN_DIR="$ANDROID_BUILD_TOP/prebuilts/gcc/linux-x86/host/x86_64-linux-glibc2.11-4.8/bin"
            LEGACY_TOOLCHAIN_PREFIX="$LEGACY_TOOLCHAIN_DIR/x86_64-linux-"
        elif [ "$HOST_OS" = "darwin" ]; then
            LEGACY_TOOLCHAIN_DIR="$ANDROID_BUILD_TOP/prebuilts/gcc/darwin-x86/host/i686-apple-darwin-4.2.1/bin"
            LEGACY_TOOLCHAIN_PREFIX="$LEGACY_TOOLCHAIN_DIR/i686-apple-darwin10-"
        fi
        if [ -d "$LEGACY_TOOLCHAIN_DIR" ] ; then
            log "Forcing generation of $HOST_OS binaries with legacy toolchain"
            CC="${LEGACY_TOOLCHAIN_PREFIX}gcc"
            CXX="${LEGACY_TOOLCHAIN_PREFIX}g++"
        fi
    fi

    CC=${CC:-gcc}
    CXX=${CXX:-g++}
    STRIP=${STRIP:-strip}
    case $HOST_TAG in
        darwin-*)
            probe_darwin_sdk
            ;;
    esac

    # Force generation of 32-bit binaries on 64-bit systems.
    # We used to test the value of $HOST_TAG for *-x86_64, but this is
    # not sufficient on certain systems.
    #
    # For example, Snow Leopard can be booted with a 32-bit kernel, running
    # a 64-bit userland, with a compiler that generates 64-bit binaries by
    # default *even* though "gcc -v" will report --target=i686-apple-darwin10!
    #
    # So know, simply probe for the size of void* by performing a small runtime
    # compilation test.
    #
    cat > $TMPC <<EOF
    /* this test should fail if the compiler generates 64-bit machine code */
    int test_array[1-2*(sizeof(void*) != 4)];
EOF
    log_n "Checking whether the compiler generates 32-bit binaries..."
    log $CC $HOST_CFLAGS -c -o $TMPO $TMPC
    $NDK_CCACHE $CC $HOST_CFLAGS -c -o $TMPO $TMPC >$TMPL 2>&1
    if [ $? != 0 ] ; then
        log "no"
        if [ "$TRY64" != "yes" ]; then
            # NOTE: We need to modify the definitions of CC and CXX directly
            #        here. Just changing the value of CFLAGS / HOST_CFLAGS
            #        will not work well with the GCC toolchain scripts.
            CC="$CC -m32"
            CXX="$CXX -m32"
        fi
    else
        log "yes"
        if [ "$TRY64" = "yes" ]; then
            CC="$CC -m64"
            CXX="$CXX -m64"
        fi
    fi

    if [ "$TRY64" = "yes" ]; then
        HOST_BITS=64
    else
        force_32bit_binaries  # to modify HOST_TAG and others
        HOST_BITS=32
    fi
}

prepare_host_build ()
{
    prepare_common_build

    # Now deal with mingw or darwin
    if [ "$MINGW" = "yes" -o "$DARWIN" = "yes" ]; then
        handle_canadian_build
        CC=$ABI_CONFIGURE_HOST-gcc
        CXX=$ABI_CONFIGURE_HOST-g++
        CPP=$ABI_CONFIGURE_HOST-cpp
        LD=$ABI_CONFIGURE_HOST-ld
        AR=$ABI_CONFIGURE_HOST-ar
        AS=$ABI_CONFIGURE_HOST-as
        RANLIB=$ABI_CONFIGURE_HOST-ranlib
        STRIP=$ABI_CONFIGURE_HOST-strip
        export CC CXX CPP LD AR AS RANLIB STRIP
    fi

    setup_ccache
}

prepare_abi_configure_build ()
{
    # detect build tag
    case $HOST_TAG in
        linux-x86)
            ABI_CONFIGURE_BUILD=i386-linux-gnu
            ;;
        linux-x86_64)
            ABI_CONFIGURE_BUILD=x86_64-linux-gnu
            ;;
        darwin-x86)
            ABI_CONFIGURE_BUILD=i686-apple-darwin
            ;;
        darwin-x86_64)
            ABI_CONFIGURE_BUILD=x86_64-apple-darwin
            ;;
        windows)
            ABI_CONFIGURE_BUILD=i686-pc-cygwin
            ;;
        *)
            echo "ERROR: Unsupported HOST_TAG: $HOST_TAG"
            echo "Please update 'prepare_host_flags' in build/tools/prebuilt-common.sh"
            ;;
    esac
}

prepare_target_build ()
{
    prepare_abi_configure_build

    # By default, assume host == build
    ABI_CONFIGURE_HOST="$ABI_CONFIGURE_BUILD"

    prepare_common_build
    HOST_GMP_ABI=$HOST_BITS

    # Now handle the --mingw/--darwin flag
    if [ "$MINGW" = "yes" -o "$DARWIN" = "yes" ] ; then
        handle_canadian_build
        STRIP=$ABI_CONFIGURE_HOST-strip
        if [ "$MINGW" = "yes" ] ; then
            # It turns out that we need to undefine this to be able to
            # perform a canadian-cross build with mingw. Otherwise, the
            # GMP configure scripts will not be called with the right options
            HOST_GMP_ABI=
        fi
    fi

    setup_ccache
}

# $1: Toolchain name
#
parse_toolchain_name ()
{
    TOOLCHAIN=$1
    if [ -z "$TOOLCHAIN" ] ; then
        echo "ERROR: Missing toolchain name!"
        exit 1
    fi

    ABI_CFLAGS_FOR_TARGET=
    ABI_CXXFLAGS_FOR_TARGET=

    # Determine ABI based on toolchain name
    #
    case "$TOOLCHAIN" in
    arm-linux-androideabi-*)
        ARCH="arm"
        ABI="armeabi"
        ABI_CONFIGURE_TARGET="arm-linux-androideabi"
        ABI_CONFIGURE_EXTRA_FLAGS="--with-arch=armv5te"
        ;;
    arm-eabi-*)
        ARCH="arm"
        ABI="armeabi"
        ABI_CONFIGURE_TARGET="arm-eabi"
        ABI_CONFIGURE_EXTRA_FLAGS="--with-arch=armv5te --disable-gold --disable-libgomp"
        ;;
    aarch64-linux-android-*)
        ARCH="arm64"
        ABI="arm64-v8a"
        ABI_CONFIGURE_TARGET="aarch64-linux-android"
        ;;
    x86-*)
        ARCH="x86"
        ABI=$ARCH
        ABI_INSTALL_NAME="x86"
        ABI_CONFIGURE_TARGET="i686-linux-android"
        # Enable C++ exceptions, RTTI and GNU libstdc++ at the same time
        # You can't really build these separately at the moment.
        ABI_CFLAGS_FOR_TARGET="-fPIC"
        ;;
    x86_64-*)
        ARCH="x86_64"
        ABI=$ARCH
        ABI_INSTALL_NAME="x86_64"
        ABI_CONFIGURE_TARGET="x86_64-linux-android"
        # Enable C++ exceptions, RTTI and GNU libstdc++ at the same time
        # You can't really build these separately at the moment.
        ABI_CFLAGS_FOR_TARGET="-fPIC"
        ;;
    mipsel*)
        ARCH="mips"
        ABI=$ARCH
        ABI_INSTALL_NAME="mips"
        ABI_CONFIGURE_TARGET="mipsel-linux-android"
        # Set default to mips32
        ABI_CONFIGURE_EXTRA_FLAGS="--with-arch=mips32"
        # Enable C++ exceptions, RTTI and GNU libstdc++ at the same time
        # You can't really build these separately at the moment.
        # Add -fpic, because MIPS NDK will need to link .a into .so.
        ABI_CFLAGS_FOR_TARGET="-fexceptions -fpic"
        ABI_CXXFLAGS_FOR_TARGET="-frtti -fpic"
        # Add --disable-fixed-point to disable fixed-point support
        ABI_CONFIGURE_EXTRA_FLAGS="$ABI_CONFIGURE_EXTRA_FLAGS --disable-fixed-point"
        ;;
    mips64el*)
        ARCH="mips64"
        ABI=$ARCH
        ABI_INSTALL_NAME="mips64"
        ABI_CONFIGURE_TARGET="mips64el-linux-android"
        # Set default to mips64r6
        ABI_CONFIGURE_EXTRA_FLAGS="--with-arch=mips64r6"
        # Enable C++ exceptions, RTTI and GNU libstdc++ at the same time
        # You can't really build these separately at the moment.
        # Add -fpic, because MIPS NDK will need to link .a into .so.
        ABI_CFLAGS_FOR_TARGET="-fexceptions -fpic"
        ABI_CXXFLAGS_FOR_TARGET="-frtti -fpic"
        # Add --disable-fixed-point to disable fixed-point support
        ABI_CONFIGURE_EXTRA_FLAGS="$ABI_CONFIGURE_EXTRA_FLAGS --disable-fixed-point"
        ;;
    * )
        echo "Invalid toolchain specified. Expected (arm-linux-androideabi-*|arm-eabi-*|x86-*|mipsel*|mips64el*)"
        echo ""
        print_help
        exit 1
        ;;
    esac

    log "Targetting CPU: $ARCH"

    GCC_VERSION=`expr -- "$TOOLCHAIN" : '.*-\([0-9x\.]*\)'`
    log "Using GCC version: $GCC_VERSION"
}

# Return the host "tag" used to identify prebuilt host binaries.
# NOTE: Handles the case where '$MINGW = true' or '$DARWIN = true'
# For now, valid values are: linux-x86, darwin-x86 and windows
get_prebuilt_host_tag ()
{
    local RET=$HOST_TAG
    if [ "$MINGW" = "yes" ]; then
        if [ "$TRY64" = "no" ]; then
            RET=windows
        else
            RET=windows-x86_64
        fi
    fi
    if [ "$DARWIN" = "yes" ]; then
        RET=darwin-x86_64  # let the following handles 32-bit case
    fi
    case $RET in
        linux-*)
            RET=linux-x86_64
            ;;
        darwin-*)
            RET=darwin-x86_64
            ;;
    esac
    echo $RET
}

# Return the executable suffix corresponding to host executables
get_prebuilt_host_exe_ext ()
{
    if [ "$MINGW" = "yes" ]; then
        echo ".exe"
    else
        echo ""
    fi
}

# Get library suffix for given ABI
# $1: ABI
# Return: .so or .bc
get_lib_suffix_for_abi ()
{
    local ABI=$1
    echo ".so"
}

# Convert an ABI name into an Architecture name
# $1: ABI name
# Result: Arch name
convert_abi_to_arch ()
{
    local RET
    local ABI=$1
    case $ABI in
        armeabi|armeabi-v7a)
            RET=arm
            ;;
        x86|mips|x86_64|mips64)
            RET=$ABI
            ;;
        mips32r6)
            RET=mips
            ;;
        arm64-v8a)
            RET=arm64
            ;;
        *)
            >&2 echo "ERROR: Unsupported ABI name: $ABI, use one of: armeabi, armeabi-v7a, x86, mips, arm64-v8a, x86_64 or mips64"
            exit 1
            ;;
    esac
    echo "$RET"
}

# Take architecture name as input, and output the list of corresponding ABIs
# Inverse for convert_abi_to_arch
# $1: ARCH name
# Out: ABI names list (comma-separated)
convert_arch_to_abi ()
{
    local RET
    local ARCH=$1
    case $ARCH in
        arm)
            RET=armeabi,armeabi-v7a
            ;;
        x86|x86_64|mips|mips64)
            RET=$ARCH
            ;;
        arm64)
            RET=arm64-v8a
            ;;
        *)
            >&2 echo "ERROR: Unsupported ARCH name: $ARCH, use one of: arm, x86, mips"
            exit 1
            ;;
    esac
    echo "$RET"
}

# Take a list of architecture names as input, and output the list of corresponding ABIs
# $1: ARCH names list (separated by spaces or commas)
# Out: ABI names list (comma-separated)
convert_archs_to_abis ()
{
    local RET
    for ARCH in $(commas_to_spaces $@); do
       ABI=$(convert_arch_to_abi $ARCH)
       if [ -n "$ABI" ]; then
          if [ -n "$RET" ]; then
             RET=$RET",$ABI"
          else
             RET=$ABI
          fi
       else   # Error message is printed by convert_arch_to_abi
          exit 1
       fi
    done
    echo "$RET"
}

# Return the default toolchain binary path prefix for given architecture and gcc version
# For example: arm 4.8 -> toolchains/<system>/arm-linux-androideabi-4.8/bin/arm-linux-androideabi-
# $1: Architecture name
# $2: GCC version
# $3: optional, system name, defaults to $HOST_TAG
get_toolchain_binprefix_for_arch ()
{
    local NAME PREFIX DIR BINPREFIX
    local SYSTEM=${3:-$(get_prebuilt_host_tag)}
    NAME=$(get_toolchain_name_for_arch $1 $2)
    PREFIX=$(get_default_toolchain_prefix_for_arch $1)
    DIR=$(get_toolchain_install . $NAME $SYSTEM)
    BINPREFIX=${DIR#./}/bin/$PREFIX-
    echo "$BINPREFIX"
}

# Return llvm toolchain binary path prefix for given llvm version
# $1: optional, system name, defaults to $HOST_TAG
get_llvm_toolchain_binprefix ()
{
    local NAME DIR BINPREFIX
    local SYSTEM=${1:-$(get_prebuilt_host_tag)}
    local VERSION=4053586
    SYSTEM=${SYSTEM%_64} # Trim _64 suffix. We only have one LLVM.
    BINPREFIX=$ANDROID_BUILD_TOP/prebuilts/clang/host/$SYSTEM/clang-$VERSION/bin
    echo "$BINPREFIX"
}

# Return default API level for a given arch
# This is the level used to build the toolchains.
#
# $1: Architecture name
get_default_api_level_for_arch ()
{
    # For now, always build the toolchain against API level 14 for 32-bit arch
    # and API level $FIRST_API64_LEVEL for 64-bit arch
    case $1 in
        *64) echo $FIRST_API64_LEVEL ;;
        *) echo 14 ;;
    esac
}

# Return the default platform sysroot corresponding to a given architecture
# This is the sysroot used to build the toolchain and other binaries like
# the STLport libraries.
# $1: Architecture name
get_default_platform_sysroot_for_arch ()
{
    local ARCH=$1
    local LEVEL=$(get_default_api_level_for_arch $ARCH)

    if [ "$ARCH" != "${ARCH%%64*}" ] ; then
        LEVEL=$FIRST_API64_LEVEL
    fi
    echo "platforms/android-$LEVEL/arch-$ARCH"
}

# Return the default platform sysroot corresponding to a given abi
# $1: ABI
get_default_platform_sysroot_for_abi ()
{
    local ARCH=$(convert_abi_to_arch $1)
    $(get_default_platform_sysroot_for_arch $ARCH)
}

# Return the default libs dir corresponding to a given architecture
# $1: Architecture name
get_default_libdir_for_arch ()
{
    case $1 in
      x86_64|mips64) echo "lib64" ;;
      arm64) echo "lib" ;; # return "lib" until aarch64 is built to look for sysroot/usr/lib64
      *) echo "lib" ;;
    esac
}

# Return the default libs dir corresponding to a given abi
# $1: ABI
get_default_libdir_for_abi ()
{
    local ARCH

    case $1 in
      mips32r6) echo "libr6" ;;
      *)
        local ARCH=$(convert_abi_to_arch $1)
        echo "$(get_default_libdir_for_arch $ARCH)"
        ;;
    esac
}

# Return the host/build specific path for prebuilt toolchain binaries
# relative to $1.
#
# $1: target root NDK directory
# $2: toolchain name
# $3: optional, host system name
#
get_toolchain_install ()
{
    local NDK="$1"
    shift
    echo "$NDK/$(get_toolchain_install_subdir "$@")"
}

# $1: toolchain name
# $2: optional, host system name
get_toolchain_install_subdir ()
{
    local SYSTEM=${2:-$(get_prebuilt_host_tag)}
    echo "toolchains/$SYSTEM/$1"
}

# Return the relative install prefix for prebuilt host
# executables (relative to the NDK top directory).
#
# Out: relative path to prebuilt install prefix
get_prebuilt_install_prefix ()
{
    echo "host-tools"
}

# Return the relative path of an installed prebuilt host
# executable.
#
# $1: executable name
# Out: path to prebuilt host executable, relative
get_prebuilt_host_exec ()
{
    local PREFIX EXE
    PREFIX=$(get_prebuilt_install_prefix)
    EXE=$(get_prebuilt_host_exe_ext)
    echo "$PREFIX/bin/$1$EXE"
}

# Return the name of a given host executable
# $1: executable base name
# Out: executable name, with optional suffix (e.g. .exe for windows)
get_host_exec_name ()
{
    local EXE=$(get_prebuilt_host_exe_ext)
    echo "$1$EXE"
}

# Return the directory where host-specific binaries are installed.
# $1: target root NDK directory
get_host_install ()
{
    echo "$1/$(get_prebuilt_install_prefix)"
}

# Set the toolchain target NDK location.
# this sets TOOLCHAIN_PATH and TOOLCHAIN_PREFIX
# $1: target NDK path
# $2: toolchain name
set_toolchain_ndk ()
{
    TOOLCHAIN_PATH=`get_toolchain_install "$1" $2`
    log "Using toolchain path: $TOOLCHAIN_PATH"

    TOOLCHAIN_PREFIX=$TOOLCHAIN_PATH/bin/$ABI_CONFIGURE_TARGET
    log "Using toolchain prefix: $TOOLCHAIN_PREFIX"
}

# Check that a toolchain is properly installed at a target NDK location
#
# $1: target root NDK directory
# $2: toolchain name
#
check_toolchain_install ()
{
    TOOLCHAIN_PATH=`get_toolchain_install "$1" $2`
    if [ ! -d "$TOOLCHAIN_PATH" ] ; then
        echo "ERROR: Cannot find directory '$TOOLCHAIN_PATH'!"
        echo "       Toolchain '$2' not installed in '$NDK_DIR'!"
        echo "       Ensure that the toolchain has been installed there before."
        exit 1
    fi

    set_toolchain_ndk $1 $2
}

# $1: toolchain source directory
check_toolchain_src_dir ()
{
    local SRC_DIR="$1"
    if [ -z "$SRC_DIR" ]; then
        echo "ERROR: Please provide the path to the toolchain source tree. See --help"
        exit 1
    fi

    if [ ! -d "$SRC_DIR" ]; then
        echo "ERROR: Not a directory: '$SRC_DIR'"
        exit 1
    fi

    if [ ! -f "$SRC_DIR/build/configure" -o ! -d "$SRC_DIR/gcc" ]; then
        echo "ERROR: Either the file $SRC_DIR/build/configure or"
        echo "       the directory $SRC_DIR/gcc does not exist."
        echo "This is not the top of a toolchain tree: $SRC_DIR"
        exit 1
    fi
}

make_repo_prop () {
    local OUT_PATH="$1/repo.prop"

    # The build server generates a repo.prop file that contains the current SHAs
    # of each project.
    if [ -f $DIST_DIR/repo.prop ]; then
        cp $DIST_DIR/repo.prop $OUT_PATH
    else
        # Generate our own if we're building locally.
        pushd $ANDROID_NDK_ROOT
        repo forall \
            -c 'echo $REPO_PROJECT $(git rev-parse HEAD)' > $OUT_PATH
        popd
    fi
}

#
# Define HOST_TAG32, as the 32-bit version of HOST_TAG
# We do this by replacing an -x86_64 suffix by -x86
HOST_TAG32=$HOST_TAG
case $HOST_TAG32 in
    *-x86_64)
        HOST_TAG32=${HOST_TAG%%_64}
        ;;
esac
