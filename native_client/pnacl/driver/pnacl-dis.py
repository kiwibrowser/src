#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys
import driver_tools
from driver_env import env
from driver_log import Log, DriverOpen, DriverClose
import filetype

EXTRA_ENV = {
  'INPUTS'          : '',
  'OUTPUT'          : '',
  'FLAGS'           : '',
}

DISPatterns = [
  ( '--file-type',            "env.set('FILE_TYPE', '1')"),
  ( ('-o','(.*)'),            "env.set('OUTPUT', pathtools.normalize($0))"),
  ( '(-.*)',                  "env.append('FLAGS', $0)"),
  ( '(.*)',                   "env.append('INPUTS', pathtools.normalize($0))"),
]

def main(argv):
  env.update(EXTRA_ENV)
  driver_tools.ParseArgs(argv, DISPatterns)

  inputs = env.get('INPUTS')
  output = env.getone('OUTPUT')

  for path in inputs + [output]:
    driver_tools.CheckPathLength(path)

  if len(inputs) == 0:
    Log.Fatal("No input files given")

  if len(inputs) > 1 and output != '':
    Log.Fatal("Cannot have -o with multiple inputs")

  for infile in inputs:
    env.push()
    env.set('input', infile)
    env.set('output', output)

    # When we output to stdout, set redirect_stdout and set log_stdout
    # to False to bypass the driver's line-by-line handling of stdout
    # which is extremely slow when you have a lot of output

    if (filetype.IsLLVMBitcode(infile) or
        filetype.IsPNaClBitcode(infile)):
      bitcodetype = 'PNaCl' if filetype.IsPNaClBitcode(infile) else 'LLVM'
      format = bitcodetype.lower()

      if env.has('FILE_TYPE'):
        sys.stdout.write('%s: %s bitcode\n' % (infile, bitcodetype))
        continue
      env.append('FLAGS', '-bitcode-format=' + format)
      if output == '':
        # LLVM by default outputs to a file if -o is missing
        # Let's instead output to stdout
        env.set('output', '-')
        env.append('FLAGS', '-f')
      driver_tools.Run('${LLVM_DIS} ${FLAGS} ${input} -o ${output}')
    elif filetype.IsELF(infile):
      if env.has('FILE_TYPE'):
        sys.stdout.write('%s: ELF\n' % infile)
        continue
      flags = env.get('FLAGS')
      if len(flags) == 0:
        env.append('FLAGS', '-d')
      if output == '':
        # objdump to stdout
        driver_tools.Run('"${OBJDUMP}" ${FLAGS} ${input}')
      else:
        # objdump always outputs to stdout, and doesn't recognize -o
        # Let's add this feature to be consistent.
        fp = DriverOpen(output, 'w')
        driver_tools.Run('${OBJDUMP} ${FLAGS} ${input}', redirect_stdout=fp)
        DriverClose(fp)
    else:
      Log.Fatal('Unknown file type')
    env.pop()
  # only reached in case of no errors
  return 0

def get_help(unused_argv):
  return """Usage: pnacl-dis [options] <input binary file> -o <output.txt>

Disassembler for PNaCl.  Converts either bitcode to text or
native code to assembly.  For native code, this just a wrapper around objdump
so this accepts the usual objdump flags.

OPTIONS:
  -o <file>        Output to file
  -help | -h       Output this help.
  --file-type      Detect and print the file type for each of the input files.
"""
