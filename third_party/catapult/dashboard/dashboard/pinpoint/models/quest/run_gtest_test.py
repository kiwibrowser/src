# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from dashboard.pinpoint.models.quest import run_gtest
from dashboard.pinpoint.models.quest import run_test


_BASE_ARGUMENTS = {
    'swarming_server': 'server',
    'dimensions': {'key': 'value'},
}


_BASE_EXTRA_ARGS = ['--gtest_repeat=1'] + run_test._DEFAULT_EXTRA_ARGS


class FromDictTest(unittest.TestCase):

  def testMinimumArguments(self):
    quest = run_gtest.RunGTest.FromDict(_BASE_ARGUMENTS)
    expected = run_gtest.RunGTest('server', {'key': 'value'}, _BASE_EXTRA_ARGS)
    self.assertEqual(quest, expected)

  def testAllArguments(self):
    arguments = dict(_BASE_ARGUMENTS)
    arguments['test'] = 'test_name'
    quest = run_gtest.RunGTest.FromDict(arguments)

    extra_args = ['--gtest_filter=test_name'] + _BASE_EXTRA_ARGS
    expected = run_gtest.RunGTest('server', {'key': 'value'}, extra_args)
    self.assertEqual(quest, expected)
