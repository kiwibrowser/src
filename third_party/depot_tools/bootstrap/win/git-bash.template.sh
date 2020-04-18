#!/usr/bin/env bash
export EDITOR=${EDITOR:=notepad}
WIN_BASE=`dirname $0`
UNIX_BASE=`cygpath "$WIN_BASE"`
export PATH="$PATH:$UNIX_BASE/${PYTHON_BIN_RELDIR_UNIX}:$UNIX_BASE/${PYTHON_BIN_RELDIR_UNIX}/Scripts"
export PYTHON_DIRECT=1
export PYTHONUNBUFFERED=1
if [[ $# > 0 ]]; then
  $UNIX_BASE/${GIT_BIN_RELDIR_UNIX}/bin/bash.exe "$@"
else
  $UNIX_BASE/${GIT_BIN_RELDIR_UNIX}/git-bash.exe &
fi
