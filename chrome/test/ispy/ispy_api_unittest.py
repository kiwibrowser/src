#!/usr/bin/env python
#
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import unittest
from PIL import Image

import ispy_api
from common import cloud_bucket
from common import mock_cloud_bucket


class ISpyApiTest(unittest.TestCase):
  """Unittest for the ISpy API."""

  def setUp(self):
    self.cloud_bucket = mock_cloud_bucket.MockCloudBucket()
    self.ispy = ispy_api.ISpyApi(self.cloud_bucket)
    self.white_img = Image.new('RGBA', (10, 10), (255, 255, 255, 255))
    self.black_img = Image.new('RGBA', (10, 10), (0, 0, 0, 255))

  def testGenerateExpectationsRunComparison(self):
    self.ispy.GenerateExpectation(
        'device', 'test', '1.1.1.1', 'versions.json',
        [self.white_img, self.white_img])
    self.ispy.UpdateExpectationVersion('1.1.1.1', 'versions.json')
    self.ispy.PerformComparison(
        'test1', 'device', 'test', '1.1.1.1', 'versions.json', self.white_img)
    expect_name = self.ispy._CreateExpectationName(
        'device', 'test', '1.1.1.1')
    self.assertFalse(self.ispy._ispy.FailureExists('test1', expect_name))
    self.ispy.PerformComparison(
        'test2', 'device', 'test', '1.1.1.1','versions.json', self.black_img)
    self.assertTrue(self.ispy._ispy.FailureExists('test2', expect_name))

  def testUpdateExpectationVersion(self):
    self.ispy.UpdateExpectationVersion('1.0.0.0', 'versions.json')
    self.ispy.UpdateExpectationVersion('1.0.4.0', 'versions.json')
    self.ispy.UpdateExpectationVersion('2.1.5.0', 'versions.json')
    self.ispy.UpdateExpectationVersion('1.1.5.0', 'versions.json')
    self.ispy.UpdateExpectationVersion('0.0.0.0', 'versions.json')
    self.ispy.UpdateExpectationVersion('1.1.5.0', 'versions.json')
    self.ispy.UpdateExpectationVersion('0.0.0.1', 'versions.json')
    versions = json.loads(self.cloud_bucket.DownloadFile('versions.json'))
    self.assertEqual(versions,
        ['2.1.5.0', '1.1.5.0', '1.0.4.0', '1.0.0.0', '0.0.0.1', '0.0.0.0'])

  def testPerformComparisonAndPrepareExpectation(self):
    self.assertFalse(self.ispy.CanRebaselineToTestRun('test'))
    self.assertRaises(
        cloud_bucket.FileNotFoundError,
        self.ispy.PerformComparisonAndPrepareExpectation,
        'test', 'device', 'expect', '1.0', 'versions.json',
        [self.white_img, self.white_img])
    self.assertTrue(self.ispy.CanRebaselineToTestRun('test'))
    self.ispy.RebaselineToTestRun('test')
    versions = json.loads(self.cloud_bucket.DownloadFile('versions.json'))
    self.assertEqual(versions, ['1.0'])
    self.ispy.PerformComparisonAndPrepareExpectation(
        'test1', 'device', 'expect', '1.1', 'versions.json',
        [self.white_img, self.white_img])


if __name__ == '__main__':
  unittest.main()
