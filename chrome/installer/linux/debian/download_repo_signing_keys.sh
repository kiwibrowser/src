#!/bin/bash
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -o nounset
set -o errexit

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

KEY_FINGERS=(
  # Debian Archive Automatic Signing Key (7.0/wheezy)
  "A1BD8E9D78F7FE5C3E65D8AF8B48AD6246925553"
  # Debian Archive Automatic Signing Key (8/jessie)
  "126C0D24BD8A2942CC7DF8AC7638D0442B90D010"
  # Debian Security Archive Automatic Signing Key (8/jessie)
  "D21169141CECD440F2EB8DDA9D6D8F6BC857C906"
  # Jessie Stable Release Key
  "75DDC3C4A499F1A18CB5F3C8CBF8D6FD518E17E1"
  # Debian Stable Release Key (9/stretch)
  "067E3C456BAE240ACEE88F6FEF0F382A1A7B6500"
  # Ubuntu Archive Automatic Signing Key
  "630239CC130E1A7FD81A27B140976EAF437D05B5"
  # Ubuntu Archive Automatic Signing Key (2012)
  "790BC7277767219C42C86F933B4FE6ACC0B21F32"
)

gpg --keyserver pgp.mit.edu --recv-keys ${KEY_FINGERS[@]}
gpg --output "${SCRIPT_DIR}/repo_signing_keys.gpg" --export ${KEY_FINGERS[@]}
