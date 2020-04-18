# -*- coding: utf-8 -*-
# Copyright 2018 Google Inc. All Rights Reserved.
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
"""Integration tests for kms command."""

from random import randint

from gslib.project_id import PopulateProjectId
import gslib.tests.testcase as testcase
from gslib.tests.testcase.integration_testcase import SkipForJSON
from gslib.tests.testcase.integration_testcase import SkipForS3
from gslib.tests.testcase.integration_testcase import SkipForXML
from gslib.tests.util import ObjectToURI as suri
from gslib.tests.util import SetBotoConfigForTest
from gslib.util import Retry


@SkipForS3('gsutil does not support KMS operations for S3 buckets.')
@SkipForXML('gsutil does not support KMS operations for S3 buckets.')
class TestKmsSuccessCases(testcase.GsUtilIntegrationTestCase):
  """Integration tests for the kms command."""

  def setUp(self):
    super(TestKmsSuccessCases, self).setUp()
    # Make sure our keyRing exists (only needs to be done once, but subsequent
    # attempts will receive a 409 and be treated as a success). Save the fully
    # qualified name for use with creating keys later.
    self.keyring_fqn = self.kms_api.CreateKeyRing(
        PopulateProjectId(None), testcase.KmsTestingResources.KEYRING_NAME,
        location=testcase.KmsTestingResources.KEYRING_LOCATION)

  @Retry(AssertionError, tries=3, timeout_secs=1)
  def DoTestAuthorize(self, specified_project=None):
    # Randomly pick 1 of 1000 key names.
    key_name = testcase.KmsTestingResources.MUTABLE_KEY_NAME_TEMPLATE % (
        randint(0, 9), randint(0, 9), randint(0, 9))
    # Make sure the key with that name has been created.
    key_fqn = self.kms_api.CreateCryptoKey(self.keyring_fqn, key_name)
    # They key may have already been created and used in a previous test
    # invocation; make sure it doesn't contain the IAM policy binding that
    # allows our project to encrypt/decrypt with it.
    key_policy = self.kms_api.GetKeyIamPolicy(key_fqn)
    while key_policy.bindings:
      key_policy.bindings.pop()
    self.kms_api.SetKeyIamPolicy(key_fqn, key_policy)
    # Set up the authorize command tokens.
    authorize_cmd = ['kms', 'authorize', '-k', key_fqn]
    if specified_project:
      authorize_cmd.extend(['-p', specified_project])

    stdout1 = self.RunGsUtil(authorize_cmd, return_stdout=True)
    stdout2 = self.RunGsUtil(authorize_cmd, return_stdout=True)

    self.assertIn(
        'Authorized project %s to encrypt and decrypt with key:\n%s' % (
            PopulateProjectId(None), key_fqn),
        stdout1)
    self.assertIn(
        ('Project %s was already authorized to encrypt and decrypt with '
         'key:\n%s.' % (PopulateProjectId(None), key_fqn)),
        stdout2)

  def DoTestServiceaccount(self, specified_project=None):
    serviceaccount_cmd = ['kms', 'serviceaccount']
    if specified_project:
      serviceaccount_cmd.extend(['-p', specified_project])

    stdout = self.RunGsUtil(serviceaccount_cmd, return_stdout=True)

    self.assertRegexpMatches(
        stdout,
        r'[^@]+@gs-project-accounts\.iam\.gserviceaccount\.com')

  def testKmsAuthorizeWithoutProjectOption(self):
    self.DoTestAuthorize()

  def testKmsAuthorizeWithProjectOption(self):
    self.DoTestAuthorize(specified_project=PopulateProjectId(None))

  def testKmsServiceaccountWithoutProjectOption(self):
    self.DoTestServiceaccount()

  def testKmsServiceaccountWithProjectOption(self):
    self.DoTestServiceaccount(specified_project=PopulateProjectId(None))

  def testKmsEncryptionFlow(self):
    # Since we have to create a bucket and set a default KMS key to test most
    # of these behaviors, we just test them all in one flow to reduce the number
    # of API calls.

    bucket_uri = self.CreateBucket()
    # Make sure our key exists.
    key_fqn = self.kms_api.CreateCryptoKey(
        self.keyring_fqn, testcase.KmsTestingResources.CONSTANT_KEY_NAME)
    encryption_get_cmd = ['kms', 'encryption', suri(bucket_uri)]

    # Test output for bucket with no default KMS key set.
    stdout = self.RunGsUtil(encryption_get_cmd, return_stdout=True)
    self.assertIn('Bucket %s has no default encryption key' % suri(bucket_uri),
                  stdout)

    # Test that setting a bucket's default KMS key works and shows up correctly
    # via a follow-up call to display it.
    stdout = self.RunGsUtil(
        ['kms', 'encryption', '-k', key_fqn, suri(bucket_uri)],
        return_stdout=True)
    self.assertIn('Setting default KMS key for bucket %s...' % suri(bucket_uri),
                  stdout)

    stdout = self.RunGsUtil(encryption_get_cmd, return_stdout=True)
    self.assertIn(
        'Default encryption key for %s:\n%s' % (suri(bucket_uri), key_fqn),
        stdout)

    # Finally, remove the bucket's default KMS key and make sure a follow-up
    # call to display it shows that no default key is set.
    stdout = self.RunGsUtil(['kms', 'encryption', '-d', suri(bucket_uri)],
                            return_stdout=True)
    self.assertIn(
        'Clearing default encryption key for %s...' % suri(bucket_uri),
        stdout)

    stdout = self.RunGsUtil(encryption_get_cmd, return_stdout=True)
    self.assertIn('Bucket %s has no default encryption key' % suri(bucket_uri),
                  stdout)


