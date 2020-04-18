#!/bin/sh
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

dirs=$(find components-chromium -type d | grep -v 'components-chromium/polymer')

gn_files=''

for dir in $dirs; do
  htmls=$(\ls $dir/*.html 2>/dev/null)
  if [ "$htmls" ]; then
    echo "Analyzing $dir"
    gn_file="$dir/BUILD.gn"
    content=$(../../../tools/polymer/generate_gn.py $htmls)
    if [ "$content" ]; then
      echo "Writing $gn_file"
      echo "$content" > "$gn_file"
      gn_files="$gn_files $gn_file"
    elif [ -f "$gn_file" ]; then
      echo "Removing $gn_file"
      rm "$gn_file"
    fi
    echo
  fi
done

git cl format $gn_files
