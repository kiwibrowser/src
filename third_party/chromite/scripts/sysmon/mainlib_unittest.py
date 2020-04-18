# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for git_metrics."""

# pylint: disable=protected-access

from __future__ import absolute_import
from __future__ import print_function

import mock

from chromite.lib import cros_test_lib
from chromite.scripts.sysmon import mainlib


class TestTimedCallback(cros_test_lib.TestCase):
  """Tests for _TimedCallback."""

  def setUp(self):
    patcher = mock.patch('time.time', autospec=True)
    self.time = patcher.start()
    self.addCleanup(patcher.stop)

  def test_initial_call_should_callback(self):
    """Test that initial call goes through."""
    cb = mock.Mock([])

    self.time.return_value = 0
    obj = mainlib._TimedCallback(cb, 10)

    obj()
    cb.assert_called_once()

  def test_call_within_interval_should_not_callback(self):
    """Test that call too soon does not callback."""
    cb = mock.Mock([])

    self.time.return_value = 0
    obj = mainlib._TimedCallback(cb, 10)

    obj()
    cb.assert_called_once()

    cb.reset_mock()
    obj()
    cb.assert_not_called()

  def test_call_after_interval_should_callback(self):
    """Test that later call does callback."""
    cb = mock.Mock([])

    self.time.return_value = 0
    obj = mainlib._TimedCallback(cb, 10)

    obj()
    cb.assert_called_once()

    self.time.return_value = 10
    cb.reset_mock()
    obj()
    cb.assert_called_once()
