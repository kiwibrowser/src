#!/bin/sh
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

dirs=$(find components-chromium -type d | grep -v 'components-chromium/polymer')

for dir in $dirs; do
  htmls=$(\ls $dir/*.html 2>/dev/null)
  if [ "$htmls" ]; then
    echo "Analyzing $dir"
    gyp_file="$dir/compiled_resources2.gyp"
    content=$(../../../tools/polymer/generate_compiled_resources_gyp.py $htmls)
    if [ "$content" ]; then
      echo "Writing $gyp_file"
      echo "$content" > "$gyp_file"
    elif [ -f "$gyp_file" ]; then
      echo "Removing $gyp_file"
      rm "$gyp_file"
    fi
    echo
  fi
done
