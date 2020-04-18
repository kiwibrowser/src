#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
''' Javascript minifier using the closure compiler

This minifier strips spaces and comments out of Javascript using the closure
compiler. It takes the original Javascript on standard input, and outputs
the minified output on standard output.

Any errors or other messages from the compiler are output on standard error.
'''

import argparse
import sys
import tempfile

from compile2 import Checker


def Minify(source):
  parser = argparse.ArgumentParser()
  parser.add_argument("-c", "--closure_args", nargs=argparse.ZERO_OR_MORE,
                      help="Arguments passed directly to the Closure compiler")
  args = parser.parse_args()
  with tempfile.NamedTemporaryFile(suffix='.js') as t1, \
       tempfile.NamedTemporaryFile(suffix='.js') as t2:
    t1.write(source)
    t1.seek(0)
    checker = Checker()
    (compile_error, compile_stderr) = checker.check(
        [t1.name],
        out_file=t2.name,
        closure_args=args.closure_args)
    if compile_error:
      print compile_stderr
    t2.seek(0)
    result = t2.read()
    return result


if __name__ == '__main__':
  orig_stdout = sys.stdout
  result = ''
  try:
    sys.stdout = sys.stderr
    result = Minify(sys.stdin.read())
  finally:
    sys.stdout = orig_stdout
    print result
