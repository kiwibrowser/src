#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import with_statement
import os
import string
import sys


FILE_TEMPLATE = \
"""<?xml version="1.0" encoding="utf-8"?>
<!--
  This file is generated.
  Please use 'src/tools/polymer/polymer_grdp_to_txt.py' and
  'src/tools/polymer/txt_to_polymer_grdp.py' to modify it, if possible.

  'polymer_grdp_to_txt.py' converts 'polymer_resources.grdp' to a plain list of
  used Polymer components:
    ...
    iron-iron-iconset/iron-iconset-extracted.js
    iron-iron-iconset/iron-iconset.html
    ...

  'txt_to_polymer_grdp.py' converts list back to GRDP file.

  Usage:
    $ polymer_grdp_to_txt.py polymer_resources.grdp > /tmp/list.txt
    $ vim /tmp/list.txt
    $ txt_to_polymer_grdp.py /tmp/list.txt > polymer_resources.grdp
-->
<grit-part>
  <!-- Polymer 1.0 -->
%(v_1_0)s
  <structure name="IDR_POLYMER_1_0_WEB_ANIMATIONS_JS_WEB_ANIMATIONS_NEXT_LITE_MIN_JS"
             file="../../../third_party/web-animations-js/sources/web-animations-next-lite.min.js"
             type="chrome_html"
             compress="gzip" />
</grit-part>
"""


DEFINITION_TEMPLATE_1_0 = \
"""  <structure name="%s"
             file="../../../third_party/polymer/v1_0/components-chromium/%s"
             type="chrome_html"
             compress="gzip" />"""


def PathToGritId(path):
  table = string.maketrans(string.lowercase + '/.-', string.uppercase + '___')
  return 'IDR_POLYMER_1_0_' + path.translate(table)


def SortKey(record):
  return (record, PathToGritId(record))


def ParseRecord(record):
  return record.strip()


class FileNotFoundException(Exception):
  pass


_HERE = os.path.dirname(os.path.realpath(__file__))
_POLYMER_DIR = os.path.join(_HERE, os.pardir, os.pardir,
    'third_party', 'polymer', 'v1_0', 'components-chromium')


def main(argv):
  with open(argv[1]) as f:
    records = [ParseRecord(r) for r in f if not r.isspace()]
  lines = { 'v_1_0': [] }
  for path in sorted(set(records), key=SortKey):
    full_path = os.path.normpath(os.path.join(_POLYMER_DIR, path))
    if not os.path.exists(full_path):
      raise FileNotFoundException('%s not found' % full_path)

    template = DEFINITION_TEMPLATE_1_0
    lines['v_1_0'].append(
        template % (PathToGritId(path), path))
  print FILE_TEMPLATE % { 'v_1_0': '\n'.join(lines['v_1_0']) }

if __name__ == '__main__':
  sys.exit(main(sys.argv))
