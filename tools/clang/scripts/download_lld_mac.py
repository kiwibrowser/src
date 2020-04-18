#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script to download lld/mac from google storage."""

import os
import re
import subprocess
import sys

import update

LLVM_BUILD_DIR = update.LLVM_BUILD_DIR
LLD_LINK_PATH = os.path.join(LLVM_BUILD_DIR, 'bin', 'lld-link')


def AlreadyUpToDate():
  if not os.path.exists(LLD_LINK_PATH):
    return False
  lld_rev = subprocess.check_output([LLD_LINK_PATH, '--version'])
  return (re.match(r'LLD.*\(trunk (\d+)\)', lld_rev).group(1) ==
             update.CLANG_REVISION)


def main():
  if AlreadyUpToDate():
    return 0
  remote_path = '%s/Mac/lld-%s.tgz' % (update.CDS_URL, update.PACKAGE_VERSION)
  update.DownloadAndUnpack(remote_path, update.LLVM_BUILD_DIR)
  return 0


if __name__ == '__main__':
  sys.exit(main())
