#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This script is a wrapper around the GN binary that is pulled from Google
Cloud Storage when you sync Chrome. The binaries go into platform-specific
subdirectories in the source tree.

This script makes there be one place for forwarding to the correct platform's
binary. It will also automatically try to find the gn binary when run inside
the chrome source tree, so users can just type "gn" on the command line
(normally depot_tools is on the path)."""

import gclient_utils
import os
import subprocess
import sys


def main(args):
  bin_path = gclient_utils.GetBuildtoolsPlatformBinaryPath()
  if not bin_path:
    print >> sys.stderr, ('gn.py: Could not find checkout in any parent of '
                          'the current path.\nThis must be run inside a '
                          'checkout.')
    return 1
  gn_path = os.path.join(bin_path, 'gn' + gclient_utils.GetExeSuffix())
  if not os.path.exists(gn_path):
    print >> sys.stderr, 'gn.py: Could not find gn executable at: %s' % gn_path
    return 2
  else:
    return subprocess.call([gn_path] + args[1:])


if __name__ == '__main__':
  try:
    sys.exit(main(sys.argv))
  except KeyboardInterrupt:
    sys.stderr.write('interrupted\n')
    sys.exit(1)
