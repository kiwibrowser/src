#!/usr/bin/env python
#
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import optparse
import sys

# Combines files in |input_files| as one proguard file and write that to
# |output_file|
def GenerateProguardFile(output_file, input_files):
  try:
    with open(output_file, "wb") as target:
      for input_file in input_files:
        f = open(input_file, "rb")
        for line in f:
          target.write(line)
  except IOError:
    raise Exception("Proguard file generation failed")


def main():
  parser = optparse.OptionParser()
  parser.add_option('--output-file',
          help='Output file for the generated proguard file')

  options, input_files = parser.parse_args()
  GenerateProguardFile(options.output_file, input_files)


if __name__ == '__main__':
  sys.exit(main())
