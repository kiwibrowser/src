# -*- coding: utf-8 -*-
# Copyright 2013 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Integration tests for the webcfg command."""

from __future__ import absolute_import

import json
import gslib.tests.testcase as testcase
from gslib.tests.testcase.integration_testcase import SkipForS3
from gslib.tests.util import ObjectToURI as suri

WEBCFG_FULL = json.loads('{"notFoundPage": "404", "mainPageSuffix": "main"}\n')
WEBCFG_MAIN = json.loads('{"mainPageSuffix": "main"}\n')
WEBCFG_ERROR = json.loads('{"notFoundPage": "404"}\n')
WEBCFG_EMPTY = 'has no website configuration'


@SkipForS3('Web set not supported for S3, web get returns XML.')
class TestWeb(testcase.GsUtilIntegrationTestCase):
  """Integration tests for the web command."""

  _set_web_cmd = ['web', 'set']
  _get_web_cmd = ['web', 'get']

  def test_full(self):
    bucket_uri = self.CreateBucket()
    self.RunGsUtil(
        self._set_web_cmd + ['-m', 'main', '-e', '404', suri(bucket_uri)])
    stdout = self.RunGsUtil(
        self._get_web_cmd + [suri(bucket_uri)], return_stdout=True)
    self.assertEquals(json.loads(stdout), WEBCFG_FULL)

  def test_main(self):
    bucket_uri = self.CreateBucket()
    self.RunGsUtil(self._set_web_cmd + ['-m', 'main', suri(bucket_uri)])
    stdout = self.RunGsUtil(
        self._get_web_cmd + [suri(bucket_uri)], return_stdout=True)
    self.assertEquals(json.loads(stdout), WEBCFG_MAIN)

  def test_error(self):
    bucket_uri = self.CreateBucket()
    self.RunGsUtil(self._set_web_cmd + ['-e', '404', suri(bucket_uri)])
    stdout = self.RunGsUtil(
        self._get_web_cmd + [suri(bucket_uri)], return_stdout=True)
    self.assertEquals(json.loads(stdout), WEBCFG_ERROR)

  def test_empty(self):
    bucket_uri = self.CreateBucket()
    self.RunGsUtil(self._set_web_cmd + [suri(bucket_uri)])
    stdout = self.RunGsUtil(
        self._get_web_cmd + [suri(bucket_uri)], return_stdout=True)
    self.assertIn(WEBCFG_EMPTY, stdout)

  def testTooFewArgumentsFails(self):
    """Ensures web commands fail with too few arguments."""
    # No arguments for get, but valid subcommand.
    stderr = self.RunGsUtil(self._get_web_cmd, return_stderr=True,
                            expected_status=1)
    self.assertIn('command requires at least', stderr)

    # No arguments for set, but valid subcommand.
    stderr = self.RunGsUtil(self._set_web_cmd, return_stderr=True,
                            expected_status=1)
    self.assertIn('command requires at least', stderr)

    # Neither arguments nor subcommand.
    stderr = self.RunGsUtil(['web'], return_stderr=True, expected_status=1)
    self.assertIn('command requires at least', stderr)


class TestWebOldAlias(TestWeb):
  _set_web_cmd = ['setwebcfg']
  _get_web_cmd = ['getwebcfg']
