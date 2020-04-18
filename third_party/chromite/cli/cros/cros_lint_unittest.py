# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module tests the cros lint command."""

from __future__ import print_function

from chromite.cli.cros import cros_lint
from chromite.lib import cros_test_lib


class LintCommandTest(cros_test_lib.TestCase):
  """Test class for our LintCommand class."""

  def testOutputArgument(self):
    """Tests that the --output argument mapping for cpplint is complete."""
    self.assertEqual(
        set(cros_lint.LintCommand.OUTPUT_FORMATS),
        set(cros_lint.CPPLINT_OUTPUT_FORMAT_MAP.keys() + ['default']))
