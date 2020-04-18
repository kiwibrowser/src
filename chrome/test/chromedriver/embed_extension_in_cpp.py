#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Embeds Chrome user data files in C++ code."""

import base64
import optparse
import os
import StringIO
import sys
import zipfile

import cpp_source


def main():
  parser = optparse.OptionParser()
  parser.add_option(
      '', '--directory', type='string', default='.',
      help='Path to directory where the cc/h  file should be created')
  options, args = parser.parse_args()

  global_string_map = {}
  string_buffer = StringIO.StringIO()
  zipper = zipfile.ZipFile(string_buffer, 'w')
  for f in args:
    zipper.write(f, os.path.basename(f), zipfile.ZIP_STORED)
  zipper.close()
  global_string_map['kAutomationExtension'] = base64.b64encode(
      string_buffer.getvalue())
  string_buffer.close()

  cpp_source.WriteSource('embedded_automation_extension',
                         'chrome/test/chromedriver/chrome',
                         options.directory, global_string_map)


if __name__ == '__main__':
  sys.exit(main())
