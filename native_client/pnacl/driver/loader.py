# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os, sys

# Enforce Python version. This has to be done before importing driver_tools,
# which won't import with Python 2.5 and earlier
python_major_version = sys.version_info[:2]
if not python_major_version in ((2, 6), (2, 7)):
  print '''Python version 2.6 or 2.7 required!
The environment variable PNACLPYTHON can override the python found in PATH'''
  sys.exit(1)

import driver_tools

# This is called with:
# loader.py <toolname> <args>

pydir = os.path.dirname(os.path.abspath(sys.argv[0]))
bindir = os.path.dirname(pydir)

toolname = sys.argv[1]
extra_args = sys.argv[2:]

module = __import__(toolname)
argv = [os.path.join(bindir, toolname)] + extra_args

driver_tools.SetupSignalHandlers()
ret = driver_tools.DriverMain(module, argv)
driver_tools.DriverExit(ret, is_final_exit=True)
