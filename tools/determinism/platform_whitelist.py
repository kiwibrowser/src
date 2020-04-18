#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Print platform's deterministic build whitelist."""

import ast
import argparse
import json
import os
import sys

BASE_DIR = os.path.dirname(os.path.abspath(__file__))


def PrintWhitelist(target_platform, output_json):
  """Print whitelist to output.

  Args:
    target_platform: name of target platform.
    output_json: output filename to store result with JSON.
                 print to stdout if the field is False.
  """
  with open(os.path.join(BASE_DIR, 'deterministic_build_whitelist.pyl')) as f:
    whitelist = ast.literal_eval(f.read())[target_platform]

  if output_json:
    with open(output_json, 'w') as f:
      json.dump(whitelist, f)
  else:
    json.dump(whitelist, sys.stdout)


def main():
  parser = argparse.ArgumentParser()
  target = {
      'darwin': 'mac', 'linux2': 'linux', 'win32': 'win'
  }.get(sys.platform, sys.platform)
  parser.add_argument('-t', '--target-platform',
                      default=target,
                      choices=('android', 'mac', 'linux', 'win'),
                      help='The target platform.')
  parser.add_argument('--output-json', help='JSON file to output differences')
  args = parser.parse_args()

  if not args.target_platform:
    parser.error('--target-platform is required')

  PrintWhitelist(args.target_platform, args.output_json)


if __name__ == '__main__':
  sys.exit(main())
