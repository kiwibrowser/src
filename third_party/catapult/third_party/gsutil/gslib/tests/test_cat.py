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
"""Tests for cat command."""

from __future__ import absolute_import

from gslib.cs_api_map import ApiSelector
from gslib.exception import NO_URLS_MATCHED_TARGET
import gslib.tests.testcase as testcase
from gslib.tests.testcase.integration_testcase import SkipForS3
from gslib.tests.util import GenerationFromURI as urigen
from gslib.tests.util import ObjectToURI as suri
from gslib.tests.util import RUN_S3_TESTS
from gslib.tests.util import SetBotoConfigForTest
from gslib.tests.util import TEST_ENCRYPTION_KEY1
from gslib.tests.util import unittest


class TestCat(testcase.GsUtilIntegrationTestCase):
  """Integration tests for cat command."""

  def test_cat_range(self):
    """Tests cat command with various range arguments."""
    key_uri = self.CreateObject(contents='0123456789')
    # Test various invalid ranges.
    stderr = self.RunGsUtil(['cat', '-r -', suri(key_uri)],
                            return_stderr=True, expected_status=1)
    self.assertIn('Invalid range', stderr)
    stderr = self.RunGsUtil(['cat', '-r a-b', suri(key_uri)],
                            return_stderr=True, expected_status=1)
    self.assertIn('Invalid range', stderr)
    stderr = self.RunGsUtil(['cat', '-r 1-2-3', suri(key_uri)],
                            return_stderr=True, expected_status=1)
    self.assertIn('Invalid range', stderr)
    stderr = self.RunGsUtil(['cat', '-r 1.7-3', suri(key_uri)],
                            return_stderr=True, expected_status=1)
    self.assertIn('Invalid range', stderr)

    # Test various valid ranges.
    stdout = self.RunGsUtil(['cat', '-r 1-3', suri(key_uri)],
                            return_stdout=True)
    self.assertEqual('123', stdout)
    stdout = self.RunGsUtil(['cat', '-r 8-', suri(key_uri)],
                            return_stdout=True)
    self.assertEqual('89', stdout)
    stdout = self.RunGsUtil(['cat', '-r 0-0', suri(key_uri)],
                            return_stdout=True)
    self.assertEqual('0', stdout)
    stdout = self.RunGsUtil(['cat', '-r -3', suri(key_uri)],
                            return_stdout=True)
    self.assertEqual('789', stdout)

  def test_cat_version(self):
    """Tests cat command on versioned objects."""
    bucket_uri = self.CreateVersionedBucket()
    # Create 2 versions of an object.
    uri1 = self.CreateObject(bucket_uri=bucket_uri, contents='data1',
                             gs_idempotent_generation=0)
    uri2 = self.CreateObject(bucket_uri=bucket_uri,
                             object_name=uri1.object_name, contents='data2',
                             gs_idempotent_generation=urigen(uri1))
    stdout = self.RunGsUtil(['cat', suri(uri1)], return_stdout=True)
    # Last version written should be live.
    self.assertEqual('data2', stdout)
    # Using either version-specific URI should work.
    stdout = self.RunGsUtil(['cat', uri1.version_specific_uri],
                            return_stdout=True)
    self.assertEqual('data1', stdout)
    stdout = self.RunGsUtil(['cat', uri2.version_specific_uri],
                            return_stdout=True)
    self.assertEqual('data2', stdout)
    if RUN_S3_TESTS:
      # S3 GETs of invalid versions return 400s.
      # Also, appending between 1 and 3 characters to the version_id can
      # result in a success (200) response from the server.
      stderr = self.RunGsUtil(['cat', uri2.version_specific_uri + '23456'],
                              return_stderr=True, expected_status=1)
      self.assertIn('BadRequestException: 400', stderr)
    else:
      # Attempting to cat invalid version should result in an error.
      stderr = self.RunGsUtil(['cat', uri2.version_specific_uri + '23'],
                              return_stderr=True, expected_status=1)
      self.assertIn(NO_URLS_MATCHED_TARGET % uri2.version_specific_uri + '23',
                    stderr)

  def test_cat_multi_arg(self):
    """Tests cat command with multiple arguments."""
    bucket_uri = self.CreateBucket()
    data1 = '0123456789'
    data2 = 'abcdefghij'
    obj_uri1 = self.CreateObject(bucket_uri=bucket_uri, contents=data1)
    obj_uri2 = self.CreateObject(bucket_uri=bucket_uri, contents=data2)
    stdout, stderr = self.RunGsUtil(
        ['cat', suri(obj_uri1), suri(bucket_uri) + 'nonexistent'],
        return_stdout=True, return_stderr=True, expected_status=1)
    # First object should print, second should produce an exception.
    self.assertIn(data1, stdout)
    self.assertIn('NotFoundException', stderr)

    stdout, stderr = self.RunGsUtil(
        ['cat', suri(bucket_uri) + 'nonexistent', suri(obj_uri1)],
        return_stdout=True, return_stderr=True, expected_status=1)

    # If first object is invalid, exception should halt output immediately.
    self.assertNotIn(data1, stdout)
    self.assertIn('NotFoundException', stderr)

    # Two valid objects should both print successfully.
    stdout = self.RunGsUtil(['cat', suri(obj_uri1), suri(obj_uri2)],
                            return_stdout=True)
    self.assertIn(data1 + data2, stdout)

  @SkipForS3('S3 customer-supplied encryption keys are not supported.')
  def test_cat_encrypted_object(self):
    if self.test_api == ApiSelector.XML:
      return unittest.skip(
          'gsutil does not support encryption with the XML API')
    object_contents = '0123456789'
    object_uri = self.CreateObject(object_name='foo', contents=object_contents,
                                   encryption_key=TEST_ENCRYPTION_KEY1)

    stderr = self.RunGsUtil(['cat', suri(object_uri)], expected_status=1,
                            return_stderr=True)
    self.assertIn('No decryption key matches object', stderr)

    boto_config_for_test = [('GSUtil', 'encryption_key', TEST_ENCRYPTION_KEY1)]

    with SetBotoConfigForTest(boto_config_for_test):
      stdout = self.RunGsUtil(['cat', suri(object_uri)], return_stdout=True)
      self.assertEqual(stdout, object_contents)
      stdout = self.RunGsUtil(['cat', '-r 1-3', suri(object_uri)],
                              return_stdout=True)
      self.assertEqual(stdout, '123')
