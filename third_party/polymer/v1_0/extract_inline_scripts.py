# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Extracts inlined scripts from the provided Polymer HTM files to a separate JS
file. A JS file extracted from the file with name 'foo.html' will have a name
'foo-extracted.js'. Inclusion of the script file will be added to
'foo.html' as '<script src="foo-extracted.js"></script>'.
"""

from os import path as os_path
import sys
import shutil

_HERE_PATH = os_path.dirname(__file__)
_SRC_PATH = os_path.normpath(os_path.join(_HERE_PATH, '..', '..', '..'))
sys.path.append(os_path.join(_SRC_PATH, 'third_party', 'node'))
import node
import node_modules


def main(original_html):
  name = os_path.splitext(os_path.basename(original_html))[0]
  dst_dir = os_path.dirname(original_html)
  extracted_html = os_path.join(dst_dir, name + '-extracted.html')
  extracted_js = os_path.join(dst_dir, name + '-extracted.js')

  node.RunNode([
      node_modules.PathToCrisper(),
      '--script-in-head', 'false',
      '--source', original_html,
      '--html', extracted_html,
      '--js', extracted_js])

  shutil.move(extracted_html, original_html)


if __name__ == '__main__':
  main(sys.argv[1])
