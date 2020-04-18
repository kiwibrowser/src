#!/usr/bin/python
# Copyright (c) 2016 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# The original shaderc build_version header file is generated directly into
# the src directory. This script merely changes the directory to an output
# directory before executing that script. This way we can generate the header
# in any directory.

import os
import subprocess
import sys

def main(args):
  if not args or not os.path.isdir(args[0]) or not os.path.isfile(args[1]):
    sys.stderr.write('Usage: shaderc_build_version_wrapper.py <outdir>'
                     ' <script_path> <script_arg1> <script_arg2>...')
    return 1

  os.chdir(args[0])
  subprocess.check_call([sys.executable] + args[1:])
  return 0


if __name__ == '__main__':
  try:
    cwd = os.getcwd()
    sys.exit(main(sys.argv[1:]))
  finally:
    os.chdir(cwd)
