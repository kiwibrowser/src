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
"""Integration tests for top-level gsutil command."""

from __future__ import absolute_import

import gslib
import gslib.tests.testcase as testcase


class TestGsUtil(testcase.GsUtilIntegrationTestCase):
  """Integration tests for top-level gsutil command."""

  def test_long_version_arg(self):
    stdout = self.RunGsUtil(['--version'], return_stdout=True)
    self.assertEqual('gsutil version: %s\n' % gslib.VERSION, stdout)

  def test_version_command(self):
    stdout = self.RunGsUtil(['version'], return_stdout=True)
    self.assertEqual('gsutil version: %s\n' % gslib.VERSION, stdout)

  def test_version_long(self):
    stdout = self.RunGsUtil(['version', '-l'], return_stdout=True)
    self.assertIn('gsutil version: %s\n' % gslib.VERSION, stdout)
    self.assertIn('boto version', stdout)
    self.assertIn('checksum', stdout)
    self.assertIn('config path', stdout)
    self.assertIn('gsutil path', stdout)
