#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Creates a grd file for packaging the trace-viewer files.

   This file is modified from the devtools generate_devtools_grd.py file.
"""

import errno
import os
import shutil
import sys
from xml.dom import minidom

kTracingResourcePrefix = 'IDR_TRACING_'
kGrdTemplate = '''<?xml version="1.0" encoding="UTF-8"?>
<grit latest_public_release="0" current_release="1"
      output_all_resource_defines="false">
  <outputs>
    <output filename="grit/tracing_resources.h" type="rc_header">
      <emit emit_type='prepend'></emit>
    </output>
    <output filename="tracing_resources.pak" type="data_package" />
  </outputs>
  <release seq="1">
    <includes>
      <if expr="not is_android"></if>
    </includes>
  </release>
</grit>

'''


class ParsedArgs:
  def __init__(self, source_files, output_filename):
    self.source_files = source_files
    self.output_filename = output_filename


def parse_args(argv):
  output_position = argv.index('--output')
  source_files = argv[:output_position]
  return ParsedArgs(source_files, argv[output_position + 1])


def make_name_from_filename(filename):
  return kTracingResourcePrefix + (os.path.splitext(filename)[1][1:]).upper()


def add_file_to_grd(grd_doc, filename):
  includes_node = grd_doc.getElementsByTagName('if')[0]
  includes_node.appendChild(grd_doc.createTextNode('\n        '))

  new_include_node = grd_doc.createElement('include')
  new_include_node.setAttribute('name', make_name_from_filename(filename))
  new_include_node.setAttribute('file', filename)
  new_include_node.setAttribute('type', 'BINDATA')
  new_include_node.setAttribute('compress', 'gzip')
  new_include_node.setAttribute('flattenhtml', 'true')
  if filename.endswith('.html'):
    new_include_node.setAttribute('allowexternalscript', 'true')
  includes_node.appendChild(new_include_node)


def main(argv):
  parsed_args = parse_args(argv[1:])

  output_directory = os.path.dirname(parsed_args.output_filename)

  doc = minidom.parseString(kGrdTemplate)
  for filename in parsed_args.source_files:
    add_file_to_grd(doc, os.path.basename(filename))

  with open(parsed_args.output_filename, 'w') as output_file:
    output_file.write(doc.toxml(encoding='UTF-8'))


if __name__ == '__main__':
  sys.exit(main(sys.argv))
