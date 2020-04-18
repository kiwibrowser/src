#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Generate a mig server and client pair from migs.def
# Emitting:
#     nacl_exc_server.c
#     nacl_exc.h

import argparse
import os
import re
import subprocess
import sys
import tempfile


def MkDirs(path):
  try:
    os.makedirs(path)
  except OSError:
    pass


def Generate(src_defs, dst_header, dst_server, sdk):
  """Generate interface headers and server from a .defs file using MIG.

  Args:
    src_defs: Most likely /usr/include/mach/exc.defs.
    dst_header: Output generated header for the interface.
    dst_server: Output generated server for the interface.
    sdk: If not none, the argument to pass to isysroot.
  """
  dst_header = os.path.abspath(dst_header)
  dst_server = os.path.abspath(dst_server)
  # Create directories containing output.
  MkDirs(os.path.dirname(dst_header))
  MkDirs(os.path.dirname(dst_server))

  # Load exc.defs (input)
  fh = open(src_defs, 'r')
  defs = fh.read()
  fh.close()

  # Modify exc.defs to isolate its namespace.
  # Change the catch_ prefix on each handler to nacl_cach_.
  (defs, count) = re.subn('ServerPrefix catch_;',
                          'ServerPrefix nacl_catch_;', defs)
  assert count == 1
  # Change the message name from exc to nacl_exc to avoid other collisions.
  # (But keep the message id base 2401 the same to match the OS.)
  (defs, count) = re.subn('exc 2401;',
                          'nacl_exc 2401;', defs)
  assert count == 1

  nacl_exc_defs_path = None
  try:
    # Write out to a temporary file.
    (nacl_exc_defs, nacl_exc_defs_path) = tempfile.mkstemp(
        suffix='.defs', prefix='run_mig')
    os.write(nacl_exc_defs, defs)
    os.close(nacl_exc_defs)

    # Run the 'Mach Interface Generator'.
    args = ['mig',
            '-server', dst_server,
            '-user', '/dev/null',
            '-header', dst_header]

    # If SDKROOT is set to an SDK that Xcode doesn't know about, it might
    # interfere with mig's ability to find a valid compiler (via xcrun -sdk ...
    # -find cc). Clear out SDKROOT if set to avoid this problem, but pass it to
    # mig as its -isysroot argument so that the desired SDK is used.
    if 'SDKROOT' in os.environ:
      args.append('-isysroot')
      args.append(os.environ['SDKROOT'])
      del os.environ['SDKROOT']
    elif sdk:
      args.append('-isysroot')
      args.append(sdk)

    args.append(nacl_exc_defs_path)
    subprocess.check_call(args)
  finally:
    if nacl_exc_defs_path and os.path.exists(nacl_exc_defs_path):
      os.remove(nacl_exc_defs_path)


def Main(args):
  parser = argparse.ArgumentParser()
  parser.add_argument('--sdk', help='Path to SDK')
  parser.add_argument('src_defs')
  parser.add_argument('dst_header')
  parser.add_argument('dst_server')
  parser.add_argument('developer_dir', nargs='?')
  parsed = parser.parse_args(args)
  if parsed.developer_dir is not None:
    os.environ['DEVELOPER_DIR'] = parsed.developer_dir
  Generate(args[0], args[1], args[2], parsed.sdk)


if __name__ == '__main__':
  Main(sys.argv[1:])
