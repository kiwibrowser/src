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
"""Tests for various combinations of configured credentials."""

from gslib.cred_types import CredTypes
from gslib.exception import CommandException
from gslib.gcs_json_api import GcsJsonApi
from gslib.tests.mock_logging_handler import MockLoggingHandler
import gslib.tests.testcase as testcase
from gslib.tests.testcase.integration_testcase import SkipForS3
from gslib.tests.util import ObjectToURI as suri
from gslib.tests.util import SetBotoConfigForTest
from gslib.util import DiscardMessagesQueue


class TestCredsConfig(testcase.GsUtilUnitTestCase):
  """Tests for various combinations of configured credentials."""

  def setUp(self):
    super(TestCredsConfig, self).setUp()
    self.log_handler = MockLoggingHandler()
    self.logger.addHandler(self.log_handler)

  def testMultipleConfiguredCreds(self):
    with SetBotoConfigForTest([
        ('Credentials', 'gs_oauth2_refresh_token', 'foo'),
        ('Credentials', 'gs_service_client_id', 'bar'),
        ('Credentials', 'gs_service_key_file', 'baz')]):

      try:
        GcsJsonApi(None, self.logger, DiscardMessagesQueue())
        self.fail('Succeeded with multiple types of configured creds.')
      except CommandException, e:
        msg = str(e)
        self.assertIn('types of configured credentials', msg)
        self.assertIn(CredTypes.OAUTH2_USER_ACCOUNT, msg)
        self.assertIn(CredTypes.OAUTH2_SERVICE_ACCOUNT, msg)


class TestCredsConfigIntegration(testcase.GsUtilIntegrationTestCase):

  @SkipForS3('Tests only uses gs credentials.')
  def testExactlyOneInvalid(self):
    bucket_uri = self.CreateBucket()
    with SetBotoConfigForTest([
        ('Credentials', 'gs_oauth2_refresh_token', 'foo'),
        ('Credentials', 'gs_service_client_id', None),
        ('Credentials', 'gs_service_key_file', None)],
                              use_existing_config=False):
      stderr = self.RunGsUtil(['ls', suri(bucket_uri)], expected_status=1,
                              return_stderr=True)
      self.assertIn('credentials are invalid', stderr)
