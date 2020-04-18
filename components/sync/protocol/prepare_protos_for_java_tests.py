#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Converts protocol buffer definitions to ones supported by the Nano library.

Note: Java files generated from the output of this script should only be used
in tests.

Chromium uses the Java Protocol Buffers Nano runtime library. This library,
compared to the standard proto compilter in C++, is limited. Most notably, the
retain_unknown_fields option is not supported.

This script also adds Java-specific configs to enable usage in Java. This script
is Sync-specific, so a Sync package name is used.
"""

import optparse
import os
import re
import sys


def main():
  parser = optparse.OptionParser(
      usage='Usage: %prog [options...] original_proto_files')
  parser.add_option('-o', '--output_dir', dest='output_dir',
                    help='Where to put the modified proto files')

  options, args = parser.parse_args()
  if not options.output_dir:
    print 'output_dir not specified.'
    return 1

  # Map of original base file names to their contents.
  original_proto_contents = {}
  for original_proto_file in args:
    with open(original_proto_file, 'r') as original_file:
      base_name = os.path.basename(original_proto_file)
      original_proto_contents[base_name] = original_file.read()

  for file_name, original_contents in original_proto_contents.iteritems():
    new_contents = ConvertProtoFileContents(original_contents)
    with open(os.path.join(options.output_dir, file_name), 'w') as new_file:
      new_file.write(new_contents)


def ConvertProtoFileContents(contents):
  """Returns a Nano-compatible version of contents.

  Args:
    contents: The contents of a protocol buffer definition file.
  """
  # Add the java_multiple_files and java_package options. Options must be set
  # after the syntax declaration, so look for the declaration and place the
  # options immediately after it.
  # TODO(pvalenzuela): Set Java options via proto compiler flags instead of
  # modifying the files here.
  syntax_regex = re.compile(r'^\s*syntax\s*=.*;', re.MULTILINE)
  syntax_end = syntax_regex.search(contents).end()
  java_options = (
      'option java_multiple_files = true; '
      'option java_package = "org.chromium.components.sync.protocol";')

  contents_to_join = (contents[:syntax_end], java_options,
                      contents[syntax_end:])
  return ''.join(contents_to_join)


if __name__ == '__main__':
  sys.exit(main())
