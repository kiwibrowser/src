#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from driver_env import env
import driver_tools
import filetype
import pathtools

EXTRA_ENV = {
    'ARGS' : '',
}

PATTERNS = [
    ('(.*)',   "env.append('ARGS', $0)"),
]

def main(argv):
  env.update(EXTRA_ENV)
  driver_tools.ParseArgs(argv, PATTERNS)

  args = env.get('ARGS')
  input = pathtools.normalize(args[-1])
  if filetype.IsPNaClBitcode(input):
    env.append('ARGS', '--bitcode-format=pnacl')
  driver_tools.CheckPathLength(input)
  driver_tools.Run('"${PNACL_ABICHECK}" ${ARGS}')
  return 0;

# Don't just call the binary with -help because most of those options are
# completely useless for this tool.
def get_help(unused_argv):
  return """
USAGE: pnacl-abicheck <input bitcode>
  If <input bitcode> is -, then standard input will be read.
"""
