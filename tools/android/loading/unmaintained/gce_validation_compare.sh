#!/bin/bash
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# gce_validation_compare.sh rootdir compare_filename
#   root_dir: root directory for the experiment.
#   compare_filename: file where the comparison breakdown is output.
#
# Computes core sets from GCE and device experiment resutls, and compare them.
# The expected directory structure is:
#
# root_dir/
#   cloud/
#     url1/         # Can be any name as long as it is mirrored under device/.
#       run1.trace  # Can be any name.
#       run2.trace
#       ...
#     url2/
#     ...
#   device/
#     url1/
#       run1.trace
#       run2.trace
#       ...
#     url2/
#     ...

root_dir=$1
compare_filename=$2

rm $compare_filename

# Check directory structure.
if [ ! -d $root_dir/cloud ]; then
  echo "$root_dir/cloud missing!"
  exit 1
fi

if [ ! -d $root_dir/device ]; then
  echo "$root_dir/device missing!"
  exit 1
fi

for device_file in $root_dir/device/*/  ; do
  cloud_file=$root_dir/cloud/$(basename $device_file)
  if [ ! -d $cloud_file ]; then
    echo "$cloud_file not found"
  fi
done

for cloud_file in $root_dir/cloud/*/  ; do
  device_file=$root_dir/device/$(basename $device_file)
  if [ ! -d $device_file ]; then
    echo "$device_file not found"
  fi
done

# Loop through all the subdirectories, compute the core sets and compare them.
for device_file in $root_dir/device/*/  ; do
  base_name=$(basename $device_file)
  python tools/android/loading/core_set.py page_core --sets device/$base_name \
    --output $device_file/core_set.json --prefix $device_file

  cloud_file=$root_dir/cloud/$base_name
  if [ -d $cloud_file ]; then
    python tools/android/loading/core_set.py page_core --sets cloud/$base_name \
      --output $cloud_file/core_set.json --prefix $cloud_file

    compare_result=$(python tools/android/loading/core_set.py compare \
      --a $cloud_file/core_set.json --b $device_file/core_set.json)
    compare_result+=" $base_name"
    echo $compare_result >> $compare_filename
  fi
done
