#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import optimize_webui
import os
import shutil
import tempfile
import unittest


_HERE_DIR = os.path.dirname(__file__)


class OptimizeWebUiTest(unittest.TestCase):
  def setUp(self):
    self._out_folder = None
    self._tmp_dirs = []
    self._tmp_src_dir = None

  def tearDown(self):
    for tmp_dir in self._tmp_dirs:
      shutil.rmtree(tmp_dir)

  def _write_file_to_src_dir(self, file_path, file_contents):
    if not self._tmp_src_dir:
      self._tmp_src_dir = self._create_tmp_dir()
    file_path_normalized = os.path.normpath(os.path.join(self._tmp_src_dir,
                                                         file_path))
    file_dir = os.path.dirname(file_path_normalized)
    if not os.path.exists(file_dir):
      os.makedirs(file_dir)
    with open(file_path_normalized, 'w') as tmp_file:
      tmp_file.write(file_contents)

  def _create_tmp_dir(self):
    # TODO(dbeam): support cross-drive paths (i.e. d:\ vs c:\).
    tmp_dir = tempfile.mkdtemp(dir=_HERE_DIR)
    self._tmp_dirs.append(tmp_dir)
    return tmp_dir

  def _read_out_file(self, file_name):
    assert self._out_folder
    return open(os.path.join(self._out_folder, file_name), 'r').read()

  def _run_optimize(self, depfile, html_in_file, html_out_file, js_out_file):
    # TODO(dbeam): make it possible to _run_optimize twice? Is that useful?
    assert not self._out_folder
    self._out_folder = self._create_tmp_dir()
    optimize_webui.main([
      '--depfile', os.path.join(self._out_folder,'depfile.d'),
      '--html_in_file', html_in_file,
      '--html_out_file', html_out_file,
      '--host', 'fake-host',
      '--input', self._tmp_src_dir,
      '--js_out_file', js_out_file,
      '--out_folder', self._out_folder,
    ])


  def testSimpleOptimize(self):
    self._write_file_to_src_dir('element.html', '<div>got here!</div>')
    self._write_file_to_src_dir('element.js', "alert('yay');")
    self._write_file_to_src_dir('element_in_dir/element_in_dir.html',
                                '<script src="element_in_dir.js">')
    self._write_file_to_src_dir('element_in_dir/element_in_dir.js',
                                "alert('hello from element_in_dir');")
    self._write_file_to_src_dir('ui.html', '''
<link rel="import" href="element.html">
<link rel="import" href="element_in_dir/element_in_dir.html">
<script src="element.js"></script>
''')

    self._run_optimize(depfile='depfile.d',
                       html_in_file='ui.html',
                       html_out_file='fast.html',
                       js_out_file='fast.js')

    fast_html = self._read_out_file('fast.html')
    self.assertNotIn('element.html', fast_html)
    self.assertNotIn('element.js', fast_html)
    self.assertNotIn('element_in_dir.html', fast_html)
    self.assertNotIn('element_in_dir.js', fast_html)
    self.assertIn('got here!', fast_html)
    self.assertIn('<script src="fast.js"></script>', fast_html)

    fast_js = self._read_out_file('fast.js')
    self.assertIn('yay', fast_js)
    self.assertIn('hello from element_in_dir', fast_js)

    depfile_d = self._read_out_file('depfile.d')
    self.assertIn('element.html', depfile_d)
    self.assertIn('element.js', depfile_d)
    self.assertIn(os.path.normpath('element_in_dir/element_in_dir.html'),
                  depfile_d)
    self.assertIn(os.path.normpath('element_in_dir/element_in_dir.js'),
                  depfile_d)


if __name__ == '__main__':
  unittest.main()
