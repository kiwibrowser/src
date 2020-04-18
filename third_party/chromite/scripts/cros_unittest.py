# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for cros."""

from __future__ import print_function

from chromite.lib import commandline
from chromite.lib import cros_test_lib
from chromite.scripts import cros


class RunScriptTest(cros_test_lib.MockTempDirTestCase):
  """Test the main functionality."""

  def setUp(self):
    self.PatchObject(cros, '_RunSubCommand', autospec=True)

  def testDefaultLogLevel(self):
    """Test that the default log level is set to notice."""
    arg_parser = self.PatchObject(commandline, 'ArgumentParser',
                                  return_value=commandline.ArgumentParser())
    cros.GetOptions({})
    arg_parser.assert_called_with(caching=True, default_log_level='notice')
