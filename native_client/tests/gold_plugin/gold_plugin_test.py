#!/usr/bin/env python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Basic sanity check for PNaCl host binaries and gold plugin.

# This provides a basic test of some PNaCl host binaries (these are covered by
# the llvm regression tests and binutils test suite, but we aren't running all
# of those on all the toolchain bots right now), and the gold plugin (which
# doesn't have any other simple targeted tests). In particular it can operate
# with only the host binaries built, and no target libraries, so it's suitable
# for running on the mac and windows toolchain bots, where we don't currently
# build the target libraries.

# This test is designed to be easily run from outside of SCons.

import argparse
import os
import re
import subprocess
import sys


def RunTool(tooldir, tool, flags, cwd, fatal_errors=True):
  """Run a tool from the toolchain.

     Arguments:
       tooldir: Directory containing the toolchain (toolchain binaries will be
                run from the 'bin' subdirectory).
       tool: The name of a tool binary in the 'bin' subdirectory
       flags: A list of flags to pass to the tool
       fatal_errors: If true, exit with an error if the run fails.
  """
  try:
    toolcmd = os.path.join(tooldir, 'bin', tool)
    cmd = [toolcmd] + flags
    print 'Running command:', cmd
    # shell=True is required to work properly on Windows
    return subprocess.check_output(' '.join(cmd), cwd=cwd, shell=True)
  except subprocess.CalledProcessError, exc:
    print 'Command failed with status', exc.returncode
    print 'Output was:'
    print exc.output
    if fatal_errors:
      sys.exit(1)


def GenerateCFile(tempdir, filename, globalvars, functions):
  'Generate a C file defining the specified globals and functions.'
  with open(os.path.join(tempdir, filename), 'w') as f:
    print >> f, 'extern int global0;'
    for g in globalvars:
      print >> f, 'int global%s = 1;' % g
    for func in functions:
      print >> f, 'int function%s() { return global0; }' % func


def CheckExpectedOutput(output, expected):
  """Assert that expected content appears in the output.

     Arguments:
       output: Output from a tool to be searched for matches
       expected: An iterable which contains regular expressions, a match for
       each of which must appear in 'output'

     Returns the number of elements in 'expected' for which no match is found.
  """
  failures = 0
  for ex in expected:
    match = re.search(ex, output)
    if not match:
      print 'Test match failed:'
      print 'Searching for regex:', ex
      failures += 1
  if failures:
    print 'output:\n', output
  return failures


def main(argv):
  parser = argparse.ArgumentParser(
      description='Run host binary and gold plugin sanity checks')
  parser.add_argument('-d', '--cwd', default=os.getcwd())
  parser.add_argument('-t', '--toolchaindir',
                      default=os.path.join(
                          os.getcwd(),
                          'toolchain', 'linux_x86', 'pnacl_newlib_raw'))
  args = parser.parse_args()

  print 'Using cwd:', args.cwd
  if not os.path.isdir(args.cwd):
    os.makedirs(args.cwd)

  if not os.path.isdir(args.toolchaindir):
    print 'Could not find toolchain directory:', args.toolchaindir
    sys.exit(1)

  GenerateCFile(args.cwd, 'module1.c', range(2), range(2))
  GenerateCFile(args.cwd, 'module2.c', range(2,512), range(2,512))

  # Since almost all of these tool runs use output from a previous tool run, we
  # let all tool run failures be fatal.
  failure_count = 0

  # Compile a C file to a bitcode object (tests clang)
  RunTool(args.toolchaindir, 'pnacl-clang',
          ['-c', 'module1.c', '-o', 'module1.bc'], args.cwd)
  RunTool(args.toolchaindir, 'pnacl-clang',
          ['-c', 'module2.c', '-o', 'module2.bc'], args.cwd)

  # Dump the bitcode object (tests llvm-dis)
  output = RunTool(args.toolchaindir, 'pnacl-dis', ['module1.bc'], args.cwd)
  failure_count += CheckExpectedOutput(
      output,
      ['@global0 = global i32 1', 'define i32 @function0'])

  # Translate a bitcode file to a native object (tests llc)
  RunTool(args.toolchaindir, 'pnacl-translate',
          ['-c', 'module1.bc', '-arch', 'x86-32', '-o', 'module1.o',
           '--allow-llvm-bitcode-input'], args.cwd)

  # Dump the object symbols (tests binutils objdump)
  output = RunTool(
      args.toolchaindir, 'pnacl-dis', ['-t', 'module1.o'], args.cwd)
  failure_count = CheckExpectedOutput(
      output,
      ['format elf32-i386', '\\.data.*global0', '\\.text.*function0'])

  # Add the bitcode files to an archive (tests binutils ar, bfd plugin)
  RunTool(args.toolchaindir, 'pnacl-ar',
          ['rc', 'libmodules.a', 'module1.bc', 'module2.bc'], args.cwd)

  # Dump the archive symbols (tests binutils nm, bfd plugin)
  output = RunTool(args.toolchaindir, 'pnacl-nm', ['libmodules.a'], args.cwd)
  failure_count += CheckExpectedOutput(
      output,
      ['module1.bc', 'module2.bc',
       'function1', 'global1', 'function511', 'global511'])

  # Link 2 files (gold, gold plugin)
  GenerateCFile(args.cwd, 'module3.c', range(512,513), range(512,513))
  RunTool(args.toolchaindir, 'pnacl-clang',
          ['-c', 'module3.c', '-o', 'module3.bc'], args.cwd)
  RunTool(args.toolchaindir, 'pnacl-ld',
          ['module3.bc', '-L.', '-lmodules', '-o', 'modules_linked'], args.cwd)
  # Verify
  output = RunTool(args.toolchaindir, 'pnacl-dis', ['modules_linked'], args.cwd)
  failure_count += CheckExpectedOutput(
      output,
      ['define internal i32 @function0', 'define internal i32 @function512'])

  print 'Got %d failures in tool output' % failure_count
  return failure_count

if __name__ == '__main__':
  sys.exit(main(sys.argv))
