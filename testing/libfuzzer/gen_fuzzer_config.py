#!/usr/bin/python2
#
# Copyright (c) 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generate or update an existing config (.options file) for libfuzzer test.

Invoked by GN from fuzzer_test.gni.
"""

import argparse
import os
import sys


CONFIG_HEADER = '''# This is an automatically generated config for libFuzzer.
[libfuzzer]
'''

def main():
  parser = argparse.ArgumentParser(description="Generate fuzzer config.")
  parser.add_argument('--config', required=True)
  parser.add_argument('--dict')
  parser.add_argument('--libfuzzer_options', nargs='+', default=[])
  args = parser.parse_args()

  # Script shouldn't be invoked without both arguments, but just in case.
  if not args.dict and not args.libfuzzer_options:
    return

  config_path = args.config
  # Generate .options file.
  with open(config_path, 'w') as options_file:
    options_file.write(CONFIG_HEADER)

    # Dict will be copied into build directory, need only basename for config.
    if args.dict:
      options_file.write('dict = %s\n' % os.path.basename(args.dict))

    for option in args.libfuzzer_options:
      options_file.write(option)
      options_file.write('\n')

if __name__ == '__main__':
  main()
