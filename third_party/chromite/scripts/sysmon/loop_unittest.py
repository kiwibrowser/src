# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for loop."""

# pylint: disable=protected-access

from __future__ import absolute_import
from __future__ import print_function

import contextlib

import mock

from chromite.lib import cros_test_lib
from chromite.scripts.sysmon import loop


class _MockTime(object):
  """Mock time and sleep.

  Provides mock behavior for time.time() and time.sleep()
  """

  def __init__(self, sleep_delta):
    """Instantiate instance.

    Args:
      sleep_delta: Modify sleep time by this many seconds.
                   But sleep will always be at least 1.
    """
    self.current_time = 0
    self._sleep_delta = sleep_delta

  def time(self):
    return self.current_time

  def sleep(self, secs):
    actual_sleep = max(secs + self._sleep_delta, 1)
    self.current_time += actual_sleep
    return actual_sleep


@contextlib.contextmanager
def _patch_time(sleep_delta):
  """Mock out time and sleep.

  Patches behavior for time.time() and time.sleep()
  """
  mock_time = _MockTime(sleep_delta)
  with mock.patch('time.time', mock_time.time), \
       mock.patch('time.sleep', mock_time.sleep):
    yield mock_time


class TestForceSleep(cros_test_lib.TestCase):
  """Tests for _force_sleep."""

  def test__force_sleep_at_least_given_secs(self):
    with _patch_time(sleep_delta=-7) as mock_time:
      loop._force_sleep(10)
    self.assertGreaterEqual(mock_time.current_time, 10)
