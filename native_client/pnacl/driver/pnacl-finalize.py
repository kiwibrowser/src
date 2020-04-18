#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import shutil

from driver_env import env
from driver_log import Log
import driver_tools
import filetype


EXTRA_ENV = {
  'INPUTS'             : '',
  'OUTPUT'             : '',
  'DISABLE_FINALIZE'   : '0',
  'DISABLE_STRIP_SYMS' : '0',
  'COMPRESS'           : '0',
}

PrepPatterns = [
    ( ('-o','(.*)'),     "env.set('OUTPUT', pathtools.normalize($0))"),
    ( '--no-finalize',   "env.set('DISABLE_FINALIZE', '1')"),
    ( '--no-strip-syms', "env.set('DISABLE_STRIP_SYMS', '1')"),
    ( '--compress',      "env.set('COMPRESS', '1')"),
    ( '--strip-(all|debug|unneeded)', ""),
    ( '(-.*)',           driver_tools.UnrecognizedOption),
    ( '(.*)',            "env.append('INPUTS', pathtools.normalize($0))"),
]

def main(argv):
  env.update(EXTRA_ENV)
  driver_tools.ParseArgs(argv, PrepPatterns)

  inputs = env.get('INPUTS')
  output = env.getone('OUTPUT')

  for path in inputs + [output]:
    driver_tools.CheckPathLength(path)

  if len(inputs) != 1:
    Log.Fatal('Can only have one input')
  f_input = inputs[0]

  # Allow in-place file changes if output isn't specified..
  if output != '':
    f_output = output
  else:
    f_output = f_input

  if env.getbool('DISABLE_FINALIZE') or filetype.IsPNaClBitcode(f_input):
    # Just copy the input file to the output file.
    if f_input != f_output:
      shutil.copyfile(f_input, f_output)
    return 0

  opt_flags = ['-disable-opt', '-strip-metadata', '-strip-module-flags',
               '--bitcode-format=pnacl', f_input, '-o', f_output]
  if env.getbool('DISABLE_STRIP_SYMS'):
    opt_flags += ['-strip-debug']
  else:
    opt_flags += ['-strip']
  # Transform the file, and convert it to a PNaCl bitcode file.
  driver_tools.RunDriver('pnacl-opt', opt_flags)
  # Compress the result if requested.
  if env.getbool('COMPRESS'):
    driver_tools.RunDriver('pnacl-compress', [f_output])
  return 0


def get_help(unused_argv):
  script = env.getone('SCRIPT_NAME')
  return """Usage: %s <options> in-file
  This tool prepares a PNaCl bitcode application for ABI stability.

  The options are:
  -h --help                 Display this output
  -o <file>                 Place the output into <file>. Otherwise, the
                            input file is modified in-place.
  --no-finalize             Don't run preparation steps (just copy in -> out).
  --no-strip-syms           Don't strip function names. NOTE: this may
                            or may not be allowed by the PNaCl ABI checker.
  --compress                Run pnacl-compress on the generated pexe to minimize
                            pexe size.
  --strip-all               Ignored for compatibility with 'strip'
  --strip-debug
  --strip-unneeded
""" % script
