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
"""Contains gsutil base integration test case class."""

from __future__ import absolute_import

from contextlib import contextmanager
import cStringIO
import locale
import logging
import os
import subprocess
import sys
import tempfile

import boto
from boto import config
from boto.exception import StorageResponseError
from boto.s3.deletemarker import DeleteMarker
from boto.storage_uri import BucketStorageUri

import gslib
from gslib.boto_translation import BotoTranslation
from gslib.cloud_api import PreconditionException
from gslib.cloud_api import Preconditions
from gslib.encryption_helper import Base64Sha256FromBase64EncryptionKey
from gslib.encryption_helper import CryptoKeyWrapperFromKey
from gslib.gcs_json_api import GcsJsonApi
from gslib.hashing_helper import Base64ToHexHash
from gslib.kms_api import KmsApi
from gslib.posix_util import ATIME_ATTR
from gslib.posix_util import GID_ATTR
from gslib.posix_util import MODE_ATTR
from gslib.posix_util import MTIME_ATTR
from gslib.posix_util import UID_ATTR
from gslib.project_id import GOOG_PROJ_ID_HDR
from gslib.project_id import PopulateProjectId
from gslib.tests.testcase import base
import gslib.tests.util as util
from gslib.tests.util import ObjectToURI as suri
from gslib.tests.util import RUN_S3_TESTS
from gslib.tests.util import SetBotoConfigForTest
from gslib.tests.util import SetEnvironmentForTest
from gslib.tests.util import unittest
from gslib.tests.util import USING_JSON_API
import gslib.third_party.storage_apitools.storage_v1_messages as apitools_messages
from gslib.util import CreateCustomMetadata
from gslib.util import DiscardMessagesQueue
from gslib.util import GetValueFromObjectCustomMetadata
from gslib.util import IS_WINDOWS
from gslib.util import Retry
from gslib.util import UTF8


LOGGER = logging.getLogger('integration-test')


# TODO: Replace tests which looks for test_api == ApiSelector.(XML|JSON) with
# these decorators.
def SkipForXML(reason):
  if not USING_JSON_API:
    return unittest.skip(reason)
  else:
    return lambda func: func


def SkipForJSON(reason):
  if USING_JSON_API:
    return unittest.skip(reason)
  else:
    return lambda func: func


def SkipForGS(reason):
  if not RUN_S3_TESTS:
    return unittest.skip(reason)
  else:
    return lambda func: func


def SkipForS3(reason):
  if RUN_S3_TESTS:
    return unittest.skip(reason)
  else:
    return lambda func: func


# TODO: Right now, most tests use the XML API. Instead, they should respect
# prefer_api in the same way that commands do.
@unittest.skipUnless(util.RUN_INTEGRATION_TESTS,
                     'Not running integration tests.')
