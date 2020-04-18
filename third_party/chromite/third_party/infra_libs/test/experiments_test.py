# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import mock

from infra_libs import experiments


class ExperimentsTest(unittest.TestCase):
  def setUp(self):
    self.mock_getfqdn = mock.patch('socket.getfqdn').start()

  def tearDown(self):
    mock.patch.stopall()

  def test_is_active_for_host(self):
    self.mock_getfqdn.return_value = 'vm122-m1'

    self.assertFalse(experiments.is_active_for_host('f1', 0))
    self.assertFalse(experiments.is_active_for_host('f2', 0))
    self.assertFalse(experiments.is_active_for_host('f3', 0))

    self.assertTrue(experiments.is_active_for_host('f1', 50))
    self.assertFalse(experiments.is_active_for_host('f2', 50))
    self.assertTrue(experiments.is_active_for_host('f3', 50))

    self.assertTrue(experiments.is_active_for_host('f1', 100))
    self.assertTrue(experiments.is_active_for_host('f2', 100))
    self.assertTrue(experiments.is_active_for_host('f3', 100))

    self.mock_getfqdn.return_value = 'vm124-m1'

    self.assertFalse(experiments.is_active_for_host('f1', 0))
    self.assertFalse(experiments.is_active_for_host('f2', 0))
    self.assertFalse(experiments.is_active_for_host('f3', 0))

    self.assertTrue(experiments.is_active_for_host('f1', 50))
    self.assertTrue(experiments.is_active_for_host('f2', 50))
    self.assertFalse(experiments.is_active_for_host('f3', 50))

    self.assertTrue(experiments.is_active_for_host('f1', 100))
    self.assertTrue(experiments.is_active_for_host('f2', 100))
    self.assertTrue(experiments.is_active_for_host('f3', 100))
