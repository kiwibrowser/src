# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import optparse
import os
import shutil
import sys


def CleanUpOldVersions(path_to_app, keep_version, stamp_path):
  versions_dir = os.path.join(path_to_app, 'Contents', 'Versions')
  if os.path.exists(versions_dir):
    for version in os.listdir(versions_dir):
      if version != keep_version:
        shutil.rmtree(os.path.join(versions_dir, version))

  open(stamp_path, 'w').close()
  os.utime(stamp_path, None)


def Main():
  parser = optparse.OptionParser(
      usage='%prog path/to/Chromium.app keep_version stamp_path')
  opts, args = parser.parse_args()
  if len(args) != 3:
    parser.error('missing arguments')
    return 1

  CleanUpOldVersions(*args)
  return 0

if __name__ == '__main__':
  sys.exit(Main())
