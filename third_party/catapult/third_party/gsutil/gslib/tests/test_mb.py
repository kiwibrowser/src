# -*- coding: utf-8 -*-
# Copyright 2014 Google Inc. All Rights Reserved.
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
"""Integration tests for mb command."""

from __future__ import absolute_import

import gslib.tests.testcase as testcase
from gslib.tests.testcase.integration_testcase import SkipForS3
from gslib.tests.util import ObjectToURI as suri


class TestMb(testcase.GsUtilIntegrationTestCase):
  """Integration tests for mb command."""

  @SkipForS3('S3 returns success when bucket already exists.')
  def test_mb_bucket_exists(self):
    bucket_uri = self.CreateBucket()
    stderr = self.RunGsUtil(['mb', suri(bucket_uri)], expected_status=1,
                            return_stderr=True)
    self.assertIn('already exists', stderr)

  def test_non_ascii_project_fails(self):
    stderr = self.RunGsUtil(['ls', '-p', 'ã', 'gs://fobarbaz'],
                            expected_status=1,
                            return_stderr=True)
    self.assertIn('Invalid non-ASCII', stderr)

