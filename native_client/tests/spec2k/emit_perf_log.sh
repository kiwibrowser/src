#!/bin/bash

# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -o nounset
set -o errexit

#@ LogRealTime <time_file> <graph_label> <bench> <compiler_setup>
#@   Take wall time data from <time_file>, and log it for the Chrome perf bots.
#@   <time_file> should be a single line where column 1 and 2 are user/sys.
LogRealTime() {
  local time_file=$1
  local graph_label=$2
  local bench=$3
  local setup=$4
  # Generate a list of times "[x,y,z]". The chromium perf log parser
  # will know to average this list of times.
  local times="[$(awk '{print $3}' ${time_file} | \
                   tr '\n' ',' | sed 's/,$//')]"
  LogPerf ${graph_label} ${bench} ${setup} "${times}" "seconds"
}

#@ LogGzippedSize <file_to_zip> <graph_label> <bench> <compiler_setup>
#@   Measure and log size of gzipped executable/bc files/etc.
LogGzippedSize() {
  local file_to_zip=$1
  local graph_label=$2
  local bench=$3
  local setup=$4
  local tempsize=`gzip ${file_to_zip} -c | wc -c`
  LogPerf ${graph_label} ${bench} ${setup} ${tempsize} "bytes"
}

#@ Emit a chrome perf log datapoint
#@  $1 :: graph_type
#@  $2 :: bench_name
#@  $3 :: compiler setup
#@  $4 :: measurement value
#@  $5 :: unit
LogPerf() {
  echo "RESULT $1_$2: $3= $4 $5"
}

######################################################################
# Main
######################################################################

# Print the usage message to stdout.
Usage() {
  egrep "^#@" $0 | cut --bytes=3-
}

[ $# = 0 ] && set -- help  # Avoid reference to undefined $1.

if [ "$(type -t $1)" != "function" ]; then
  Usage
  echo "ERROR: unknown mode '$1'." >&2
  exit 1
fi

eval "$@"
