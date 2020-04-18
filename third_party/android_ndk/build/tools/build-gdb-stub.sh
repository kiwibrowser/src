#!/bin/bash

NDK_BUILDTOOLS_PATH="$(dirname $0)"
. "$NDK_BUILDTOOLS_PATH/prebuilt-common.sh"
. "$NDK_BUILDTOOLS_PATH/common-build-host-funcs.sh"

PROGRAM_PARAMETERS=""
PROGRAM_DESCRIPTION="\
This program is used to build the gdb stub for Windows and replace a
gdb executable with it. Because of the replacing nature of this, I
check to see if there's a gdb-orig.exe there already and if so, 'undo'
the process first by putting it back. Sample usage:

$0 --gdb-executable-path=\$NDK/toolchains/arm-linux-androideabi-4.8/prebuilt/bin/arm-linux-androideabi-gdb.exe \\
   --python-prefix-dir=\$NDK/prebuilt/windows \\
   --mingw-w64-gcc-path=\$HOME/i686-w64-mingw32/bin/i686-w64-mingw32-gcc
"

NDK_DIR=$ANDROID_NDK_ROOT

PYTHON_PREFIX_DIR=
register_var_option "--python-prefix-dir=<path>" PYTHON_PREFIX_DIR "Python prefix directory."

GDB_EXECUTABLE_PATH=
register_var_option "--gdb-executable-path=<path>" GDB_EXECUTABLE_PATH "GDB executable file to stubify."

MINGW_W64_GCC=
register_var_option "--mingw-w64-gcc=<program>" MINGW_W64_GCC "MinGW-w64 gcc program to use."

DEBUG_STUB=
register_var_option "--debug" DEBUG_STUB "Build stub in debug mode."

extract_parameters "$@"

if [ -n "$DEBUG_STUB" ]; then
    STUB_CFLAGS="-O0 -g -D_DEBUG"
else
    STUB_CFLAGS="-O2 -s -DNDEBUG"
fi

if [ ! -f "$GDB_EXECUTABLE_PATH" ]; then
    panic "GDB executable $GDB_EXECUTABLE_PATH doesn't exist!"
fi

if [ ! -d "$PYTHON_PREFIX_DIR" ]; then
    panic "Python prefix dir $PYTHON_PREFIX_DIR doesn't exist!"
fi

if [ -z "$MINGW_W64_GCC" ]; then
    panic "Please specify an existing MinGW-w64 cross compiler with --mingw-w64-gcc=<program>"
fi

GDB_BIN_DIR=$(cd $(dirname "$GDB_EXECUTABLE_PATH"); pwd)
PYTHON_PREFIX_DIR=$(cd "$PYTHON_PREFIX_DIR"; pwd)
PYTHON_BIN_DIR=${PYTHON_PREFIX_DIR}/bin

# Sed is used to get doubled up Windows style dir separators.
GDB_TO_PYTHON_REL_DIR=$(relpath "$GDB_BIN_DIR" "$PYTHON_BIN_DIR" | sed -e s./.\\\\\\\\.g)
PYTHONHOME_REL_DIR=$(relpath "$GDB_BIN_DIR" "$PYTHON_PREFIX_DIR" | sed -e s./.\\\\\\\\.g)
dump "GDB_TO_PYTHON_REL_DIR=$GDB_TO_PYTHON_REL_DIR"
dump "PYTHONHOME_REL_DIR=$PYTHONHOME_REL_DIR"

GDB_EXECUTABLE_ORIG=$(dirname "$GDB_EXECUTABLE_PATH")/$(basename "$GDB_EXECUTABLE_PATH" ".exe")-orig.exe
if [ -f "$GDB_EXECUTABLE_ORIG" ] ; then
    echo "Warning : Found an existing gdb-stub called $GDB_EXECUTABLE_ORIG so will un-do a previous run of this script."
    cp "$GDB_EXECUTABLE_ORIG" "$GDB_EXECUTABLE_PATH"
fi
cp "$GDB_EXECUTABLE_PATH" "$GDB_EXECUTABLE_ORIG"

GDB_EXECUTABLE_ORIG_FILENAME=$(basename "$GDB_EXECUTABLE_ORIG")

# Build the stub in-place of the real gdb.
run $MINGW_W64_GCC $STUB_CFLAGS "$NDK_DIR/sources/host-tools/gdb-stub/gdb-stub.c" \
                    -o "$GDB_EXECUTABLE_PATH" \
                    -DGDB_TO_PYTHON_REL_DIR=\"$GDB_TO_PYTHON_REL_DIR\" \
                    -DPYTHONHOME_REL_DIR=\"$PYTHONHOME_REL_DIR\" \
                    -DGDB_EXECUTABLE_ORIG_FILENAME=\"$GDB_EXECUTABLE_ORIG_FILENAME\"

fail_panic "Can't build gdb stub!"

dump "GDB Stub done!"
