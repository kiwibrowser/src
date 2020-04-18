#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A wrapper script for using pylint from the command line."""

import os
import subprocess
import sys


_HERE = os.path.dirname(os.path.abspath(__file__))
_PYLINT = os.path.join(_HERE, 'third_party', 'pylint.py')
_RC_FILE = os.path.join(_HERE, 'pylintrc')


# Run pylint. We prepend the command-line with the depot_tools rcfile. If
# another rcfile is to be used, passing --rcfile a second time on the command-
# line will work fine.
command = [sys.executable, _PYLINT]
if os.path.isfile(_RC_FILE):
  # The file can be removed to test 'normal' pylint behavior.
  command.append('--rcfile=%s' % _RC_FILE)
command.extend(sys.argv[1:])
try:
  sys.exit(subprocess.call(command))
except KeyboardInterrupt:
  sys.stderr.write('interrupted\n')
  sys.exit(1)
