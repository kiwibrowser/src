# -*- coding: utf-8 -*-
# Copyright 2014 Google Inc.  All Rights Reserved.
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
# limitations under the License.   .

"""Unit tests for storage URLs."""

from __future__ import absolute_import


from gslib.storage_url import IsFileUrlString
from gslib.storage_url import StorageUrlFromString
import gslib.tests.testcase as testcase


class TestStorageUrl(testcase.GsUtilUnitTestCase):
  """Unit tests for storage URLs."""

  def setUp(self):
    super(TestStorageUrl, self).setUp()

  def test_is_file_url_string(self):
    self.assertTrue(IsFileUrlString('abc'))
    self.assertTrue(IsFileUrlString('file://abc'))
    self.assertFalse(IsFileUrlString('gs://abc'))
    self.assertFalse(IsFileUrlString('s3://abc'))

  def test_storage_url_from_string(self):
    storage_url = StorageUrlFromString('abc')
    self.assertTrue(storage_url.IsFileUrl())
    self.assertEquals('abc', storage_url.object_name)

    storage_url = StorageUrlFromString('file://abc/123')
    self.assertTrue(storage_url.IsFileUrl())
    self.assertEquals('abc/123', storage_url.object_name)

    storage_url = StorageUrlFromString('gs://abc/123')
    self.assertTrue(storage_url.IsCloudUrl())
    self.assertEquals('abc', storage_url.bucket_name)
    self.assertEquals('123', storage_url.object_name)

    storage_url = StorageUrlFromString('s3://abc/123')
    self.assertTrue(storage_url.IsCloudUrl())
    self.assertEquals('abc', storage_url.bucket_name)
    self.assertEquals('123', storage_url.object_name)
