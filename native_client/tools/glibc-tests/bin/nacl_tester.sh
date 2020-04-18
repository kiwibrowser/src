#!/bin/bash
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

xfail_list="$(dirname "$0")"/../xfail_list.txt

if [[ $# < 1 ]]; then
  echo "usage: $0 /full/path/to/test args" 1>&2
  exit 121
fi

# Kills all background processes that we created.
function cleanup() {
  if [[ "$tpid" != "0" ]]; then
    kill "$tpid"
  fi
  if [[ "$wpid" != "0" ]]; then
    kill -INT "$wpid"
  fi
}

# Handles ^C.
function interrupted() {
  echo "Interrupted: exiting" 1>&2
  cleanup
  exit 122
}
trap interrupted SIGINT

# Handles test timeout.
function timeout() {
  echo "$@" " (TIMEOUT) FAIL" 1>&2
  wpid=0
  cleanup
  exit 123
}
trap timeout SIGUSR2

tpid=0
wpid=0

# Run the test in background.
"$@" &
tpid=$!

# Check for timeout in background, handle in timeout().
# note: 10 seconds is not enough for rt/tst-clock to finish.
(sleep 15; kill -USR2 $$ 2>/dev/null) &
wpid=$!

# Wait for the test to finish, collect exitcode.
wait $tpid
ret=$?
tpid=0

cleanup
readonly prog="$1"

# Take two last components of the full path as test name.
readonly tst=${prog#${prog%/*/*}/}

# Find if the test is expectd to fail.
LANG=C grep -x "$tst" "$xfail_list" >/dev/null 2>&1
readonly must_pass=$?

if [[ "$ret" = "0" ]]; then
  if [[ "$must_pass" != "0" ]]; then
    echo "$@" PASS 1>&2
    exit 0
  fi
  # Expected to fail, but passed.  It is a soft failure.
  echo "$@" XPASS 1>&2
  exit 120
elif [[ "$ret" = "115" ]]; then
  # The test was excluded.
  exit 0
else
  if [[ "$must_pass" = "0" ]]; then
    # Expected to fail and failed.  Good.
    echo "$@" XFAIL 1>&2
    exit 0
  fi

  echo "$@" FAIL 1>&2
  exit $ret
fi
