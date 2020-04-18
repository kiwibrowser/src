#!/bin/bash

# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

ASSETS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CHROME_APP_DIR="$(cd "${ASSETS_DIR}/.." && pwd)"
IMAGES_DIR="$(cd "${CHROME_APP_DIR}/template/images" && pwd)"

SRC_FILES=("icon.svg" "icon.svg" "icon.svg" "body.svg" "cog1.svg" "cog2.svg")
DST_FILES=( \
  "icon_16.png" "icon_48.png" "icon_128.png" "body.png" "cog1.png" "cog2.png")
DST_SIZES=(16 48 128 300 60 30)

# Generate bitmap images.
for i in "${!SRC_FILES[@]}"; do
  src_file="${ASSETS_DIR}/${SRC_FILES[$i]}"
  dst_file="${IMAGES_DIR}/${DST_FILES[$i]}"
  size="${DST_SIZES[$i]}"
  echo "Generating ${dst_file}"
  inkscape -z -e "${dst_file}" -w "${size}" -h "${size}" "${src_file}" > \
    /dev/null
done

