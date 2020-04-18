#!/bin/bash
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

cd "$(dirname $0)"/..
readonly base=$PWD

readonly bitsplatform=$1

if [ "$bitsplatform" != 32 ] && [ "$bitsplatform" != 64 ]; then
  echo -n "Usage: $0 <32|64>"
  exit 1;
fi

readonly proxy_bin="$base"/glibc-tests/bin
readonly bld_old="$base"/BUILD/build-glibc"$bitsplatform"
readonly bld="$base"/glibc-tests/build-glibc"$bitsplatform"
readonly base_timestamp="$bld"/base-timestamp
readonly logfile=/tmp/$(basename $0).$$.log

cat "$base"/glibc-tests/exclude_list.txt | egrep -v '^#|^[[:space:]]*$' | \
  while read file; do
    echo $file EXCLUDED_IN_NACL
  done

echo -n "Copying the glibc build directory... "
rm -rf "$bld"
/bin/cp -a "$bld_old" "$base"/glibc-tests/
echo done

touch "$base_timestamp"
cat "$base"/glibc-tests/exclude_list.txt | egrep -v '^#|^[[:space:]]*$' | \
  while read file; do
    touch -r "$base_timestamp" "$bld/$file"{.o,,.os,.so,.out}
  done

# Run the tests using 'make -k' to be able to see all failures.
# Set the pipe to return last failed exit code since by default 'tee' overwrites
# the exit code.
set -o pipefail
make -k -C "$base"/SRC/glibc check \
  LDFLAGS=-B"$proxy_bin" objdir="$bld" \
  run-program-prefix="$proxy_bin"/nacl_tester.sh 2>&1 |
    tee "$logfile"
ret=$?
set +o pipefail

if egrep ' FAIL$|Error' "$logfile" > "$logfile".fail; then
  echo "Error: Unexpected test failures:" 1>&2
  ret=1
  cat "$logfile".fail 1>&2
elif [[ "$ret" != "0" ]]; then
  echo "Error: make command failed" 1>&2
else
  echo "All tests passed successfully!" 1>&2
fi

rm -f "$logfile" "$logfile".fail
exit $ret
