# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generates a mini_installer with a lower version than an existing one."""

import argparse
import subprocess
import sys


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('--out', help='Path to the generated mini_installer.')
  args = parser.parse_args()
  assert args.out

  return subprocess.call([
      'alternate_version_generator.exe',
      '--force',
      '--previous',
      '--out=' + args.out,
      ])


if '__main__' == __name__:
  sys.exit(main())
