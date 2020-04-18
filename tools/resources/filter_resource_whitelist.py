#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""filter_resource_whitelist.py [-h] [--input INPUT] [--filter FILTER]
[--output OUTPUT]

INPUT specifies a resource whitelist file containing resource IDs that should
be whitelisted, where each line of INPUT contains a single resource ID.

FILTER specifies a resource whitelist file containing resource IDs that should
not be whitelisted, where each line of FILTER contains a single resource ID.

Filters a resource whitelist by removing resource IDs that are contained in a
another resource whitelist.

This script is used to generate Monochrome's locale paks.
"""

import argparse
import sys


def main():
  parser = argparse.ArgumentParser(usage=__doc__)
  parser.add_argument(
      '--input', type=argparse.FileType('r'), required=True,
      help='A resource whitelist where each line contains one resource ID. '
           'These IDs, excluding the ones in FILTER, are to be included.')
  parser.add_argument(
      '--filter', type=argparse.FileType('r'), required=True,
      help='A resource whitelist where each line contains one resource ID. '
           'These IDs are to be excluded.')
  parser.add_argument(
      '--output', type=argparse.FileType('w'), default=sys.stdout,
      help='The resource list path to write (default stdout)')

  args = parser.parse_args()

  input_resources = list(int(resource_id) for resource_id in args.input)
  filter_resources = set(int(resource_id) for resource_id in args.filter)
  output_resources = [resource_id for resource_id in input_resources
                      if resource_id not in filter_resources]

  for resource_id in sorted(output_resources):
    args.output.write('%d\n' % resource_id)

if __name__ == '__main__':
  main()
