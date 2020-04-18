#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import driver_tools
from driver_env import env
from driver_log import Log
import filetype

EXTRA_ENV = {
  'TOOLNAME':   '',
  'INPUTS':     '',
  'FLAGS':      '',
}

PATTERNS = [
  ( '(-.*)',    "env.append('FLAGS', $0)"),
  ( '(.*)',     "env.append('INPUTS', pathtools.normalize($0))"),
]

def main(argv):
  env.update(EXTRA_ENV)
  driver_tools.ParseArgs(argv, PATTERNS)

  inputs = env.get('INPUTS')

  if len(inputs) == 0:
    Log.Fatal("No input files given")

  for infile in inputs:
    driver_tools.CheckPathLength(infile)
    env.push()
    env.set('input', infile)

    # For frozen PNaCl bitcode, use 'llvm-nm -bitcode-format=pnacl'. For all
    # other formats, use the binutils nm with our gold plugin.
    # Update: llvm-nm -bitcode-format=pnacl is currently disabled.
    if filetype.IsPNaClBitcode(infile):
      Log.Fatal(
          'nm on finalized bitcode is currently disabled.\n'
          'See: https://code.google.com/p/nativeclient/issues/detail?id=3993')
    else:
      env.set('TOOLNAME', '${NM}')
      env.append('FLAGS', '--plugin=${GOLD_PLUGIN_SO}')

    driver_tools.Run('"${TOOLNAME}" ${FLAGS} ${input}')
    env.pop()

  # only reached in case of no errors
  return 0

def get_help(unused_argv):
  return """
Usage: %s [option(s)] [file(s)]
 List symbols in [file(s)].

 * For stable PNaCl bitcode files, this calls the llvm-nm tool.
 * For all other files, this calls the standard nm from binutils - please see
   that tool's help pages for options.
""" % env.getone('SCRIPT_NAME')