class GsUtilIntegrationTestCase(base.GsUtilTestCase):
  """Base class for gsutil integration tests."""
  GROUP_TEST_ADDRESS = 'gs-discussion@googlegroups.com'
  GROUP_TEST_ID = (
      '00b4903a97d097895ab58ef505d535916a712215b79c3e54932c2eb502ad97f5')
  USER_TEST_ADDRESS = 'gsutiltestuser@gmail.com'
  USER_TEST_ID = (
      '00b4903a97b201e40d2a5a3ddfe044bb1ab79c75b2e817cbe350297eccc81c84')
  DOMAIN_TEST = 'google.com'
  # No one can create this bucket without owning the gmail.com domain, and we
  # won't create this bucket, so it shouldn't exist.
  # It would be nice to use google.com here but JSON API disallows
  # 'google' in resource IDs.
  nonexistent_bucket_name = 'nonexistent-bucket-foobar.gmail.com'

  def setUp(self):
    """Creates base configuration for integration tests."""
    super(GsUtilIntegrationTestCase, self).setUp()
    self.bucket_uris = []

    # Set up API version and project ID handler.
    self.api_version = boto.config.get_value(
        'GSUtil', 'default_api_version', '1')

    # Instantiate a JSON API for use by the current integration test.
    self.json_api = GcsJsonApi(BucketStorageUri, logging.getLogger(),
                               DiscardMessagesQueue(), 'gs')
    self.xml_api = BotoTranslation(BucketStorageUri, logging.getLogger(),
                                   DiscardMessagesQueue, self.default_provider)
    self.kms_api = KmsApi()

    self.multiregional_buckets = util.USE_MULTIREGIONAL_BUCKETS

    if util.RUN_S3_TESTS:
      self.nonexistent_bucket_name = (
          'nonexistentbucket-asf801rj3r9as90mfnnkjxpo02')

  # Retry with an exponential backoff if a server error is received. This
  # ensures that we try *really* hard to clean up after ourselves.
  # TODO: As long as we're still using boto to do the teardown,
  # we decorate with boto exceptions.  Eventually this should be migrated
  # to CloudApi exceptions.
  @Retry(StorageResponseError, tries=7, timeout_secs=1)
  def tearDown(self):
    super(GsUtilIntegrationTestCase, self).tearDown()

    while self.bucket_uris:
      bucket_uri = self.bucket_uris[-1]
      try:
        bucket_list = self._ListBucket(bucket_uri)
      except StorageResponseError, e:
        # This can happen for tests of rm -r command, which for bucket-only
        # URIs delete the bucket at the end.
        if e.status == 404:
          self.bucket_uris.pop()
          continue
        else:
          raise
      while bucket_list:
        error = None
        for k in bucket_list:
          try:
            if isinstance(k, DeleteMarker):
              bucket_uri.get_bucket().delete_key(k.name,
                                                 version_id=k.version_id)
            else:
              k.delete()
          except StorageResponseError, e:
            # This could happen if objects that have already been deleted are
            # still showing up in the listing due to eventual consistency. In
            # that case, we continue on until we've tried to deleted every
            # object in the listing before raising the error on which to retry.
            if e.status == 404:
              error = e
            else:
              raise
        if error:
          raise error  # pylint: disable=raising-bad-type
        bucket_list = self._ListBucket(bucket_uri)
      bucket_uri.delete_bucket()
      self.bucket_uris.pop()

  def _SetObjectCustomMetadataAttribute(self, provider, bucket_name,
                                        object_name, attr_name, attr_value):
    """Sets a custom metadata attribute for an object.

    Args:
      provider: Provider string for the bucket, ex. 'gs' or 's3.
      bucket_name: The name of the bucket the object is in.
      object_name: The name of the object itself.
      attr_name: The name of the custom metadata attribute to set.
      attr_value: The value of the custom metadata attribute to set.

    Returns:
      None
    """
    obj_metadata = apitools_messages.Object()
    obj_metadata.metadata = CreateCustomMetadata({attr_name: attr_value})
    if provider == 'gs':
      self.json_api.PatchObjectMetadata(bucket_name, object_name, obj_metadata,
                                        provider=provider)
    else:
      self.xml_api.PatchObjectMetadata(bucket_name, object_name, obj_metadata,
                                       provider=provider)

  def SetPOSIXMetadata(self, provider, bucket_name, object_name, atime=None,
                       mtime=None, uid=None, gid=None, mode=None):
    """Sets POSIX metadata for the object."""
    obj_metadata = apitools_messages.Object()
    obj_metadata.metadata = apitools_messages.Object.MetadataValue(
        additionalProperties=[])
    if atime is not None:
      CreateCustomMetadata(entries={ATIME_ATTR: atime},
                           custom_metadata=obj_metadata.metadata)
    if mode is not None:
      CreateCustomMetadata(entries={MODE_ATTR: mode},
                           custom_metadata=obj_metadata.metadata)
    if mtime is not None:
      CreateCustomMetadata(entries={MTIME_ATTR: mtime},
                           custom_metadata=obj_metadata.metadata)
    if uid is not None:
      CreateCustomMetadata(entries={UID_ATTR: uid},
                           custom_metadata=obj_metadata.metadata)
    if gid is not None:
      CreateCustomMetadata(entries={GID_ATTR: gid},
                           custom_metadata=obj_metadata.metadata)
    if provider == 'gs':
      self.json_api.PatchObjectMetadata(bucket_name, object_name, obj_metadata,
                                        provider=provider)
    else:
      self.xml_api.PatchObjectMetadata(bucket_name, object_name, obj_metadata,
                                       provider=provider)

  def ClearPOSIXMetadata(self, obj):
    """Uses the setmeta command to clear POSIX attributes from user metadata.

    Args:
      obj: The object to clear POSIX metadata for.
    """
    provider_meta_string = 'goog' if obj.scheme == 'gs' else 'amz'
    self.RunGsUtil(['setmeta',
                    '-h', 'x-%s-meta-%s' % (provider_meta_string, ATIME_ATTR),
                    '-h', 'x-%s-meta-%s' % (provider_meta_string, MTIME_ATTR),
                    '-h', 'x-%s-meta-%s' % (provider_meta_string, UID_ATTR),
                    '-h', 'x-%s-meta-%s' % (provider_meta_string, GID_ATTR),
                    '-h', 'x-%s-meta-%s' % (provider_meta_string, MODE_ATTR),
                    suri(obj)])

  def _ServiceAccountCredentialsPresent(self):
    # TODO: Currently, service accounts cannot be project owners (unless
    # they are grandfathered). Unfortunately, setting a canned ACL other
    # than project-private, the ACL that buckets get by default, removes
    # project-editors access from the bucket ACL. So any canned ACL that would
    # actually represent a change the bucket would also orphan the service
    # account's access to the bucket. If service accounts can be owners
    # in the future, remove this function and update all callers.
    return (config.has_option('Credentials', 'gs_service_key_file') or
            config.has_option('GoogleCompute', 'service_account'))

  def _ListBucket(self, bucket_uri):
    if bucket_uri.scheme == 's3':
      # storage_uri will omit delete markers from bucket listings, but
      # these must be deleted before we can remove an S3 bucket.
      return list(v for v in bucket_uri.get_bucket().list_versions())
    return list(bucket_uri.list_bucket(all_versions=True))

  def AssertNObjectsInBucket(self, bucket_uri, num_objects, versioned=False):
    """Checks (with retries) that 'ls bucket_uri/**' returns num_objects.

    This is a common test pattern to deal with eventual listing consistency for
    tests that rely on a set of objects to be listed.

    Args:
      bucket_uri: storage_uri for the bucket.
      num_objects: number of objects expected in the bucket.
      versioned: If True, perform a versioned listing.

    Raises:
      AssertionError if number of objects does not match expected value.

    Returns:
      Listing split across lines.
    """
    def _CheckBucket():
      command = ['ls', '-a'] if versioned else ['ls']
      b_uri = [suri(bucket_uri) + '/**'] if num_objects else [suri(bucket_uri)]
      listing = self.RunGsUtil(command + b_uri, return_stdout=True).split('\n')
      # num_objects + one trailing newline.
      self.assertEquals(len(listing), num_objects + 1)
      return listing

    if self.multiregional_buckets:
      # Use @Retry as hedge against bucket listing eventual consistency.
      @Retry(AssertionError, tries=5, timeout_secs=1)
      def _Check1():
        return _CheckBucket()

      return _Check1()
    else:
      return _CheckBucket()

  def AssertObjectUsesCSEK(self, object_uri_str, encryption_key):
    """Strongly consistent check that the correct CSEK encryption key is used.

    This check forces use of the JSON API, as encryption information is not
    returned in object metadata via the XML API.
    """
    with SetBotoConfigForTest([('GSUtil', 'prefer_api', 'json')]):
      stdout = self.RunGsUtil(['stat', object_uri_str], return_stdout=True)
    self.assertIn(
        Base64Sha256FromBase64EncryptionKey(encryption_key), stdout,
        'Object %s did not use expected encryption key with hash %s. '
        'Actual object: %s'%
        (object_uri_str, Base64Sha256FromBase64EncryptionKey(encryption_key),
         stdout))

  def AssertObjectUsesCMEK(self, object_uri_str, encryption_key):
    """Strongly consistent check that the correct KMS encryption key is used.

    This check forces use of the JSON API, as encryption information is not
    returned in object metadata via the XML API.
    """
    with SetBotoConfigForTest([('GSUtil', 'prefer_api', 'json')]):
      stdout = self.RunGsUtil(['stat', object_uri_str], return_stdout=True)
    self.assertRegexpMatches(stdout, r'KMS key:\s+%s' % encryption_key)

  def AssertObjectUnencrypted(self, object_uri_str):
    """Checks that no CSEK or CMEK attributes appear in `stat` output.

    This check forces use of the JSON API, as encryption information is not
    returned in object metadata via the XML API.
    """
    with SetBotoConfigForTest([('GSUtil', 'prefer_api', 'json')]):
      stdout = self.RunGsUtil(['stat', object_uri_str], return_stdout=True)
    self.assertNotIn('Encryption key SHA256', stdout)
    self.assertNotIn('KMS key', stdout)

  def CreateBucket(self, bucket_name=None, test_objects=0, storage_class=None,
                   provider=None, prefer_json_api=False,
                   versioning_enabled=False):
    """Creates a test bucket.

    The bucket and all of its contents will be deleted after the test.

    Args:
      bucket_name: Create the bucket with this name. If not provided, a
                   temporary test bucket name is constructed.
      test_objects: The number of objects that should be placed in the bucket.
                    Defaults to 0.
      storage_class: Storage class to use. If not provided we us standard.
      provider: Provider to use - either "gs" (the default) or "s3".
      prefer_json_api: If True, use the JSON creation functions where possible.
      versioning_enabled: If True, set the bucket's versioning attribute to
          True.

    Returns:
      StorageUri for the created bucket.
    """
    if not provider:
      provider = self.default_provider

    # Location is controlled by the -b test flag.
    if self.multiregional_buckets or provider == 's3':
      location = None
    else:
      location = 'us-central1'

    if prefer_json_api and provider == 'gs':
      json_bucket = self.CreateBucketJson(bucket_name=bucket_name,
                                          test_objects=test_objects,
                                          storage_class=storage_class,
                                          location=location,
                                          versioning_enabled=versioning_enabled)
      bucket_uri = boto.storage_uri(
          'gs://%s' % json_bucket.name.encode(UTF8).lower(),
          suppress_consec_slashes=False)
      self.bucket_uris.append(bucket_uri)
      return bucket_uri

    bucket_name = bucket_name or self.MakeTempName('bucket')

    bucket_uri = boto.storage_uri('%s://%s' % (provider, bucket_name.lower()),
                                  suppress_consec_slashes=False)

    if provider == 'gs':
      # Apply API version and project ID headers if necessary.
      headers = {'x-goog-api-version': self.api_version}
      headers[GOOG_PROJ_ID_HDR] = PopulateProjectId()
    else:
      headers = {}

    # Parallel tests can easily run into bucket creation quotas.
    # Retry with exponential backoff so that we create them as fast as we
    # reasonably can.
    @Retry(StorageResponseError, tries=7, timeout_secs=1)
    def _CreateBucketWithExponentialBackoff():
      try:
        bucket_uri.create_bucket(storage_class=storage_class,
                                 location=location or '',
                                 headers=headers)
      except StorageResponseError, e:
        # If the service returns a transient error or a connection breaks,
        # it's possible the request succeeded. If that happens, the service
        # will return 409s for all future calls even though our intent
        # succeeded. If the error message says we already own the bucket,
        # assume success to reduce test flakiness. This depends on
        # randomness of test naming buckets to prevent name collisions for
        # test buckets created concurrently in the same project, which is
        # acceptable because this is far less likely than service errors.
        if e.status == 409 and e.body and 'already own' in e.body:
          pass
        else:
          raise

    _CreateBucketWithExponentialBackoff()
    self.bucket_uris.append(bucket_uri)

    if versioning_enabled:
      bucket_uri.configure_versioning(True)

    for i in range(test_objects):
      self.CreateObject(bucket_uri=bucket_uri,
                        object_name=self.MakeTempName('obj'),
                        contents='test %d' % i)
    return bucket_uri

  def CreateVersionedBucket(self, bucket_name=None, test_objects=0):
    """Creates a versioned test bucket.

    The bucket and all of its contents will be deleted after the test.

    Args:
      bucket_name: Create the bucket with this name. If not provided, a
                   temporary test bucket name is constructed.
      test_objects: The number of objects that should be placed in the bucket.
                    Defaults to 0.

    Returns:
      StorageUri for the created bucket with versioning enabled.
    """
    # Note that we prefer the JSON API so that we don't require two separate
    # steps to create and then set versioning on the bucket (as versioning
    # propagation on an existing bucket is subject to eventual consistency).
    bucket_uri = self.CreateBucket(
        bucket_name=bucket_name,
        test_objects=test_objects,
        prefer_json_api=True,
        versioning_enabled=True)
    return bucket_uri

  def CreateObject(self, bucket_uri=None, object_name=None, contents=None,
                   prefer_json_api=False, encryption_key=None, mode=None,
                   mtime=None, uid=None, gid=None, storage_class=None,
                   gs_idempotent_generation=0, kms_key_name=None):
    """Creates a test object.

    Args:
      bucket_uri: The URI of the bucket to place the object in. If not
          specified, a new temporary bucket is created.
      object_name: The name to use for the object. If not specified, a temporary
          test object name is constructed.
      contents: The contents to write to the object. If not specified, the key
          is not written to, which means that it isn't actually created
          yet on the server.
      prefer_json_api: If true, use the JSON creation functions where possible.
      encryption_key: AES256 encryption key to use when creating the object,
          if any.
      mode: The POSIX mode for the object. Must be a base-8 3-digit integer
          represented as a string.
      mtime: The modification time of the file in POSIX time (seconds since
          UTC 1970-01-01). If not specified, this defaults to the current
          system time.
      uid: A POSIX user ID.
      gid: A POSIX group ID.
      storage_class: String representing the storage class to use for the
          object.
      gs_idempotent_generation: For use when overwriting an object for which
          you know the previously uploaded generation. Create GCS object
          idempotently by supplying this generation number as a precondition
          and assuming the current object is correct on precondition failure.
          Defaults to 0 (new object); to disable, set to None.
      kms_key_name: Fully-qualified name of the KMS key that should be used to
          encrypt the object. Note that this is currently only valid for 'gs'
          objects.

    Returns:
      A StorageUri for the created object.
    """
    bucket_uri = bucket_uri or self.CreateBucket()

    if (contents and
        bucket_uri.scheme == 'gs' and
        (prefer_json_api or encryption_key or kms_key_name)):

      object_name = object_name or self.MakeTempName('obj')
      json_object = self.CreateObjectJson(
          contents=contents, bucket_name=bucket_uri.bucket_name,
          object_name=object_name, encryption_key=encryption_key,
          mtime=mtime, storage_class=storage_class,
          gs_idempotent_generation=gs_idempotent_generation,
          kms_key_name=kms_key_name)
      object_uri = bucket_uri.clone_replace_name(object_name)
      # pylint: disable=protected-access
      # Need to update the StorageUri with the correct values while
      # avoiding creating a versioned string.

      md5 = (Base64ToHexHash(json_object.md5Hash),
             json_object.md5Hash.strip('\n"\''))
      object_uri._update_from_values(None,
                                     json_object.generation,
                                     True,
                                     md5=md5)
      # pylint: enable=protected-access
      return object_uri

    bucket_uri = bucket_uri or self.CreateBucket()
    object_name = object_name or self.MakeTempName('obj')
    key_uri = bucket_uri.clone_replace_name(object_name)
    if contents is not None:
      if bucket_uri.scheme == 'gs' and gs_idempotent_generation is not None:
        try:
          key_uri.set_contents_from_string(
              contents, headers={
                  'x-goog-if-generation-match': str(gs_idempotent_generation)})
        except StorageResponseError, e:
          if e.status == 412:
            pass
          else:
            raise
      else:
        key_uri.set_contents_from_string(contents)
    custom_metadata_present = (mode is not None or mtime is not None
                               or uid is not None or gid is not None)
    if custom_metadata_present:
      self.SetPOSIXMetadata(bucket_uri.scheme, bucket_uri.bucket_name,
                            object_name, atime=None, mtime=mtime,
                            uid=uid, gid=gid, mode=mode)
    return key_uri

  def CreateBucketJson(self, bucket_name=None, test_objects=0,
                       storage_class=None, location=None,
                       versioning_enabled=False):
    """Creates a test bucket using the JSON API.

    The bucket and all of its contents will be deleted after the test.

    Args:
      bucket_name: Create the bucket with this name. If not provided, a
                   temporary test bucket name is constructed.
      test_objects: The number of objects that should be placed in the bucket.
                    Defaults to 0.
      storage_class: Storage class to use. If not provided we use standard.
      location: Location to use.
      versioning_enabled: If True, set the bucket's versioning attribute to
          True.

    Returns:
      Apitools Bucket for the created bucket.
    """
    bucket_name = bucket_name or self.MakeTempName('bucket')
    bucket_metadata = apitools_messages.Bucket(name=bucket_name.lower())
    if storage_class:
      bucket_metadata.storageClass = storage_class
    if location:
      bucket_metadata.location = location
    if versioning_enabled:
      bucket_metadata.versioning = (
          apitools_messages.Bucket.VersioningValue(enabled=True))

    # TODO: Add retry and exponential backoff.
    bucket = self.json_api.CreateBucket(bucket_name.lower(),
                                        metadata=bucket_metadata)
    # Add bucket to list of buckets to be cleaned up.
    # TODO: Clean up JSON buckets using JSON API.
    self.bucket_uris.append(
        boto.storage_uri('gs://%s' % (bucket_name.lower()),
                         suppress_consec_slashes=False))
    for i in range(test_objects):
      self.CreateObjectJson(bucket_name=bucket_name,
                            object_name=self.MakeTempName('obj'),
                            contents='test %d' % i)
    return bucket

  def CreateObjectJson(self, contents, bucket_name=None, object_name=None,
                       encryption_key=None, mtime=None, storage_class=None,
                       gs_idempotent_generation=None, kms_key_name=None):
    """Creates a test object (GCS provider only) using the JSON API.

    Args:
      contents: The contents to write to the object.
      bucket_name: Name of bucket to place the object in. If not specified,
          a new temporary bucket is created.
      object_name: The name to use for the object. If not specified, a temporary
          test object name is constructed.
      encryption_key: AES256 encryption key to use when creating the object,
          if any.
      mtime: The modification time of the file in POSIX time (seconds since
          UTC 1970-01-01). If not specified, this defaults to the current
          system time.
      storage_class: String representing the storage class to use for the
          object.
      gs_idempotent_generation: For use when overwriting an object for which
          you know the previously uploaded generation. Create GCS object
          idempotently by supplying this generation number as a precondition
          and assuming the current object is correct on precondition failure.
          Defaults to 0 (new object); to disable, set to None.
      kms_key_name: Fully-qualified name of the KMS key that should be used to
          encrypt the object. Note that this is currently only valid for 'gs'
          objects.

    Returns:
      An apitools Object for the created object.
    """
    bucket_name = bucket_name or self.CreateBucketJson().name
    object_name = object_name or self.MakeTempName('obj')
    preconditions = Preconditions(gen_match=gs_idempotent_generation)
    custom_metadata = apitools_messages.Object.MetadataValue(
        additionalProperties=[])
    if mtime is not None:
      CreateCustomMetadata({MTIME_ATTR: mtime}, custom_metadata)
    object_metadata = apitools_messages.Object(
        name=object_name,
        metadata=custom_metadata,
        bucket=bucket_name,
        contentType='application/octet-stream',
        storageClass=storage_class,
        kmsKeyName=kms_key_name)
    encryption_keywrapper = CryptoKeyWrapperFromKey(encryption_key)
    try:
      return self.json_api.UploadObject(
          cStringIO.StringIO(contents),
          object_metadata, provider='gs',
          encryption_tuple=encryption_keywrapper,
          preconditions=preconditions)
    except PreconditionException:
      if gs_idempotent_generation is None:
        raise
      with SetBotoConfigForTest([('GSUtil', 'decryption_key1',
                                  encryption_key)]):
        return self.json_api.GetObjectMetadata(bucket_name, object_name)

  def VerifyObjectCustomAttribute(self, bucket_name, object_name, attr_name,
                                  expected_value, expected_present=True):
    """Retrieves and verifies an object's custom metadata attribute.

    Args:
      bucket_name: The name of the bucket the object is in.
      object_name: The name of the object itself.
      attr_name: The name of the custom metadata attribute.
      expected_value: The expected retrieved value for the attribute.
      expected_present: True if the attribute must be present in the
          object metadata, False if it must not be present.

    Returns:
      None
    """
    gsutil_api = (self.json_api if self.default_provider == 'gs'
                  else self.xml_api)
    metadata = gsutil_api.GetObjectMetadata(bucket_name, object_name,
                                            provider=self.default_provider,
                                            fields=['metadata/%s' % attr_name])
    attr_present, value = GetValueFromObjectCustomMetadata(
        metadata, attr_name, default_value=expected_value)
    self.assertEqual(expected_present, attr_present)
    self.assertEqual(expected_value, value)

  def RunGsUtil(self, cmd, return_status=False,
                return_stdout=False, return_stderr=False,
                expected_status=0, stdin=None, env_vars=None):
    """Runs the gsutil command.

    Args:
      cmd: The command to run, as a list, e.g. ['cp', 'foo', 'bar']
      return_status: If True, the exit status code is returned.
      return_stdout: If True, the standard output of the command is returned.
      return_stderr: If True, the standard error of the command is returned.
      expected_status: The expected return code. If not specified, defaults to
                       0. If the return code is a different value, an exception
                       is raised.
      stdin: A string of data to pipe to the process as standard input.
      env_vars: A dictionary of variables to extend the subprocess's os.environ
                with.

    Returns:
      If multiple return_* values were specified, this method returns a tuple
      containing the desired return values specified by the return_* arguments
      (in the order those parameters are specified in the method definition).
      If only one return_* value was specified, that value is returned directly
      rather than being returned within a 1-tuple.
    """
    cmd = ([gslib.GSUTIL_PATH] + ['--testexceptiontraces'] +
           ['-o', 'GSUtil:default_project_id=' + PopulateProjectId()] +
           cmd)
    if IS_WINDOWS:
      cmd = [sys.executable] + cmd
    env = os.environ.copy()
    if env_vars:
      env.update(env_vars)
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                         stdin=subprocess.PIPE, env=env)
    (stdout, stderr) = p.communicate(stdin)
    status = p.returncode

    if expected_status is not None:
      self.assertEqual(
          status, expected_status,
          msg='Expected status %d, got %d.\nCommand:\n%s\n\nstderr:\n%s' % (
              expected_status, status, ' '.join(cmd), stderr))

    toreturn = []
    if return_status:
      toreturn.append(status)
    if return_stdout:
      if IS_WINDOWS:
        stdout = stdout.replace('\r\n', '\n')
      toreturn.append(stdout)
    if return_stderr:
      if IS_WINDOWS:
        stderr = stderr.replace('\r\n', '\n')
      toreturn.append(stderr)

    if len(toreturn) == 1:
      return toreturn[0]
    elif toreturn:
      return tuple(toreturn)

  def RunGsUtilTabCompletion(self, cmd, expected_results=None):
    """Runs the gsutil command in tab completion mode.

    Args:
      cmd: The command to run, as a list, e.g. ['cp', 'foo', 'bar']
      expected_results: The expected tab completion results for the given input.
    """
    cmd = [gslib.GSUTIL_PATH] + ['--testexceptiontraces'] + cmd
    cmd_str = ' '.join(cmd)

    @Retry(AssertionError, tries=5, timeout_secs=1)
    def _RunTabCompletion():
      """Runs the tab completion operation with retries."""
      results_string = None
      with tempfile.NamedTemporaryFile(
          delete=False) as tab_complete_result_file:
        # argcomplete returns results via the '8' file descriptor so we
        # redirect to a file so we can capture them.
        cmd_str_with_result_redirect = '%s 8>%s' % (
            cmd_str, tab_complete_result_file.name)
        env = os.environ.copy()
        env['_ARGCOMPLETE'] = '1'
        # Use a sane default for COMP_WORDBREAKS.
        env['_ARGCOMPLETE_COMP_WORDBREAKS'] = '''"'@><=;|&(:'''
        if 'COMP_WORDBREAKS' in env:
          env['_ARGCOMPLETE_COMP_WORDBREAKS'] = env['COMP_WORDBREAKS']
        env['COMP_LINE'] = cmd_str
        env['COMP_POINT'] = str(len(cmd_str))
        subprocess.call(cmd_str_with_result_redirect, env=env, shell=True)
        results_string = tab_complete_result_file.read().decode(
            locale.getpreferredencoding())
      if results_string:
        results = results_string.split('\013')
      else:
        results = []
      self.assertEqual(results, expected_results)

    # When tests are run in parallel, tab completion could take a long time,
    # so choose a long timeout value.
    with SetBotoConfigForTest([('GSUtil', 'tab_completion_timeout', '120')]):
      _RunTabCompletion()

  @contextmanager
  def SetAnonymousBotoCreds(self):
    # Tell gsutil not to override the real error message with a warning about
    # anonymous access if no credentials are provided in the config file.
    boto_config_for_test = [
        ('Tests', 'bypass_anonymous_access_warning', 'True')]

    # Also, maintain any custom host/port/API configuration, since we'll need
    # to contact the same host when operating in a development environment.
    for creds_config_key in (
        'gs_host', 'gs_json_host', 'gs_post', 'gs_json_port'):
      boto_config_for_test.append(
          ('Credentials', creds_config_key,
           boto.config.get('Credentials', creds_config_key, None)))
    boto_config_for_test.append(
        ('Boto', 'https_validate_certificates',
         boto.config.get('Boto', 'https_validate_certificates', None)))
    for api_config_key in ('json_api_version', 'prefer_api'):
      boto_config_for_test.append(
          ('GSUtil', api_config_key,
           boto.config.get('GSUtil', api_config_key, None)))

    with SetBotoConfigForTest(boto_config_for_test, use_existing_config=False):
      # Make sure to reset Developer Shell credential port so that the child
      # gsutil process is really anonymous.
      with SetEnvironmentForTest({'DEVSHELL_CLIENT_PORT': None}):
        yield

  def _VerifyLocalMode(self, path, expected_mode):
    """Verifies the mode of the file specified at path.

    Args:
      path: The path of the file on the local file system.
      expected_mode: The expected mode as a 3-digit base-8 number.

    Returns:
      None
    """
    self.assertEqual(expected_mode, int(oct(os.stat(path).st_mode)[-3:], 8))

  def _VerifyLocalUid(self, path, expected_uid):
    """Verifies the uid of the file specified at path.

    Args:
      path: The path of the file on the local file system.
      expected_uid: The expected uid of the file.

    Returns:
      None
    """
    self.assertEqual(expected_uid, os.stat(path).st_uid)

  def _VerifyLocalGid(self, path, expected_gid):
    """Verifies the gid of the file specified at path.

    Args:
      path: The path of the file on the local file system.
      expected_gid: The expected gid of the file.

    Returns:
      None
    """
    self.assertEqual(expected_gid, os.stat(path).st_gid)

  def VerifyLocalPOSIXPermissions(self, path, gid=None, uid=None, mode=None):
    """Verifies the uid, gid, and mode of the file specified at path.

    Will only check the attribute if the corresponding method parameter is not
    None.

    Args:
      path: The path of the file on the local file system.
      gid: The expected gid of the file.
      uid: The expected uid of the file.
      mode: The expected mode of the file.

    Returns:
      None
    """
    if gid is not None:
      self._VerifyLocalGid(path, gid)
    if uid is not None:
      self._VerifyLocalUid(path, uid)
    if mode is not None:
      self._VerifyLocalMode(path, mode)

  def FlatListDir(self, directory):
    """Perform a flat listing over directory.

    Args:
      directory: The directory to list

    Returns:
      Listings with path separators canonicalized to '/', to make assertions
      easier for Linux vs Windows.
    """
    result = []
    for dirpath, _, filenames in os.walk(directory):
      for f in filenames:
        result.append(os.path.join(dirpath, f))
    return '\n'.join(result).replace('\\', '/')

  def FlatListBucket(self, bucket_url_string):
    """Perform a flat listing over bucket_url_string."""
    return self.RunGsUtil(['ls', suri(bucket_url_string, '**')],
                          return_stdout=True)


class KmsTestingResources(object):
  """Constants for KMS resource names to be used in integration testing."""
  KEYRING_LOCATION = 'global'
  # Since KeyRings and their child resources cannot be deleted, we minimize the
  # number of resources created by using a hard-coded keyRing name.
  KEYRING_NAME = 'keyring-for-gsutil-integration-tests'

  # Used by tests where we don't need to alter the state of a cryptoKey and/or
  # its IAM policy bindings once it's initialized the first time.
  CONSTANT_KEY_NAME = 'key-for-gsutil-integration-tests'
  CONSTANT_KEY_NAME2 = 'key-for-gsutil-integration-tests2'
  # Pattern used for keys that should only be operated on by one tester at a
  # time. Because multiple integration test invocations can run at the same
  # time, we want to minimize the risk of them operating on each other's key,
  # while also not creating too many one-time-use keys (as they cannot be
  # deleted). Tests should fill in the %d entries with a digit between 0 and 9.
  MUTABLE_KEY_NAME_TEMPLATE = 'cryptokey-for-gsutil-integration-tests-%d%d%d'
