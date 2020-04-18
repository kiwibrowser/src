#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import unittest

from environment import GetAppVersionNonMemoized


class EnvironmentTest(unittest.TestCase):
  def testGetAppVersion(self):
    # GetAppVersion uses 2 heuristics: the CURRENT_VERSION_ID environment
    # variable that AppEngine sets, or the version extracted from app.yaml
    # if no such variable exists (e.g. preview.py). The latter, we assume,
    # is already tested because AppYamlHelper.ExtractVersion is already
    # tested. So, for this test, we fake a CURRENT_VERSION_ID.
    def test_single(expected, current_version_id):
      key = 'CURRENT_VERSION_ID'
      old_value = os.environ.get(key)
      os.environ[key] = current_version_id
      try:
        self.assertEqual(expected, GetAppVersionNonMemoized())
      finally:
        if old_value is None:
          del os.environ[key]
        else:
          os.environ[key] = old_value
    def test_all(expected):
      test_single(expected, expected)
      test_single(expected, expected + '.48w7dl48wl')
      test_single(expected, expected + '/48w7dl48wl')
      test_single(expected, expected + '.48w7dl48wl.w847lw83')
      test_single(expected, expected + '.48w7dl48wl/w847lw83')
      test_single(expected, expected + '/48w7dl48wl.w847lw83')
      test_single(expected, expected + '/48w7dl48wl/w847lw83')
    test_all('2')
    test_all('2-0')
    test_all('2-0-25')
    test_all('2-0-25-b')


if __name__ == '__main__':
  unittest.main()
