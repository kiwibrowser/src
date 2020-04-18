# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for cros_list_overlays.py"""

from __future__ import print_function

from chromite.lib import cros_test_lib
from chromite.lib import portage_util
from chromite.scripts import cros_list_overlays


class ListOverlaysTest(cros_test_lib.MockTestCase):
  """Tests for main()"""

  def setUp(self):
    self.pfind_mock = self.PatchObject(portage_util, 'FindPrimaryOverlay')
    self.find_mock = self.PatchObject(portage_util, 'FindOverlays')

  def testSmoke(self):
    """Basic sanity check"""
    cros_list_overlays.main([])

  def testPrimary(self):
    """Basic primary check"""
    cros_list_overlays.main(['--primary_only', '--board', 'foo'])


def main(_argv):
  cros_test_lib.main(level='info', module=__name__)
