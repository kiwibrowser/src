#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import driver_tools
from driver_env import env
from driver_log import Log
import sys

EXTRA_ENV = {
  'INPUTS'      : '',
  'OUTPUT'      : '',

  'MC_FLAGS'       : '-assemble -filetype=obj ${MC_FLAGS_%ARCH%}',
  'MC_FLAGS_ARM'   : ('-arch=arm -triple=armv7a-nacl -mcpu=cortex-a9 ' +
                      '-mattr=+neon'),
  'MC_FLAGS_X8632' : '-arch=x86 -triple=i686-nacl',
  'MC_FLAGS_X8664' : '-arch=x86-64 -triple=x86_64-nacl',
  'MC_FLAGS_MIPS32': '-arch=mipsel -triple=mipsel-nacl',
  'MC_FLAGS_ARM_NONSFI': ('-arch=arm -triple=armv7a -mcpu=cortex-a9 '
                          '-mattr=+neon'),
  'MC_FLAGS_X8632_NONSFI': '-arch=x86 -triple=i686',

  'RUN_LLVM_AS'    : '${LLVM_AS} ${input} -o ${output}',
  'RUN_LLVM_MC'    : '${LLVM_MC} ${MC_FLAGS} ${input} -o ${output}',
}

VERSION_STR = """Portable Native Client assembler
Compatibility:
GNU assembler version 2.21.51 (pnacl-pc-nacl) using BFD version (GNU Binutils) 2.21.51.20110525
Low Level Virtual Machine (http://llvm.org/)
"""

def DumpVersionDebug():
  sys.stderr.write(VERSION_STR)

def DumpVersionAndExit():
  sys.stdout.write(VERSION_STR)
  driver_tools.DriverExit(0)

ASPatterns = [
  ( '-o(.+)',          "env.set('OUTPUT', pathtools.normalize($0))"),
  ( ('-o', '(.+)'),    "env.set('OUTPUT', pathtools.normalize($0))"),

  ( '-v',              DumpVersionDebug),
  ( '--version',       DumpVersionAndExit),

  # Ignore these assembler flags
  ( '(-Qy)',                  ""),
  ( ('(--traditional-format)', '.*'), ""),
  ( '(-gstabs)',              ""),
  ( '(--gstabs)',             ""),
  ( '(-gdwarf2)',             ""),
  ( '(--gdwarf2)',            ""),
  ( '(--fatal-warnings)',     ""),
  ( '(-meabi=.*)',            ""),
  ( '(-mfpu=.*)',             ""),
  ( '(-march=.*)',            ""),

  ( '(-.+)',  driver_tools.UnrecognizedOption),

  # Unmatched parameters should be treated as
  # assembly inputs by the "as" incarnation.
  ( '(-)',   "env.append('INPUTS', $0)"),
  ( '(.*)',  "env.append('INPUTS', pathtools.normalize($0))"),
]

def main(argv):
  env.update(EXTRA_ENV)
  driver_tools.ParseArgs(argv, ASPatterns)
  arch = driver_tools.GetArch()

  inputs = env.get('INPUTS')
  output = env.getone('OUTPUT')

  for path in inputs + [output]:
    driver_tools.CheckPathLength(path)

  num_inputs = len(inputs)
  if num_inputs > 1:
    Log.Fatal('Expecting exactly one input file')
  elif num_inputs == 1:
    the_input = inputs[0]
  else:
    # stdin
    the_input = '-'


  if arch:
    output_type = 'o'
  else:
    output_type = 'po'

  if output == '':
    output = 'a.out'

  env.push()
  env.set('input', the_input)
  env.set('output', output)

  if output_type == 'po':
    # .ll to .po
    driver_tools.Run("${RUN_LLVM_AS}")
  else:
    # .s to .o
    driver_tools.Run("${RUN_LLVM_MC}")
  env.pop()
  # only reached in case of no errors
  return 0

def get_help(argv):
  return """
LLVM Assembler for PNaCl
Transform LLVM assembly (.ll) to LLVM bitcode.

Usage: pnacl-as [options] <input .ll file> -o <output.po>

OPTIONS:
  -o <file>        Output to file
  --version        Display version information
  -help | -h       Output this help.
"""
