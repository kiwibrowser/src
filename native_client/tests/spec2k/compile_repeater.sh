#!/bin/bash

# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

repetitions=$1
target_name=$2
shift 2

echo "Repeating compile and timing ${REPETITIONS} times"
time_file="${target_name}.compile_time"
rm -f "${time_file}"
for ((i=0; i<${repetitions}; i++)); do
  /usr/bin/time -f "%U %S %e %C" --append -o "${time_file}" "$@"
done
