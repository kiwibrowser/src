#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys
import xml.sax


class PathsExtractor(xml.sax.ContentHandler):

  def __init__(self):
    self.paths = []

  def startElement(self, name, attrs):
    if name != 'structure':
      return
    path = attrs['file']
    if path.startswith('../../../third_party/web-animations-js'):
      return
    prefix_1_0 = '../../../third_party/polymer/v1_0/components-chromium/'
    if path.startswith(prefix_1_0):
      self.paths.append(path[len(prefix_1_0):])
    else:
      raise Exception("Unexpected path %s." % path)

def main(argv):
  xml_handler = PathsExtractor()
  xml.sax.parse(argv[1], xml_handler)
  print '\n'.join(sorted(xml_handler.paths))


if __name__ == '__main__':
  sys.exit(main(sys.argv))
