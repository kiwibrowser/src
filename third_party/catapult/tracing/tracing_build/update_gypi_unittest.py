# Copyright (c) 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from tracing_build.update_gypi import GypiFile


class UpdateGypiTests(unittest.TestCase):

  def setUp(self):
    self.file_groups = ['group1', 'group2']

  def testGypiTokenizer(self):
    content = ("useless data\n'group1': [\n    <file list goes here>\n"
               "    ]\nNote the four spaces before the ] above")
    gypi_files = GypiFile(content, self.file_groups)
    self.assertEqual(3, len(gypi_files._tokens))
    self.assertEqual('plain', gypi_files._tokens[0].token_id)
    self.assertEqual(
        "useless data\n'group1': [\n", gypi_files._tokens[0].data)
    self.assertEqual('group1', gypi_files._tokens[1].token_id)
    self.assertEqual("    <file list goes here>\n", gypi_files._tokens[1].data)
    self.assertEqual('plain', gypi_files._tokens[2].token_id)
    self.assertEqual(
        "    ]\nNote the four spaces before the ] above",
        gypi_files._tokens[2].data)

  def testGypiFileListBuilder(self):
    gypi_file = GypiFile('', self.file_groups)
    existing_list = ("    '/four/spaces/indent',\n'"
                     "    '/five/spaces/but/only/first/line/matters',\n")
    new_list = ['item1', 'item2', 'item3']
    self.assertEqual(
        "    'item1',\n    'item2',\n    'item3',\n",
        gypi_file._GetReplacementListAsString(existing_list, new_list))
