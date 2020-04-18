#!/usr/bin/python
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Wrapper for invoking the BFD loader

A simple script to invoke the bfd loader instead of gold.
This script is in a filename "ld" so it can be invoked from gcc
via the -B flag.
"""
# TODO(bradchen): Delete this script when Gold supports linker scripts properly.
import os
import subprocess
import sys

def PathTo(fname):
  if fname[0] == os.pathsep:
    return fname
  for p in os.environ["PATH"].split(os.pathsep):
    fpath = os.path.join(p, fname)
    if os.path.exists(fpath):
      return fpath
  return fname


def GccPrintName(cxx_bin, what, switch, defresult):
  popen = subprocess.Popen(cxx_bin + ' ' + switch,
                           shell=True,
                           stdout=subprocess.PIPE,
                           stdin=subprocess.PIPE)
  result, error = popen.communicate()
  if popen.returncode != 0:
    print "Could not find %s: %s" % (what, error)
    return defresult
  return result.strip()


def FindLDBFD(cxx_bin):
  ld = GccPrintName(cxx_bin, 'ld', '-print-prog-name=ld', 'ld')
  ld_bfd = PathTo(ld + ".bfd")
  if os.access(ld_bfd, os.X_OK):
    return ld_bfd
  return ld


def FindLibgcc(cxx_bin):
  return GccPrintName(cxx_bin, 'libgcc', '-print-libgcc-file-name', None)


def main(args):
  # Find path to compiler, either from the command line or the environment,
  # falling back to just 'g++'
  if '--compiler' in args:
    index = args.index('--compiler')
    cxx_bin = args[index + 1]
    if not cxx_bin:
      sys.stderr.write("Empty --compiler option specified\n")
      return 1
    del args[index:index + 2]
  else:
    cxx_bin = os.getenv('CXX', 'g++')

  args = [FindLDBFD(cxx_bin)] + args
  libgcc = FindLibgcc(cxx_bin)
  if libgcc is not None:
    args.append(libgcc)
  return subprocess.call(args)

if __name__ == "__main__":
  sys.exit(main(sys.argv[1:]))
