#!/usr/bin/env python
#
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import optparse
import os
import sys

REPOSITORY_ROOT = os.path.abspath(os.path.join(
    os.path.dirname(__file__), '..', '..', '..'))

sys.path.append(os.path.join(REPOSITORY_ROOT, 'build/android/gyp/util'))
import build_utils


def ExtractJars(options):
  # The paths of the files in the jar will be the same as they are passed in to
  # the command. Because of this, the command should be run in
  # options.classes_dir so the .class file paths in the jar are correct.
  jar_cwd = options.classes_dir
  build_utils.DeleteDirectory(jar_cwd)
  build_utils.MakeDirectory(jar_cwd)
  for jar in build_utils.ParseGnList(options.jars):
    jar_path = os.path.abspath(jar)
    jar_cmd = ['jar', 'xf', jar_path]
    build_utils.CheckOutput(jar_cmd, cwd=jar_cwd)


def main():
  parser = optparse.OptionParser()
  build_utils.AddDepfileOption(parser)
  parser.add_option('--classes-dir', help='Directory to extract .class files.')
  parser.add_option('--jars', help='Paths to jars to extract.')
  parser.add_option('--stamp', help='Path to touch on success.')

  options, _ = parser.parse_args()

  ExtractJars(options)

  if options.depfile:
    assert options.stamp
    build_utils.WriteDepfile(options.depfile, options.stamp)

  if options.stamp:
    build_utils.Touch(options.stamp)


if __name__ == '__main__':
  sys.exit(main())

