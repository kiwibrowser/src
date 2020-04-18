#!/bin/bash
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

readonly E_GENERAL=1

error() {
  (
  # Red log line.
  tput setaf 1
  echo "ERROR: $1"
  tput sgr0
  ) >&2
}

warning() {
  (
  # Yellow warning line.
  tput setaf 3
  echo "WARNING: $1"
  tput sgr0
  ) >&2
}

info() {
  echo "INFO: $1"
}
