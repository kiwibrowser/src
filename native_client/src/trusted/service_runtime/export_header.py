#!/usr/bin/python
# Copyright (c) 2008 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tools for exporting Native Client ABI header files.

This module is used to export Native Client ABI header files -- which
are in the native_client/src/trusted/service_runtime/include directory
-- for use with the SDK and newlib to compile NaCl applications.
"""

import os
import re
import sys


def ProcessStream(instr, outstr):
  """Read internal version of header file from instr, write exported
  version to outstr.  The transformations are:

  1) Remove nacl_abi_ prefixes (in its various incarnations)

  2) Change include directives from the Google coding style
     "native_client/include/foo/bar.h" to <foo/bar.h>, and from
     "native_client/src/trusted/service_runtime/include/baz/quux.h" to
     <baz/quux.h>.

  3) Change include guards from
     "#ifdef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_FOO_H" to
     "#ifdef EXPORT_SRC_TRUSTED_SERVICE_RUNTIME_FOO_H"

  4) Replace "defined(NACL_IN_TOOLCHAIN_HEADERS)" with "1", so that
     the header can conditionalize based on whether it is being used
     as headers for a toolchain.  Note that doing "#ifdef
     __native_client__" in the headers is not good because untrusted
     code can directly #include from service_runtime/include/ just as
     trusted code can.
  """

  abi = r'\b(?:nacl_abi_|NaClAbi|NACL_ABI_)([A-Za-z0-9_]*)'
  cabi = re.compile(abi)

  inc = (r'^#\s*include\s+"native_client(?:/src/trusted/service_runtime)?'+
         r'/include/([^"]*)"')
  cinc = re.compile(inc)

  in_toolchain_sym = re.compile(r'defined\(NACL_IN_TOOLCHAIN_HEADERS\)')

  inc_guard = r'^#\s*(ifndef|define)\s+_?NATIVE_CLIENT[A-Z_]+_H'
  cinc_guard = re.compile(inc_guard)
  include_guard_found = False

  for line in instr:
    line = in_toolchain_sym.sub('(1 /* NACL_IN_TOOLCHAIN_HEADERS */)', line)
    if cinc.search(line):
      print >>outstr, cinc.sub(r'#include <\1>', line)
    elif cinc_guard.match(line):
      include_guard_found = True
      print >>outstr, re.sub('NATIVE_CLIENT', 'EXPORT', line),
    else:
      print >>outstr, cabi.sub(r'\1', line),

  if not include_guard_found:
    print >>sys.stderr, 'No include guard found in', instr.name
    sys.exit(1)


def ProcessDir(srcdir, dstdir):
  if not os.path.isdir(srcdir):
    return
  if not os.path.isdir(dstdir):
    os.makedirs(dstdir)
  for fn in os.listdir(srcdir):
    srcpath = os.path.join(srcdir, fn)
    dstpath = os.path.join(dstdir, fn)
    if os.path.isfile(srcpath) and fn.endswith('.h'):
      ProcessStream(open(srcpath),
                    open(dstpath, 'w'))
    elif os.path.isdir(srcpath):
      ProcessDir(srcpath, dstpath)


def main(argv):
  if len(argv) != 3:
    print >>sys.stderr, ('Usage: ./export_header source/include/path'
                         ' dest/include/path')
    return 1
  ProcessDir(argv[1], argv[2])
  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv))
