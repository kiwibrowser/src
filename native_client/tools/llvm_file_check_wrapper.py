# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# We want to run FileCheck | <cmd_line>, but all that as a parameter
# to the python test wrapper (see CommandTest in ../SConstruct).
# Without this wrapper for FileCheck, the command interpreter takes
# everything on the left of '|' and pipes it to everything on the right,
# which is not what we want.

import subprocess
import sys

def Main(args):
  if len(args) < 3:
    print "llvm_file_check_wrapper <filecheck> <check_file> <cmd_line>"
    print "Where:"
    print "<filecheck> is the path to FileCheck"
    print "<check_file> is a text file containing 'CHECK' statements"
    print "<cmd_line> is the command line to use as input to FileCheck"
    return 1

  file_check=args[0]
  check_file=args[1]
  what_to_run=args[2:]

  what_to_run_proc = subprocess.Popen(what_to_run, stdout=subprocess.PIPE)
  file_check_proc = subprocess.check_output((file_check, check_file),
          stdin=what_to_run_proc.stdout)
  what_to_run_proc.wait()

if __name__ == '__main__':
  sys.exit(Main(sys.argv[1:]))
