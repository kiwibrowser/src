# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for cros_logging."""

from __future__ import print_function

import sys

from chromite.lib import cros_logging as logging
from chromite.lib import cros_test_lib


class CrosloggingTest(cros_test_lib.OutputTestCase):
  """Test logging works as expected."""

  def setUp(self):
    self.logger = logging.getLogger()
    sh = logging.StreamHandler(sys.stdout)
    self.logger.addHandler(sh)

  def AssertLogContainsMsg(self, msg, functor, *args, **kwargs):
    """Asserts that calling functor logs a line that contains msg.

    Args:
      msg: The message to look for.
      functor: A function taking no arguments to test.
      *args, **kwargs: passthrough arguments to AssertLogContainsMsg.
    """
    with self.OutputCapturer():
      functor()
    self.AssertOutputContainsLine(msg, *args, **kwargs)

  def testNotice(self):
    """Test logging.notice works and is between INFO and WARNING."""
    msg = 'notice message'
    self.logger.setLevel(logging.INFO)
    self.AssertLogContainsMsg(msg, lambda: logging.notice(msg))
    self.logger.setLevel(logging.WARNING)
    self.AssertLogContainsMsg(msg, lambda: logging.notice(msg), invert=True)

  def testPrintBuildbotFunctionsNoMarker(self):
    """PrintBuildbot* without markers should not be recognized by buildbot."""
    self.AssertLogContainsMsg('@@@STEP_LINK@',
                              lambda: logging.PrintBuildbotLink('name', 'url'),
                              check_stderr=True, invert=True)
    self.AssertLogContainsMsg('@@@@STEP_TEXT@',
                              lambda: logging.PrintBuildbotStepText('text'),
                              check_stderr=True, invert=True)
    self.AssertLogContainsMsg('@@@STEP_WARNINGS@@@',
                              logging.PrintBuildbotStepWarnings,
                              check_stderr=True, invert=True)
    self.AssertLogContainsMsg('@@@STEP_FAILURE@@@',
                              logging.PrintBuildbotStepFailure,
                              check_stderr=True, invert=True)
    self.AssertLogContainsMsg('@@@BUILD_STEP',
                              lambda: logging.PrintBuildbotStepName('name'),
                              check_stderr=True, invert=True)
    self.AssertLogContainsMsg(
        '@@@SET_BUILD_PROPERTY',
        lambda: logging.PrintBuildbotSetBuildProperty('name', {'a': 'value'}),
        check_stderr=True, invert=True)

  def testPrintBuildbotFunctionsWithMarker(self):
    """PrintBuildbot* with markers should be recognized by buildbot."""
    logging.EnableBuildbotMarkers()
    self.AssertLogContainsMsg('@@@STEP_LINK@name@url@@@',
                              lambda: logging.PrintBuildbotLink('name', 'url'),
                              check_stderr=True)
    self.AssertLogContainsMsg('@@@STEP_TEXT@text@@@',
                              lambda: logging.PrintBuildbotStepText('text'),
                              check_stderr=True)
    self.AssertLogContainsMsg('@@@STEP_WARNINGS@@@',
                              logging.PrintBuildbotStepWarnings,
                              check_stderr=True)
    self.AssertLogContainsMsg('@@@STEP_FAILURE@@@',
                              logging.PrintBuildbotStepFailure,
                              check_stderr=True)
    self.AssertLogContainsMsg('@@@BUILD_STEP@name@@@',
                              lambda: logging.PrintBuildbotStepName('name'),
                              check_stderr=True)
    self.AssertLogContainsMsg(
        '@@@SET_BUILD_PROPERTY@name@"value"@@@',
        lambda: logging.PrintBuildbotSetBuildProperty('name', 'value'),
        check_stderr=True)
