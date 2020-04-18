#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script to download llvm-objdump and related utils from google storage."""

import os
import re
import subprocess
import sys
import urllib2

import update

LLVM_BUILD_DIR = update.LLVM_BUILD_DIR
OBJDUMP_PATH = os.path.join(LLVM_BUILD_DIR, 'bin', 'llvm-objdump')
STAMP_FILE = os.path.normpath(
    os.path.join(LLVM_BUILD_DIR, 'llvmobjdump_build_revision'))


def AlreadyUpToDate():
  if not os.path.exists(OBJDUMP_PATH) or not os.path.exists(STAMP_FILE):
    return False
  stamp = update.ReadStampFile(STAMP_FILE)
  return stamp.rstrip() == update.PACKAGE_VERSION


def DownloadAndUnpackLlvmObjDumpPackage(platform):
  cds_file = 'llvmobjdump-%s.tgz' % update.PACKAGE_VERSION
  cds_full_url = update.GetPlatformUrlPrefix(platform) + cds_file
  try:
    update.DownloadAndUnpack(cds_full_url, update.LLVM_BUILD_DIR)
  except urllib2.URLError:
    print 'Failed to download prebuilt utils %s' % cds_file
    print 'Use --force-local-build if you want to build locally.'
    print 'Exiting.'
    sys.exit(1)


def main():
  if not AlreadyUpToDate():
    DownloadAndUnpackLlvmObjDumpPackage(sys.platform)
  return 0

if __name__ == '__main__':
  sys.exit(main())
