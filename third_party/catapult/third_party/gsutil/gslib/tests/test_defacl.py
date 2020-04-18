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
"""Integration tests for the defacl command."""

from __future__ import absolute_import

import re

from gslib.cs_api_map import ApiSelector
import gslib.tests.testcase as case
from gslib.tests.testcase.integration_testcase import SkipForS3
from gslib.tests.util import ObjectToURI as suri

PUBLIC_READ_JSON_ACL_TEXT = '"entity":"allUsers","role":"READER"'


@SkipForS3('S3 does not support default object ACLs.')
class TestDefacl(case.GsUtilIntegrationTestCase):
  """Integration tests for the defacl command."""

  _defacl_ch_prefix = ['defacl', 'ch']
  _defacl_get_prefix = ['defacl', 'get']
  _defacl_set_prefix = ['defacl', 'set']

  def _MakeScopeRegex(self, role, entity_type, email_address):
    template_regex = (r'\{.*"entity":\s*"%s-%s".*"role":\s*"%s".*\}' %
                      (entity_type, email_address, role))
    return re.compile(template_regex, flags=re.DOTALL)

  def testChangeDefaultAcl(self):
    """Tests defacl ch."""
    bucket = self.CreateBucket()

    test_regex = self._MakeScopeRegex(
        'OWNER', 'group', self.GROUP_TEST_ADDRESS)
    test_regex2 = self._MakeScopeRegex(
        'READER', 'group', self.GROUP_TEST_ADDRESS)
    json_text = self.RunGsUtil(self._defacl_get_prefix +
                               [suri(bucket)], return_stdout=True)
    self.assertNotRegexpMatches(json_text, test_regex)

    self.RunGsUtil(self._defacl_ch_prefix +
                   ['-g', self.GROUP_TEST_ADDRESS+':FC', suri(bucket)])
    json_text2 = self.RunGsUtil(self._defacl_get_prefix +
                                [suri(bucket)], return_stdout=True)
    self.assertRegexpMatches(json_text2, test_regex)

    self.RunGsUtil(self._defacl_ch_prefix +
                   ['-g', self.GROUP_TEST_ADDRESS+':READ', suri(bucket)])
    json_text3 = self.RunGsUtil(self._defacl_get_prefix +
                                [suri(bucket)], return_stdout=True)
    self.assertRegexpMatches(json_text3, test_regex2)

    stderr = self.RunGsUtil(self._defacl_ch_prefix +
                            ['-g', self.GROUP_TEST_ADDRESS+':WRITE',
                             suri(bucket)],
                            return_stderr=True, expected_status=1)
    self.assertIn('WRITER cannot be set as a default object ACL', stderr)

  def testChangeDefaultAclEmpty(self):
    """Tests adding and removing an entry from an empty default object ACL."""

    bucket = self.CreateBucket()

    # First, clear out the default object ACL on the bucket.
    self.RunGsUtil(self._defacl_set_prefix + ['private', suri(bucket)])
    json_text = self.RunGsUtil(self._defacl_get_prefix +
                               [suri(bucket)], return_stdout=True)
    empty_regex = r'\[\]\s*'
    self.assertRegexpMatches(json_text, empty_regex)

    group_regex = self._MakeScopeRegex(
        'READER', 'group', self.GROUP_TEST_ADDRESS)
    self.RunGsUtil(self._defacl_ch_prefix +
                   ['-g', self.GROUP_TEST_ADDRESS+':READ', suri(bucket)])
    json_text2 = self.RunGsUtil(self._defacl_get_prefix +
                                [suri(bucket)], return_stdout=True)
    self.assertRegexpMatches(json_text2, group_regex)

    if self.test_api == ApiSelector.JSON:
      # TODO: Enable when JSON service respects creating a private (no entries)
      # default object ACL via PATCH. For now, only supported in XML.
      return

    # After adding and removing a group, the default object ACL should be empty.
    self.RunGsUtil(self._defacl_ch_prefix +
                   ['-d', self.GROUP_TEST_ADDRESS, suri(bucket)])
    json_text3 = self.RunGsUtil(self._defacl_get_prefix +
                                [suri(bucket)], return_stdout=True)
    self.assertRegexpMatches(json_text3, empty_regex)

  def testChangeMultipleBuckets(self):
    """Tests defacl ch on multiple buckets."""
    bucket1 = self.CreateBucket()
    bucket2 = self.CreateBucket()

    test_regex = self._MakeScopeRegex(
        'READER', 'group', self.GROUP_TEST_ADDRESS)
    json_text = self.RunGsUtil(self._defacl_get_prefix + [suri(bucket1)],
                               return_stdout=True)
    self.assertNotRegexpMatches(json_text, test_regex)
    json_text = self.RunGsUtil(self._defacl_get_prefix + [suri(bucket2)],
                               return_stdout=True)
    self.assertNotRegexpMatches(json_text, test_regex)

    self.RunGsUtil(self._defacl_ch_prefix +
                   ['-g', self.GROUP_TEST_ADDRESS+':READ',
                    suri(bucket1), suri(bucket2)])
    json_text = self.RunGsUtil(self._defacl_get_prefix + [suri(bucket1)],
                               return_stdout=True)
    self.assertRegexpMatches(json_text, test_regex)
    json_text = self.RunGsUtil(self._defacl_get_prefix + [suri(bucket2)],
                               return_stdout=True)
    self.assertRegexpMatches(json_text, test_regex)

  def testChangeMultipleAcls(self):
    """Tests defacl ch with multiple ACL entries."""
    bucket = self.CreateBucket()

    test_regex_group = self._MakeScopeRegex(
        'READER', 'group', self.GROUP_TEST_ADDRESS)
    test_regex_user = self._MakeScopeRegex(
        'OWNER', 'user', self.USER_TEST_ADDRESS)
    json_text = self.RunGsUtil(self._defacl_get_prefix + [suri(bucket)],
                               return_stdout=True)
    self.assertNotRegexpMatches(json_text, test_regex_group)
    self.assertNotRegexpMatches(json_text, test_regex_user)

    self.RunGsUtil(self._defacl_ch_prefix +
                   ['-g', self.GROUP_TEST_ADDRESS+':READ',
                    '-u', self.USER_TEST_ADDRESS+':fc', suri(bucket)])
    json_text = self.RunGsUtil(self._defacl_get_prefix + [suri(bucket)],
                               return_stdout=True)
    self.assertRegexpMatches(json_text, test_regex_group)
    self.assertRegexpMatches(json_text, test_regex_user)

  def testEmptyDefAcl(self):
    bucket = self.CreateBucket()
    self.RunGsUtil(self._defacl_set_prefix + ['private', suri(bucket)])
    stdout = self.RunGsUtil(self._defacl_get_prefix + [suri(bucket)],
                            return_stdout=True)
    self.assertEquals(stdout.rstrip(), '[]')
    self.RunGsUtil(self._defacl_ch_prefix +
                   ['-u', self.USER_TEST_ADDRESS+':fc', suri(bucket)])

  def testDeletePermissionsWithCh(self):
    """Tests removing permissions with defacl ch."""
    bucket = self.CreateBucket()

    test_regex = self._MakeScopeRegex(
        'OWNER', 'user', self.USER_TEST_ADDRESS)
    json_text = self.RunGsUtil(
        self._defacl_get_prefix + [suri(bucket)], return_stdout=True)
    self.assertNotRegexpMatches(json_text, test_regex)

    self.RunGsUtil(self._defacl_ch_prefix +
                   ['-u', self.USER_TEST_ADDRESS+':fc', suri(bucket)])
    json_text = self.RunGsUtil(
        self._defacl_get_prefix + [suri(bucket)], return_stdout=True)
    self.assertRegexpMatches(json_text, test_regex)

    self.RunGsUtil(self._defacl_ch_prefix +
                   ['-d', self.USER_TEST_ADDRESS, suri(bucket)])
    json_text = self.RunGsUtil(
        self._defacl_get_prefix + [suri(bucket)], return_stdout=True)
    self.assertNotRegexpMatches(json_text, test_regex)

  def testTooFewArgumentsFails(self):
    """Tests calling defacl with insufficient number of arguments."""
    # No arguments for get, but valid subcommand.
    stderr = self.RunGsUtil(self._defacl_get_prefix, return_stderr=True,
                            expected_status=1)
    self.assertIn('command requires at least', stderr)

    # No arguments for set, but valid subcommand.
    stderr = self.RunGsUtil(self._defacl_set_prefix, return_stderr=True,
                            expected_status=1)
    self.assertIn('command requires at least', stderr)

    # No arguments for ch, but valid subcommand.
    stderr = self.RunGsUtil(self._defacl_ch_prefix, return_stderr=True,
                            expected_status=1)
    self.assertIn('command requires at least', stderr)

    # Neither arguments nor subcommand.
    stderr = self.RunGsUtil(['defacl'], return_stderr=True, expected_status=1)
    self.assertIn('command requires at least', stderr)


class TestDefaclOldAlias(TestDefacl):
  _defacl_ch_prefix = ['chdefacl']
  _defacl_get_prefix = ['getdefacl']
  _defacl_set_prefix = ['setdefacl']
