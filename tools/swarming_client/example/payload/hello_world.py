#!/usr/bin/env python
# coding=utf-8
# Copyright 2012 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""This script is meant to be run on a Swarming bot."""

import os
import sys


def main():
  print('Hello world: ' + sys.argv[1])
  if len(sys.argv) == 3:
    # Write a file in ${ISOLATED_OUTDIR}.
    p = os.path.join(sys.argv[2].decode('utf-8'), u'da 💣.txt').encode('utf-8')
    with open(p, 'wb') as f:
      r = 'FOO:%r' % os.environ.get('FOO')
      f.write(r)
  return 0


if __name__ == '__main__':
  sys.exit(main())