@SkipForS3('gsutil does not support KMS operations for S3 buckets.')
@SkipForJSON('These tests only check for failures when the XML API is forced.')
class TestKmsSubcommandsFailWhenXmlForced(testcase.GsUtilIntegrationTestCase):
  """Tests that kms subcommands fail early when forced to use the XML API."""

  boto_config_hmac_auth_only = [
      # Overwrite other credential types.
      ('Credentials', 'gs_oauth2_refresh_token', None),
      ('Credentials', 'gs_service_client_id', None),
      ('Credentials', 'gs_service_key_file', None),
      ('Credentials', 'gs_service_key_file_password', None),
      # Add hmac credentials.
      ('Credentials', 'gs_access_key_id', 'dummykey'),
      ('Credentials', 'gs_secret_access_key', 'dummysecret'),
  ]
  dummy_keyname = ('projects/my-project/locations/global/'
                   'keyRings/my-keyring/cryptoKeys/my-key')

  def DoTestSubcommandFailsWhenXmlForcedFromHmacInBotoConfig(self, subcommand):
    with SetBotoConfigForTest(self.boto_config_hmac_auth_only):
      stderr = self.RunGsUtil(subcommand, expected_status=1, return_stderr=True)
      self.assertIn('The "kms" command can only be used with', stderr)

  def testEncryptionFailsWhenXmlForcedFromHmacInBotoConfig(self):
    self.DoTestSubcommandFailsWhenXmlForcedFromHmacInBotoConfig(
        ['kms', 'encryption', 'gs://dummybucket'])

  def testEncryptionDashKFailsWhenXmlForcedFromHmacInBotoConfig(self):
    self.DoTestSubcommandFailsWhenXmlForcedFromHmacInBotoConfig(
        ['kms', 'encryption', '-k', self.dummy_keyname, 'gs://dummybucket'])

  def testEncryptionDashDFailsWhenXmlForcedFromHmacInBotoConfig(self):
    self.DoTestSubcommandFailsWhenXmlForcedFromHmacInBotoConfig(
        ['kms', 'encryption', '-d', 'gs://dummybucket'])

  def testServiceaccountFailsWhenXmlForcedFromHmacInBotoConfig(self):
    self.DoTestSubcommandFailsWhenXmlForcedFromHmacInBotoConfig(
        ['kms', 'serviceaccount', 'gs://dummybucket'])

  def testAuthorizeFailsWhenXmlForcedFromHmacInBotoConfig(self):
    self.DoTestSubcommandFailsWhenXmlForcedFromHmacInBotoConfig(
        ['kms', 'authorize', '-k', self.dummy_keyname, 'gs://dummybucket'])

