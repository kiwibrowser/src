# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for cts_helper module."""

from __future__ import print_function

import os
import osutils
from chromite.lib import cts_helper
from chromite.lib import cros_test_lib


class CtsHelperTestCase(cros_test_lib.MockTestCase):
  """Tests for functions that do not interact with the file system."""

  def testIsCtsTest(self):
    self.assertTrue(cts_helper.isCtsTest('cheets_CTS_N.arm.all'))
    self.assertTrue(cts_helper.isCtsTest('cheets_CTS_N.x86.all'))
    self.assertTrue(cts_helper.isCtsTest('cheets_GTS.all'))
    self.assertFalse(cts_helper.isCtsTest('cheets_GTS'))
    self.assertFalse(cts_helper.isCtsTest('cheets_CTS'))

  def testGetXMLPattern(self):
    self.assertEqual('test_result.xml',
                     cts_helper.getXMLPattern('cheets_CTS_P.arm.all'))
    self.assertEqual('test_result.xml',
                     cts_helper.getXMLPattern('cheets_GTS.arm.all'))

class UnmockedTests(cros_test_lib.TempDirTestCase):
  """Tests for functions which interact with the file system."""
  def testGetApfeFiles(self):
    results_path = os.path.join(self.tempdir, 'tmp')
    os.makedirs(results_path)

    test_folder = os.path.join(results_path, 'test_folder')
    file1 = os.path.join(test_folder, 'cheets_CTS_N', 'results', 'android-cts',
                         '2017.10.03_10.43.10.zip')
    file2 = os.path.join(test_folder, 'cheets_CTS_N', 'results', 'android-cts',
                         '2017.10.03_10.43.22.zip')
    osutils.WriteFile(file1, '', makedirs=True)
    osutils.WriteFile(file2, '', makedirs=True)

    self.assertEqual(
        set(cts_helper.getApfeFiles('cheets_CTS_N.Audio', test_folder)),
        set([file1, file2]))

  def testGetXMLGZFiles1(self):
    results_path = os.path.join(self.tempdir, 'tmp')
    os.makedirs(results_path)

    test_folder = os.path.join(results_path, 'test_folder')
    file1 = os.path.join(test_folder, 'cheets_CTS_N', 'results', 'android-cts',
                         '2017.10.03_10.43.22', 'test_result.xml')
    file2 = os.path.join(test_folder, 'cheets_CTS_N', 'results', 'android-cts',
                         '2017.10.03_10.43.24', 'test_result.xml')
    osutils.WriteFile(file1, '', makedirs=True)
    osutils.WriteFile(file2, '', makedirs=True)

    self.assertEqual(set(cts_helper.getXMLGZFiles('cheets_CTS_N', test_folder)),
                     set([file1+'.gz', file2+'.gz']))
