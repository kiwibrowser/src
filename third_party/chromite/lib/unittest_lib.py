# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittest-only utility functions library."""

from __future__ import print_function

import os

from chromite.lib import cros_build_lib
from chromite.lib import osutils


class BuildELFError(Exception):
  """Generic error building an ELF file."""


def BuildELF(filename, defined_symbols=None, undefined_symbols=None,
             used_libs=None, executable=False, static=False):
  """Builds a dynamic ELF with the provided import and exports.

  Compiles and links a dynamic program that exports some functions, as libraries
  do, and requires some symbols from other libraries. Dependencies shoud live
  in the same directory as the result. This function

  Args:
    filename: The output filename where the ELF is created.
    defined_symbols: The list of symbols this ELF exports.
    undefined_symbols: The list of symbols this ELF requires from other ELFs.
    used_libs: The list of libraries this ELF loads dynamically, including only
        the name of the library. For example, 'bz2' rather than 'libbz2.so.1.0'.
    executable: Whether the file has a main() function.
    static: Whether the file is statically linked (implies executable=True).
  """
  if defined_symbols is None:
    defined_symbols = []
  if undefined_symbols is None:
    undefined_symbols = []
  if used_libs is None:
    used_libs = []
  if static and not executable:
    raise ValueError('static requires executable to be True.')

  source = ''.join('void %s();\n' % sym for sym in undefined_symbols)
  source += """
void __defined_symbols(const char*) __attribute__ ((visibility ("hidden")));
void __defined_symbols(const char* sym) {
  %s
}
""" % ('\n  '.join('%s();' % sym for sym in undefined_symbols))

  source += ''.join("""
void %s() __attribute__ ((visibility ("default")));
void %s() { __defined_symbols("%s"); }
""" % (sym, sym, sym) for sym in defined_symbols)

  if executable:
    source += """
int main() {
  __defined_symbols("main");
  return 42;
}
"""
  source_fn = filename + '_tmp.c'
  osutils.WriteFile(source_fn, source)

  outdir = os.path.dirname(filename)
  cmd = ['gcc', '-o', filename, source_fn]
  if not executable:
    cmd += ['-shared', '-fPIC']
  if static:
    cmd += ['-static']
  cmd += ['-L.', '-Wl,-rpath=./']
  cmd += ['-l%s' % lib for lib in used_libs]
  try:
    cros_build_lib.RunCommand(
        cmd, cwd=outdir, redirect_stdout=True, redirect_stderr=True,
        print_cmd=False)
  except cros_build_lib.RunCommandError, e:
    raise BuildELFError('%s\n%s' % (e.message, e.result.error))
  finally:
    os.unlink(source_fn)
