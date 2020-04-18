#!/usr/bin/python
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os.path
import sys

# This is a replacement for the "scons" script in SCons.  The script that comes
# with the distribution searches for SCons in standard directories, but we want
# the one in the third-party directory.  Searching can screw us up if there's a
# version of SCons installed elsewhere.
# http://code.google.com/p/nativeclient/issues/detail?id=1928

root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
scons_dir = os.path.join(root, 'third_party', 'scons-2.0.1', 'engine')
sys.path.insert(0, scons_dir)

if __name__ == "__main__":
  import SCons.Script
  SCons.Script.main()
