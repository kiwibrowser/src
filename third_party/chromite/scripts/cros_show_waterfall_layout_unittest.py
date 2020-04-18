# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for cros_show_waterfall_layout."""

from __future__ import print_function

import json
import os

from chromite.lib import constants
from chromite.lib import cros_test_lib
from chromite.scripts import cros_show_waterfall_layout

# pylint: disable=protected-access


class JsonDumpTest(cros_test_lib.OutputTestCase):
  """Test the json dumping functionality of cbuildbot_view_config."""

  def setUp(self):
    bin_name = os.path.basename(__file__).rstrip('_unittest.py')
    self.bin_path = os.path.join(constants.CHROMITE_BIN_DIR, bin_name)

  def testJSONDumpLoadable(self):
    """Make sure config export functionality works."""
    with self.OutputCapturer() as output:
      cros_show_waterfall_layout.main(['--format', 'json'])
      layout = json.loads(output.GetStdout())
    self.assertFalse(not layout)

  def testTextDump(self):
    """Make sure text dumping is capable of being produced."""
    with self.OutputCapturer() as output:
      cros_show_waterfall_layout.main(['--format', 'text'])
    self.assertFalse(not output.GetStdout())
