#!/usr/bin/env python
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys
import unittest

import PRESUBMIT

sys.path.append(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))

from PRESUBMIT_test_mocks import (MockInputApi, MockOutputApi, MockAffectedFile)

class MetricsProtoCheckerTest(unittest.TestCase):

  def testModifiedWithoutReadme(self):
    input_api = MockInputApi()
    input_api.files = [MockAffectedFile('somefile.proto', 'some diff')]
    self.assertEqual(1, len(PRESUBMIT.CheckChange(input_api, MockOutputApi())))


  def testModifiedWithoutReadme(self):
    input_api = MockInputApi()
    input_api.files = [
      MockAffectedFile('somefile.proto', 'some diff'),
      MockAffectedFile(PRESUBMIT.README, 'some diff'),
    ]
    self.assertEqual(0, len(PRESUBMIT.CheckChange(input_api, MockOutputApi())))

if __name__ == '__main__':
    unittest.main()
