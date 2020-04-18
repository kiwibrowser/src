# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import os.path
import shutil
import subprocess
import sys

# Usage: python copy_keystone_framework.py /path/to/input /path/to/output
#        [DEVELOPER_DIR]
#
# This script copies the KeystoneRegistration.framework, removing its
# versioned directory structure, thinning it to just x86_64, and deleting
# the Headers directory.

def Main(args):
  if len(args) != 3 and len(args) != 4:
    print >> sys.stderr, '%s: /path/to/input /path/to/output' % (args[0],)
    return 1
  if len(args) == 4:
    os.environ['DEVELOPER_DIR'] = args[3]

  # Delete any old copies of the framework.
  output_path = os.path.join(args[2], 'KeystoneRegistration.framework')
  if os.path.exists(output_path):
    shutil.rmtree(output_path)

  # Copy the framework out of its versioned directory. Use rsync to exclude
  # dotfiles and the Headers directories.
  subprocess.check_call(
      ['rsync', '-acC', '--delete',
       '--exclude', 'Headers', '--exclude', 'PrivateHeaders',
       '--include', '*.so',
       os.path.join(args[1], 'Versions/Current/'),
       output_path])

  # Thin the library to just the required arch.
  library_path = os.path.join(output_path, 'KeystoneRegistration')
  subprocess.check_call(
      ['lipo', '-thin', 'x86_64', library_path, '-o', library_path])
  return 0


if __name__ == '__main__':
  sys.exit(Main(sys.argv))
