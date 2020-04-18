#!/usr/bin/env sh

exit_with_usage ()
{
    echo "Usage: $0 [ignored.py] --prefix|--exec-prefix|--includes|--libs|--cflags|--ldflags|--extension-suffix|--help|--abiflags|--configdir"
    exit 1
}

case "$1" in
    *.py)
        shift
    ;;
esac

if [ "$1" = "" ] ; then
    exit_with_usage
fi

# Returns the actual prefix where this script was installed to.
installed_prefix ()
{
    local RESULT=$(dirname $(cd $(dirname "$1") && pwd -P))
    local READLINK=readlink
    if [ "$(uname -s)" = "Darwin" ] ; then
        # readlink in darwin can't handle -f.  Use greadlink from MacPorts instead.
        READLINK=greadlink
    fi
    if [ $(which $READLINK) ] ; then
        RESULT=$($READLINK -f "$RESULT")
    fi
    echo $RESULT
}

prefix_build="/buildbot/tmp/buildhost/linux-x86_64/install/host-tools"
prefix_real=$(installed_prefix "$0")

exec_prefix_build="${prefix}"
exec_prefix_real="$prefix_real"

# Use sed to fix paths from their built to locations to their installed to locations.

# The ${prefix}/include and ${exec_prefix}/lib macros can be '$prefix/include' and the like, so we
# need to avoid replacing the prefix multiple times.
prefix="$prefix_build"
exec_prefix="$exec_prefix_build"

includedir=$(echo "${prefix}/include" | sed "s#^$prefix_build#$prefix_real#")
libdir=$(echo "${exec_prefix}/lib" | sed "s#^$prefix_build#$prefix_real#")

prefix="$prefix_real"
exec_prefix="$exec_prefix_real"

CFLAGS="-O2 -Os -fomit-frame-pointer -s"
VERSION="2.7"
LIBM="-lm"
LIBC=""
SYSLIBS="$LIBM $LIBC"
ABIFLAGS="@ABIFLAGS@"
# Protect against lack of substitution.
if [ "$ABIFLAGS" = "@ABIFLAGS@" ] ; then
    ABIFLAGS=
fi
LIBS="-lpython${VERSION}${ABIFLAGS} -lpthread -ldl  -lutil $SYSLIBS"
BASECFLAGS=" -fno-strict-aliasing"
LDLIBRARY="libpython${VERSION}.a"
LINKFORSHARED="-Xlinker -export-dynamic"
OPT="-DNDEBUG -fwrapv -O3 -Wall -Wstrict-prototypes"
PY_ENABLE_SHARED="0"
DLLLIBRARY=""
LIBDEST=${prefix}/lib/python${VERSION}
LIBPL=${LIBDEST}/config
SO=".so"
PYTHONFRAMEWORK=""
INCDIR="-I$includedir/python${VERSION}${ABIFLAGS}"
PLATINCDIR="-I$includedir/python${VERSION}${ABIFLAGS}"

# Scan for --help or unknown argument.
for ARG in $*
do
    case $ARG in
        --help)
            exit_with_usage
        ;;
        --prefix|--exec-prefix|--includes|--libs|--cflags|--ldflags)
        ;;
        *)
            exit_with_usage
        ;;
    esac
done

for ARG in $*
do
    case $ARG in
        --prefix)
            echo "$prefix"
        ;;
        --exec-prefix)
            echo "$exec_prefix"
        ;;
        --includes)
            echo "$INCDIR"
        ;;
        --cflags)
            echo "$INCDIR $BASECFLAGS $CFLAGS $OPT"
        ;;
        --libs)
            echo "$LIBS"
        ;;
        --ldflags)
            LINKFORSHAREDUSED=
            if [ -z "$PYTHONFRAMEWORK" ] ; then
                LINKFORSHAREDUSED=$LINKFORSHARED
            fi
            LIBPLUSED=
            if [ "$PY_ENABLE_SHARED" = "0" -o -n "${DLLLIBRARY}" ] ; then
                LIBPLUSED="-L$LIBPL"
            fi
            echo "$LIBPLUSED -L$libdir $LIBS $LINKFORSHAREDUSED"
        ;;
esac
done
