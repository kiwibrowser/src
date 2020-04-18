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
"""Integration tests for rsync command."""

import os

import crcmod
from gslib.hashing_helper import SLOW_CRCMOD_RSYNC_WARNING
from gslib.posix_util import ConvertDatetimeToPOSIX
from gslib.posix_util import GID_ATTR
from gslib.posix_util import MODE_ATTR
from gslib.posix_util import MTIME_ATTR
from gslib.posix_util import NA_TIME
from gslib.posix_util import UID_ATTR
from gslib.project_id import PopulateProjectId
import gslib.tests.testcase as testcase
from gslib.tests.testcase.integration_testcase import SkipForGS
from gslib.tests.testcase.integration_testcase import SkipForS3
from gslib.tests.testcase.integration_testcase import SkipForXML
from gslib.tests.util import BuildErrorRegex
from gslib.tests.util import ObjectToURI as suri
from gslib.tests.util import ORPHANED_FILE
from gslib.tests.util import POSIX_GID_ERROR
from gslib.tests.util import POSIX_INSUFFICIENT_ACCESS_ERROR
from gslib.tests.util import POSIX_MODE_ERROR
from gslib.tests.util import POSIX_UID_ERROR
from gslib.tests.util import SequentialAndParallelTransfer
from gslib.tests.util import SetBotoConfigForTest
from gslib.tests.util import TailSet
from gslib.tests.util import unittest
from gslib.util import IS_OSX
from gslib.util import IS_WINDOWS
from gslib.util import Retry
from gslib.util import UsingCrcmodExtension

# These POSIX-specific variables aren't defined for Windows.
# pylint: disable=g-import-not-at-top
if not IS_WINDOWS:
  from gslib.tests.util import DEFAULT_MODE
  from gslib.tests.util import INVALID_GID
  from gslib.tests.util import INVALID_UID
  from gslib.tests.util import NON_PRIMARY_GID
  from gslib.tests.util import PRIMARY_GID
  from gslib.tests.util import USER_ID
# pylint: enable=g-import-not-at-top

NO_CHANGES = 'Building synchronization state...\nStarting synchronization...\n'
if not UsingCrcmodExtension(crcmod):
  NO_CHANGES = SLOW_CRCMOD_RSYNC_WARNING + '\n' + NO_CHANGES


# TODO: Add inspection to the retry wrappers in this test suite where the state
# at the end of a retry block is depended upon by subsequent tests (since
# listing content can vary depending on which backend server is reached until
# eventual consistency is reached).
# TODO: Remove retry wrappers and AssertNObjectsInBucket calls if GCS ever
# supports strong listing consistency.
class TestRsync(testcase.GsUtilIntegrationTestCase):
  """Integration tests for rsync command."""

  def _GetMetadataAttribute(self, bucket_name, object_name, attr_name):
    """Retrieves and returns an attribute from an objects metadata.

    Args:
      bucket_name: The name of the bucket the object is in.
      object_name: The name of the object itself.
      attr_name: The name of the custom metadata attribute.

    Returns:
      The value at the specified attribute name in the metadata. If not present,
      returns None.
    """
    gsutil_api = (self.json_api if self.default_provider == 'gs'
                  else self.xml_api)
    metadata = gsutil_api.GetObjectMetadata(bucket_name, object_name,
                                            provider=self.default_provider,
                                            fields=[attr_name])
    return getattr(metadata, attr_name, None)

  def _VerifyObjectMtime(self, bucket_name, object_name, expected_mtime,
                         expected_present=True):
    """Retrieves the object's mtime.

    Args:
      bucket_name: The name of the bucket the object is in.
      object_name: The name of the object itself.
      expected_mtime: The expected retrieved mtime.
      expected_present: True if the mtime must be present in the
          object metadata, False if it must not be present.


    Returns:
      None
    """
    self.VerifyObjectCustomAttribute(
        bucket_name, object_name, MTIME_ATTR, expected_mtime,
        expected_present=expected_present)

  def test_invalid_args(self):
    """Tests various invalid argument cases."""
    bucket_uri = self.CreateBucket()
    obj1 = self.CreateObject(bucket_uri=bucket_uri, object_name='obj1',
                             contents='obj1')
    tmpdir = self.CreateTempDir()
    # rsync object to bucket.
    self.RunGsUtil(['rsync', suri(obj1), suri(bucket_uri)], expected_status=1)
    # rsync bucket to object.
    self.RunGsUtil(['rsync', suri(bucket_uri), suri(obj1)], expected_status=1)
    # rsync bucket to non-existent bucket.
    self.RunGsUtil(['rsync', suri(bucket_uri), self.nonexistent_bucket_name],
                   expected_status=1)
    # rsync object to dir.
    self.RunGsUtil(['rsync', suri(obj1), tmpdir], expected_status=1)
    # rsync dir to object.
    self.RunGsUtil(['rsync', tmpdir, suri(obj1)], expected_status=1)
    # rsync dir to non-existent bucket.
    self.RunGsUtil(['rsync', tmpdir, suri(obj1), self.nonexistent_bucket_name],
                   expected_status=1)

  # Note: The tests below exercise the cases
  # {src_dir, src_bucket} X {dst_dir, dst_bucket}. We use gsutil rsync -d for
  # all the cases but then have just one test without -d (test_bucket_to_bucket)
  # as representative of handling without the -d option. This provides
  # reasonable test coverage because the -d handling it src/dest URI-type
  # independent, and keeps the test case combinations more manageable.

  def test_invalid_src_mtime(self):
    """Tests that an exception is thrown if mtime cannot be cast as a long."""
    # Create 1 bucket with 1 file present with mtime set as a string of
    # non-numeric characters, and as a number.
    bucket1_uri = self.CreateBucket()
    bucket2_uri = self.CreateBucket()
    self.CreateObject(bucket_uri=bucket1_uri, object_name='obj1',
                      contents='obj1', mtime='xyz')
    self.CreateObject(bucket_uri=bucket1_uri, object_name='obj2',
                      contents='obj2', mtime=123)
    # This creates an object that has an mtime sometime on 41091-11-25 UTC. It
    # is used to verify that a warning is thrown for objects set at least a day
    # in the future. If this test is not updated before that date, this test
    # will fail because of the hardcoded timestamp.
    self.CreateObject(bucket_uri=bucket1_uri, object_name='obj3',
                      contents='obj3', mtime=1234567891011L)
    # Create objects with a negative mtime.
    self.CreateObject(bucket_uri=bucket1_uri, object_name='obj4',
                      contents='obj4', mtime=-100)
    self.CreateObject(bucket_uri=bucket1_uri, object_name='obj5',
                      contents='obj5', mtime=-1)

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check1():
      stderr = self.RunGsUtil(['rsync', suri(bucket1_uri),
                               suri(bucket2_uri)], return_stderr=True)
      self.assertIn('obj1 has an invalid mtime in its metadata', stderr)
      self.assertNotIn('obj2 has an invalid mtime in its metadata', stderr)
      self.assertIn('obj3 has an mtime more than 1 day from current system '
                    'time', stderr)
      self.assertIn('obj4 has a negative mtime in its metadata', stderr)
      self.assertIn('obj5 has a negative mtime in its metadata', stderr)
    _Check1()

  @unittest.skipIf(IS_WINDOWS, 'POSIX attributes not available on Windows.')
  @unittest.skipUnless(UsingCrcmodExtension(crcmod),
                       'Test requires fast crcmod.')
  def test_bucket_to_bucket_preserve_posix(self):
    """Tests that rsync -P works with bucket to bucket."""
    # Note that unlike bucket to dir tests POSIX attributes cannot be verified
    # beyond basic validations because the cloud buckets have no notion of UID
    # or GID especially considering that these values can come from systems
    # with vastly different mappings. In addition, the mode will not be
    # verified.
    src_bucket = self.CreateBucket()
    dst_bucket = self.CreateBucket()
    # Create source objects.
    self.CreateObject(bucket_uri=src_bucket, object_name='obj1',
                      contents='obj1', mode='444')
    self.CreateObject(bucket_uri=src_bucket, object_name='obj2',
                      contents='obj2', gid=PRIMARY_GID)
    self.CreateObject(bucket_uri=src_bucket, object_name='obj3',
                      contents='obj3', gid=NON_PRIMARY_GID())
    self.CreateObject(bucket_uri=src_bucket, object_name='obj4',
                      contents='obj3', uid=INVALID_UID(), gid=INVALID_GID(),
                      mode='222')
    self.CreateObject(bucket_uri=src_bucket, object_name='obj5',
                      contents='obj5', uid=USER_ID, gid=PRIMARY_GID,
                      mode=str(DEFAULT_MODE))
    # Create destination objects.
    # obj5 at the source and destination have the same content so we will only
    # patch the destination metadata instead of copying the entire object.
    self.CreateObject(bucket_uri=dst_bucket, object_name='obj5',
                      contents='obj5')

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check1():
      """Test bucket to bucket rsync with -P flag and verify attributes."""
      stderr = self.RunGsUtil(['rsync', '-P', suri(src_bucket),
                               suri(dst_bucket)], return_stderr=True)
      listing1 = TailSet(suri(src_bucket), self.FlatListBucket(src_bucket))
      listing2 = TailSet(suri(dst_bucket), self.FlatListBucket(dst_bucket))
      # First bucket should have un-altered content.
      self.assertEquals(listing1, set(['/obj1', '/obj2', '/obj3', '/obj4',
                                       '/obj5']))
      # dst_bucket should have new content from src_bucket.
      self.assertEquals(listing2, set(['/obj1', '/obj2', '/obj3', '/obj4',
                                       '/obj5']))
      self.assertIn('Copying POSIX attributes from src to dst for', stderr)
    _Check1()

    self.VerifyObjectCustomAttribute(dst_bucket.bucket_name, 'obj1',
                                     MODE_ATTR, '444')
    self.VerifyObjectCustomAttribute(dst_bucket.bucket_name, 'obj2',
                                     GID_ATTR, str(PRIMARY_GID))
    self.VerifyObjectCustomAttribute(dst_bucket.bucket_name, 'obj3',
                                     GID_ATTR, str(NON_PRIMARY_GID()))
    # Verify all of the attributes for obj4. Even though these are all 'invalid'
    # values, the file was copied to the destination because bucket to bucket
    # with preserve POSIX enabled will blindly copy the object metadata.
    self.VerifyObjectCustomAttribute(dst_bucket.bucket_name, 'obj4',
                                     GID_ATTR, str(INVALID_GID()))
    self.VerifyObjectCustomAttribute(dst_bucket.bucket_name, 'obj4',
                                     UID_ATTR, str(INVALID_UID()))
    self.VerifyObjectCustomAttribute(dst_bucket.bucket_name, 'obj4',
                                     MODE_ATTR, '222')
    # Verify obj5 attributes were copied.
    self.VerifyObjectCustomAttribute(dst_bucket.bucket_name, 'obj5',
                                     UID_ATTR, str(USER_ID))
    self.VerifyObjectCustomAttribute(dst_bucket.bucket_name, 'obj5',
                                     GID_ATTR, str(PRIMARY_GID))
    self.VerifyObjectCustomAttribute(dst_bucket.bucket_name, 'obj5',
                                     MODE_ATTR, str(DEFAULT_MODE))

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check2():
      """Check that we are not patching destination metadata a second time."""
      stderr = self.RunGsUtil(['rsync', '-P', suri(src_bucket),
                               suri(dst_bucket)], return_stderr=True)
      self.assertNotIn('Copying POSIX attributes from src to dst for', stderr)
    _Check2()

  def test_bucket_to_bucket_same_objects_src_mtime(self):
    """Tests bucket to bucket with mtime.

    Each has the same items but only the source has mtime stored in its
    metadata.
    Ensure that destination now also has the mtime of the files in its metadata.
    """
    # Create 2 buckets where the source and destination have 2 objects each with
    # the same name and content, where mtime is only set on src_bucket.
    src_bucket = self.CreateBucket()
    dst_bucket = self.CreateBucket()
    self.CreateObject(bucket_uri=src_bucket, object_name='obj1',
                      contents='obj1', mtime=0)
    self.CreateObject(bucket_uri=src_bucket, object_name='subdir/obj2',
                      contents='subdir/obj2', mtime=1)
    self.CreateObject(bucket_uri=dst_bucket, object_name='obj1',
                      contents='obj1')
    self.CreateObject(bucket_uri=dst_bucket, object_name='subdir/obj2',
                      contents='subdir/obj2')

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check1():
      """Tests rsync works as expected."""
      self.RunGsUtil(['rsync', '-r', suri(src_bucket), suri(dst_bucket)])
      listing1 = TailSet(suri(src_bucket), self.FlatListBucket(src_bucket))
      # First bucket should have un-altered content.
      self.assertEquals(listing1, set(['/obj1', '/subdir/obj2']))
    _Check1()

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check2():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', suri(src_bucket), suri(dst_bucket)], return_stderr=True))
    _Check2()

    # Verify objects' mtime in dst_bucket
    self._VerifyObjectMtime(dst_bucket.bucket_name, 'obj1', '0')
    self._VerifyObjectMtime(dst_bucket.bucket_name, 'subdir/obj2', '1')

  def test_bucket_to_bucket_src_mtime(self):
    """Tests bucket to bucket where source has mtime in files."""
    # Create 2 buckets where the source has 2 objects one at root level and the
    # other in a subdirectory. The other bucket will be empty.
    src_bucket = self.CreateBucket()
    dst_bucket = self.CreateBucket()
    obj1 = self.CreateObject(bucket_uri=src_bucket, object_name='obj1',
                             contents='obj1', mtime=0)
    obj2 = self.CreateObject(bucket_uri=src_bucket, object_name='subdir/obj2',
                             contents='subdir/obj2', mtime=1)
    # Verify objects' mtime in the buckets
    self._VerifyObjectMtime(obj1.bucket_name, obj1.object_name, '0')
    self._VerifyObjectMtime(obj2.bucket_name, obj2.object_name, '1')

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check1():
      """Tests rsync works as expected."""
      self.RunGsUtil(['rsync', '-r', suri(src_bucket), suri(dst_bucket)])
      listing1 = TailSet(suri(src_bucket), self.FlatListBucket(src_bucket))
      listing2 = TailSet(suri(dst_bucket), self.FlatListBucket(dst_bucket))
      # First bucket should have un-altered content.
      self.assertEquals(listing1, set(['/obj1', '/subdir/obj2']))
      # Second bucket should have new objects added from source bucket.
      self.assertEquals(listing2, set(['/obj1', '/subdir/obj2']))
    _Check1()

    # Get and verify the metadata for the 2 objects at the destination.
    self._VerifyObjectMtime(dst_bucket.bucket_name, 'obj1', '0')
    self._VerifyObjectMtime(dst_bucket.bucket_name, 'subdir/obj2', '1')

  def test_bucket_to_bucket_dst_mtime(self):
    """Tests bucket to bucket where destination has mtime in objects."""
    # Create 2 buckets where the source has files without mtime and the
    # destination has it present in its object's metadata. For obj1 and obj6
    # this tests the behavior where the file names are the same but the content
    # is different. obj1 has no mtime at source, but obj6 has mtime at src that
    # matches dst.
    src_bucket = self.CreateBucket()
    dst_bucket = self.CreateBucket()
    self.CreateObject(bucket_uri=src_bucket, object_name='obj1',
                      contents='OBJ1')
    self.CreateObject(bucket_uri=src_bucket, object_name='subdir/obj2',
                      contents='subdir/obj2')
    self.CreateObject(bucket_uri=src_bucket, object_name='.obj3',
                      contents='.obj3')
    self.CreateObject(bucket_uri=src_bucket, object_name='subdir/obj4',
                      contents='subdir/obj4')
    self.CreateObject(bucket_uri=src_bucket, object_name='obj6',
                      contents='OBJ6', mtime=100)
    self.CreateObject(bucket_uri=dst_bucket, object_name='obj1',
                      contents='obj1', mtime=10)
    self.CreateObject(bucket_uri=dst_bucket, object_name='subdir/obj2',
                      contents='subdir/obj2', mtime=10)
    self.CreateObject(bucket_uri=dst_bucket, object_name='.obj3',
                      contents='.OBJ3', mtime=1000000000000L)
    self.CreateObject(bucket_uri=dst_bucket, object_name='subdir/obj5',
                      contents='subdir/obj5', mtime=10)
    self.CreateObject(bucket_uri=dst_bucket, object_name='obj6',
                      contents='obj6', mtime=100)

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check1():
      """Tests rsync works as expected."""
      self.RunGsUtil(['rsync', '-r', '-d', suri(src_bucket), suri(dst_bucket)])
      listing1 = TailSet(suri(src_bucket), self.FlatListBucket(src_bucket))
      listing2 = TailSet(suri(dst_bucket), self.FlatListBucket(dst_bucket))
      # First bucket should have un-altered content.
      self.assertEquals(listing1, set(['/obj1', '/subdir/obj2', '/.obj3',
                                       '/subdir/obj4', '/obj6']))
      # Second bucket should have new objects added from source bucket.
      self.assertEquals(listing2, set(['/obj1', '/subdir/obj2', '/.obj3',
                                       '/subdir/obj4', '/obj6']))
    _Check1()

    # Get and verify the metadata for the objects at the destination.
    self._VerifyObjectMtime(dst_bucket.bucket_name, 'obj1', NA_TIME,
                            expected_present=False)
    self._VerifyObjectMtime(dst_bucket.bucket_name, 'subdir/obj2', '10')
    self._VerifyObjectMtime(dst_bucket.bucket_name, 'subdir/obj4', NA_TIME,
                            expected_present=False)

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check3():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', suri(src_bucket), suri(dst_bucket)], return_stderr=True))
    _Check3()

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check4():
      # Check that obj1 changed because mtime was not available; hashes were
      # used to compare the objects.
      self.assertEquals('OBJ1', self.RunGsUtil(
          ['cat', suri(dst_bucket, 'obj1')], return_stdout=True))
      # Ensure that .obj3 was updated even though its modification time comes
      # after the creation time of .obj3 at the source.
      self.assertEquals('.obj3', self.RunGsUtil(
          ['cat', suri(dst_bucket, '.obj3')], return_stdout=True))
      # Check that obj6 was updated even though the mtimes match. In this case
      # bucket to bucket sync will compare hashes.
      self.assertEquals('OBJ6', self.RunGsUtil(
          ['cat', suri(dst_bucket, 'obj6')], return_stdout=True))
    _Check4()

    # Now rerun the rsync with the -c option.
    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check5():
      """Tests rsync -c works as expected."""
      self.RunGsUtil(['rsync', '-r', '-d', '-c', suri(src_bucket),
                      suri(dst_bucket)])
      listing1 = TailSet(suri(src_bucket), self.FlatListBucket(src_bucket))
      listing2 = TailSet(suri(dst_bucket), self.FlatListBucket(dst_bucket))
      # First bucket should have un-altered content.
      self.assertEquals(listing1, set(['/obj1', '/subdir/obj2', '/.obj3',
                                       '/subdir/obj4', '/obj6']))
      # Second bucket should have new objects added from source bucket.
      self.assertEquals(listing2, set(['/obj1', '/subdir/obj2', '/.obj3',
                                       '/subdir/obj4', '/obj6']))
      # Assert that the contents of obj6 have now changed because the -c flag
      # was used to force checksums.
      self.assertEquals('OBJ6', self.RunGsUtil(
          ['cat', suri(dst_bucket, 'obj6')], return_stdout=True))
      # Verify the mtime for obj6 is correct.
      self._VerifyObjectMtime(dst_bucket.bucket_name, 'obj6', '100')
    _Check5()

  def test_bucket_to_bucket(self):
    """Tests that flat and recursive rsync between 2 buckets works correctly."""
    # Create 2 buckets with 1 overlapping object, 1 extra object at root level
    # in each, and 1 extra object 1 level down in each, where one of the objects
    # starts with "." to test that we don't skip those objects. Make the
    # overlapping objects named the same but with different content, to test
    # that we detect and properly copy in that case.
    bucket1_uri = self.CreateBucket()
    bucket2_uri = self.CreateBucket()
    self.CreateObject(bucket_uri=bucket1_uri, object_name='obj1',
                      contents='obj1')
    self.CreateObject(bucket_uri=bucket1_uri, object_name='.obj2',
                      contents='.obj2', mtime=10)
    self.CreateObject(bucket_uri=bucket1_uri, object_name='subdir/obj3',
                      contents='subdir/obj3')
    self.CreateObject(bucket_uri=bucket1_uri, object_name='obj6',
                      contents='obj6_', mtime=100)
    # .obj2 will be replaced and have mtime of 10
    self.CreateObject(bucket_uri=bucket2_uri, object_name='.obj2',
                      contents='.OBJ2')
    self.CreateObject(bucket_uri=bucket2_uri, object_name='obj4',
                      contents='obj4')
    self.CreateObject(bucket_uri=bucket2_uri, object_name='subdir/obj5',
                      contents='subdir/obj5')
    self.CreateObject(bucket_uri=bucket2_uri, object_name='obj6',
                      contents='obj6', mtime=100)

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check1():
      """Tests rsync works as expected."""
      self.RunGsUtil(['rsync', suri(bucket1_uri), suri(bucket2_uri)])
      listing1 = TailSet(suri(bucket1_uri), self.FlatListBucket(bucket1_uri))
      listing2 = TailSet(suri(bucket2_uri), self.FlatListBucket(bucket2_uri))
      # First bucket should have un-altered content.
      self.assertEquals(listing1, set(['/obj1', '/.obj2', '/subdir/obj3',
                                       '/obj6']))
      # Second bucket should have new objects added from source bucket (without
      # removing extraneeous object found in dest bucket), and without the
      # subdir objects synchronized.
      self.assertEquals(listing2, set(['/obj1', '/.obj2', '/obj4',
                                       '/subdir/obj5', '/obj6']))
      # Assert that the src/dest objects that had same length but different
      # content were correctly synchronized (bucket to bucket rsync uses
      # checksums).
      self.assertEquals('.obj2', self.RunGsUtil(
          ['cat', suri(bucket1_uri, '.obj2')], return_stdout=True))
      self.assertEquals('.obj2', self.RunGsUtil(
          ['cat', suri(bucket2_uri, '.obj2')], return_stdout=True))
      self.assertEquals('obj6_', self.RunGsUtil(
          ['cat', suri(bucket2_uri, 'obj6')], return_stdout=True))
      # Verify that .obj2 had its mtime updated at the destination.
      self._VerifyObjectMtime(bucket2_uri.bucket_name, '.obj2', '10')
    _Check1()

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check2():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', suri(bucket1_uri), suri(bucket2_uri)], return_stderr=True))
    _Check2()

    # Now add, overwrite, and remove some objects in each bucket and test
    # rsync -r.
    self.CreateObject(bucket_uri=bucket1_uri, object_name='obj6',
                      contents='obj6')
    self.CreateObject(bucket_uri=bucket2_uri, object_name='obj7',
                      contents='obj7')
    self.RunGsUtil(['rm', suri(bucket1_uri, 'obj1')])
    self.RunGsUtil(['rm', suri(bucket2_uri, '.obj2')])

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check3():
      self.RunGsUtil(['rsync', '-r', suri(bucket1_uri), suri(bucket2_uri)])
      listing1 = TailSet(suri(bucket1_uri), self.FlatListBucket(bucket1_uri))
      listing2 = TailSet(suri(bucket2_uri), self.FlatListBucket(bucket2_uri))
      # First bucket should have un-altered content.
      self.assertEquals(listing1, set(['/.obj2', '/obj6', '/subdir/obj3']))
      # Second bucket should have objects tha were newly added to first bucket
      # (wihout removing extraneous dest bucket objects), and without the
      # subdir objects synchronized.
      self.assertEquals(listing2, set(['/obj1', '/.obj2', '/obj4', '/obj6',
                                       '/obj7', '/subdir/obj3',
                                       '/subdir/obj5']))
    _Check3()

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check4():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-r', suri(bucket1_uri), suri(bucket2_uri)],
          return_stderr=True))
    _Check4()

  def test_bucket_to_bucket_minus_d(self):
    """Tests that flat and recursive rsync between 2 buckets works correctly."""
    # Create 2 buckets with 1 overlapping object, 1 extra object at root level
    # in each, and 1 extra object 1 level down in each, where one of the objects
    # starts with "." to test that we don't skip those objects. Make the
    # overlapping objects named the same but with different content, to test
    # that we detect and properly copy in that case.
    bucket1_uri = self.CreateBucket()
    bucket2_uri = self.CreateBucket()
    self.CreateObject(bucket_uri=bucket1_uri, object_name='obj1',
                      contents='obj1')
    self.CreateObject(bucket_uri=bucket1_uri, object_name='.obj2',
                      contents='.obj2')
    self.CreateObject(bucket_uri=bucket1_uri, object_name='subdir/obj3',
                      contents='subdir/obj3')
    self.CreateObject(bucket_uri=bucket2_uri, object_name='.obj2',
                      contents='.OBJ2')
    self.CreateObject(bucket_uri=bucket2_uri, object_name='obj4',
                      contents='obj4')
    self.CreateObject(bucket_uri=bucket2_uri, object_name='subdir/obj5',
                      contents='subdir/obj5')

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check1():
      """Tests rsync works as expected."""
      self.RunGsUtil(['rsync', '-d', suri(bucket1_uri), suri(bucket2_uri)])
      listing1 = TailSet(suri(bucket1_uri), self.FlatListBucket(bucket1_uri))
      listing2 = TailSet(suri(bucket2_uri), self.FlatListBucket(bucket2_uri))
      # First bucket should have un-altered content.
      self.assertEquals(listing1, set(['/obj1', '/.obj2', '/subdir/obj3']))
      # Second bucket should have content like first bucket but without the
      # subdir objects synchronized.
      self.assertEquals(listing2, set(['/obj1', '/.obj2', '/subdir/obj5']))
      # Assert that the src/dest objects that had same length but different
      # content were correctly synchronized (bucket to bucket rsync uses
      # checksums).
      self.assertEquals('.obj2', self.RunGsUtil(
          ['cat', suri(bucket1_uri, '.obj2')], return_stdout=True))
      self.assertEquals('.obj2', self.RunGsUtil(
          ['cat', suri(bucket2_uri, '.obj2')], return_stdout=True))
    _Check1()

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check2():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-d', suri(bucket1_uri), suri(bucket2_uri)],
          return_stderr=True))
    _Check2()

    # Now add and remove some objects in each bucket and test rsync -r.
    self.CreateObject(bucket_uri=bucket1_uri, object_name='obj6',
                      contents='obj6')
    self.CreateObject(bucket_uri=bucket2_uri, object_name='obj7',
                      contents='obj7')
    self.RunGsUtil(['rm', suri(bucket1_uri, 'obj1')])
    self.RunGsUtil(['rm', suri(bucket2_uri, '.obj2')])

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check3():
      self.RunGsUtil(['rsync', '-d', '-r',
                      suri(bucket1_uri), suri(bucket2_uri)])
      listing1 = TailSet(suri(bucket1_uri), self.FlatListBucket(bucket1_uri))
      listing2 = TailSet(suri(bucket2_uri), self.FlatListBucket(bucket2_uri))
      # First bucket should have un-altered content.
      self.assertEquals(listing1, set(['/.obj2', '/obj6', '/subdir/obj3']))
      # Second bucket should have content like first bucket but without the
      # subdir objects synchronized.
      self.assertEquals(listing2, set(['/.obj2', '/obj6', '/subdir/obj3']))
    _Check3()

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check4():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-d', '-r', suri(bucket1_uri), suri(bucket2_uri)],
          return_stderr=True))
    _Check4()

  # Test sequential upload as well as parallel composite upload case.
  @SequentialAndParallelTransfer
  @unittest.skipUnless(UsingCrcmodExtension(crcmod),
                       'Test requires fast crcmod.')
  def test_dir_to_bucket_mtime(self):
    """Tests dir to bucket with mtime.

    Each has the same items, the source has mtime for all objects, whereas dst
    only has mtime for obj5 and obj6 to test for different a later mtime at src
    and the same mtime from src to dst, respectively. Ensure that destination
    now also has the mtime of the files in its metadata.
    """
    # Create directory and bucket, where the directory has different
    # combinations of mtime and sub-directories.
    tmpdir = self.CreateTempDir()
    subdir = os.path.join(tmpdir, 'subdir')
    os.mkdir(subdir)
    self.CreateTempFile(tmpdir=tmpdir, file_name='obj1',
                        contents='obj1', mtime=10)
    self.CreateTempFile(tmpdir=tmpdir, file_name='.obj2',
                        contents='.obj2', mtime=10)
    self.CreateTempFile(tmpdir=subdir, file_name='obj3',
                        contents='subdir/obj3', mtime=10)
    self.CreateTempFile(tmpdir=subdir, file_name='obj5',
                        contents='subdir/obj5', mtime=15)
    self.CreateTempFile(tmpdir=tmpdir, file_name='obj6',
                        contents='obj6', mtime=100)
    self.CreateTempFile(tmpdir=tmpdir, file_name='obj7',
                        contents='obj7_', mtime=100)
    bucket_uri = self.CreateBucket()
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj1',
                      contents='OBJ1')
    self.CreateObject(bucket_uri=bucket_uri, object_name='.obj2',
                      contents='.obj2')
    self._SetObjectCustomMetadataAttribute(self.default_provider,
                                           bucket_uri.bucket_name, '.obj2',
                                           'test', 'test')
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj4',
                      contents='obj4')
    self.CreateObject(bucket_uri=bucket_uri, object_name='subdir/obj5',
                      contents='subdir/obj5', mtime=10)
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj6',
                      contents='OBJ6', mtime=100)
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj7',
                      contents='obj7', mtime=100)

    cumulative_stderr = set()
    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check1():
      """Tests rsync works as expected."""
      stderr = self.RunGsUtil(
          ['rsync', '-r', '-d', tmpdir, suri(bucket_uri)], return_stderr=True)
      cumulative_stderr.update([s for s in stderr.splitlines() if s])
      listing1 = TailSet(tmpdir, self.FlatListDir(tmpdir))
      listing2 = TailSet(suri(bucket_uri), self.FlatListBucket(bucket_uri))
      # Dir should have un-altered content.
      self.assertEquals(listing1, set(['/obj1', '/.obj2', '/subdir/obj3',
                                       '/subdir/obj5', '/obj6', '/obj7']))
      # Bucket should have content like dir.
      self.assertEquals(listing2, set(['/obj1', '/.obj2', '/subdir/obj3',
                                       '/subdir/obj5', '/obj6', '/obj7']))
      # Check that obj6 didn't change even though the contents did. This is
      # because the object/file have the same mtime.
      self.assertEquals('OBJ6', self.RunGsUtil(
          ['cat', suri(bucket_uri, 'obj6')], return_stdout=True))
      # Check that obj7 changed because the size was different.
      self.assertEquals('obj7_', self.RunGsUtil(
          ['cat', suri(bucket_uri, 'obj7')], return_stdout=True))
    _Check1()

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check2():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-r', '-d', tmpdir, suri(bucket_uri)], return_stderr=True))
    _Check2()

    # Verify objects' mtime in the bucket.
    self._VerifyObjectMtime(bucket_uri.bucket_name, 'obj1', '10')
    self._VerifyObjectMtime(bucket_uri.bucket_name, '.obj2', '10')
    self._VerifyObjectMtime(bucket_uri.bucket_name, 'subdir/obj3', '10')
    self._VerifyObjectMtime(bucket_uri.bucket_name, 'subdir/obj5', '15')
    self._VerifyObjectMtime(bucket_uri.bucket_name, 'obj6', '100')
    # Rsync using S3 without object owner permission will copy over old objects.
    copied_over_object_notice = (
        'Copying whole file/object for %s instead of patching because you '
        'don\'t have owner permission on the object.' %
        suri(bucket_uri, '.obj2'))
    if copied_over_object_notice not in cumulative_stderr:
      # Make sure test attribute wasn't blown away when mtime was updated.
      self.VerifyObjectCustomAttribute(
          bucket_uri.bucket_name, '.obj2', 'test', 'test')

    # Now rerun the rsync with the -c option.
    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check4():
      """Tests rsync -c works as expected."""
      self.RunGsUtil(['rsync', '-r', '-d', '-c', tmpdir, suri(bucket_uri)])
      listing1 = TailSet(tmpdir, self.FlatListDir(tmpdir))
      listing2 = TailSet(suri(bucket_uri), self.FlatListBucket(bucket_uri))
      # Dir should have un-altered content.
      self.assertEquals(listing1, set(['/obj1', '/.obj2', '/subdir/obj3',
                                       '/subdir/obj5', '/obj6', '/obj7']))
      # Bucket should have content like dir with the subdirectories synced.
      self.assertEquals(listing2, set(['/obj1', '/.obj2', '/subdir/obj3',
                                       '/subdir/obj5', '/obj6', '/obj7']))
      # Assert that the contents of obj6 have now changed because the -c flag
      # was used to force checksums.
      self.assertEquals('obj6', self.RunGsUtil(
          ['cat', suri(bucket_uri, 'obj6')], return_stdout=True))
      self._VerifyObjectMtime(bucket_uri.bucket_name, 'obj6', '100')
    _Check4()

  # Test sequential upload as well as parallel composite upload case.
  @SequentialAndParallelTransfer
  @unittest.skipUnless(UsingCrcmodExtension(crcmod),
                       'Test requires fast crcmod.')
  def test_dir_to_bucket_seek_ahead(self):
    """Tests that rsync seek-ahead iterator works correctly."""
    # Unfortunately, we have to retry the entire operation in the case of
    # eventual consistency because the estimated values will differ.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check1():
      """Test estimating an rsync upload operation."""
      tmpdir = self.CreateTempDir()
      subdir = os.path.join(tmpdir, 'subdir')
      os.mkdir(subdir)
      self.CreateTempFile(tmpdir=tmpdir, file_name='obj1',
                          contents='obj1')
      self.CreateTempFile(tmpdir=tmpdir, file_name='.obj2',
                          contents='.obj2')
      self.CreateTempFile(tmpdir=subdir, file_name='obj3',
                          contents='subdir/obj3')
      bucket_uri = self.CreateBucket()
      self.CreateObject(bucket_uri=bucket_uri, object_name='.obj2',
                        contents='.OBJ2')
      self.CreateObject(bucket_uri=bucket_uri, object_name='obj4',
                        contents='obj4')
      self.CreateObject(bucket_uri=bucket_uri, object_name='subdir/obj5',
                        contents='subdir/obj5')
      # Need to make sure the bucket listing is caught-up, otherwise the
      # first rsync may not see .obj2 and overwrite it.
      self.AssertNObjectsInBucket(bucket_uri, 3)

      with SetBotoConfigForTest([('GSUtil', 'task_estimation_threshold', '1'),
                                 ('GSUtil', 'task_estimation_force', 'True')]):
        stderr = self.RunGsUtil(
            ['-m', 'rsync', '-d', '-r', tmpdir, suri(bucket_uri)],
            return_stderr=True)
        # Objects: 4 (2 removed, 2 added)
        # Bytes: 15 (added objects are 4 bytes and 11 bytes, respectively).
        self.assertIn(
            'Estimated work for this command: objects: 5, total size: 20',
            stderr)

        self.AssertNObjectsInBucket(bucket_uri, 3)
        # Re-running should produce no estimate, because there is no work to do.
        stderr = self.RunGsUtil(
            ['-m', 'rsync', '-d', '-r', tmpdir, suri(bucket_uri)],
            return_stderr=True)
        self.assertNotIn('Estimated work', stderr)

    _Check1()

    tmpdir = self.CreateTempDir(test_files=1)
    bucket_uri = self.CreateBucket()
    # Running with task estimation turned off (and work to perform) should not
    # produce an estimate.
    with SetBotoConfigForTest([('GSUtil', 'task_estimation_threshold', '0'),
                               ('GSUtil', 'task_estimation_force', 'True')]):
      stderr = self.RunGsUtil(
          ['-m', 'rsync', '-d', '-r', tmpdir, suri(bucket_uri)],
          return_stderr=True)
      self.assertNotIn('Estimated work', stderr)

  # Test sequential upload as well as parallel composite upload case.
  @SequentialAndParallelTransfer
  @unittest.skipUnless(UsingCrcmodExtension(crcmod),
                       'Test requires fast crcmod.')
  def test_dir_to_bucket_minus_d(self):
    """Tests that flat and recursive rsync dir to bucket works correctly."""
    # Create dir and bucket with 1 overlapping object, 1 extra object at root
    # level in each, and 1 extra object 1 level down in each, where one of the
    # objects starts with "." to test that we don't skip those objects. Make the
    # overlapping objects named the same but with different content, to test
    # that we detect and properly copy in that case.
    tmpdir = self.CreateTempDir()
    subdir = os.path.join(tmpdir, 'subdir')
    os.mkdir(subdir)
    bucket_uri = self.CreateBucket()
    self.CreateTempFile(tmpdir=tmpdir, file_name='obj1', contents='obj1')
    self.CreateTempFile(tmpdir=tmpdir, file_name='.obj2', contents='.obj2')
    self.CreateTempFile(tmpdir=subdir, file_name='obj3', contents='subdir/obj3')
    self.CreateObject(bucket_uri=bucket_uri, object_name='.obj2',
                      contents='.OBJ2')
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj4',
                      contents='obj4')
    self.CreateObject(bucket_uri=bucket_uri, object_name='subdir/obj5',
                      contents='subdir/obj5')

    # Need to make sure the bucket listing is caught-up, otherwise the
    # first rsync may not see .obj2 and overwrite it.
    self.AssertNObjectsInBucket(bucket_uri, 3)

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check1():
      """Tests rsync works as expected."""
      self.RunGsUtil(['rsync', '-d', tmpdir, suri(bucket_uri)])
      listing1 = TailSet(tmpdir, self.FlatListDir(tmpdir))
      listing2 = TailSet(suri(bucket_uri), self.FlatListBucket(bucket_uri))
      # Dir should have un-altered content.
      self.assertEquals(listing1, set(['/obj1', '/.obj2', '/subdir/obj3']))
      # Bucket should have content like dir but without the subdir objects
      # synchronized.
      self.assertEquals(listing2, set(['/obj1', '/.obj2', '/subdir/obj5']))
      # Assert that the src/dest objects that had same length but different
      # content were synchronized (dir to bucket rsync uses checksums as a
      # backup to mtime unless you specify -c to make hashes the priority).
      with open(os.path.join(tmpdir, '.obj2')) as f:
        self.assertEquals('.obj2', '\n'.join(f.readlines()))
      self.assertEquals('.obj2', self.RunGsUtil(
          ['cat', suri(bucket_uri, '.obj2')], return_stdout=True))
    _Check1()

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check2():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-d', tmpdir, suri(bucket_uri)], return_stderr=True))
    _Check2()

    # Now rerun the rsync with the -c option.
    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check3():
      """Tests rsync -c works as expected."""
      self.RunGsUtil(['rsync', '-d', '-c', tmpdir, suri(bucket_uri)])
      listing1 = TailSet(tmpdir, self.FlatListDir(tmpdir))
      listing2 = TailSet(suri(bucket_uri), self.FlatListBucket(bucket_uri))
      # Dir should have un-altered content.
      self.assertEquals(listing1, set(['/obj1', '/.obj2', '/subdir/obj3']))
      # Bucket should have content like dir but without the subdir objects
      # synchronized.
      self.assertEquals(listing2, set(['/obj1', '/.obj2', '/subdir/obj5']))
      # Assert that the src/dest objects that had same length but different
      # content were synchronized (dir to bucket rsync with -c uses checksums).
      with open(os.path.join(tmpdir, '.obj2')) as f:
        self.assertEquals('.obj2', '\n'.join(f.readlines()))
      self.assertEquals('.obj2', self.RunGsUtil(
          ['cat', suri(bucket_uri, '.obj2')], return_stdout=True))
    _Check3()

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check4():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-d', '-c', tmpdir, suri(bucket_uri)], return_stderr=True))
    _Check4()

    # Now add and remove some objects in dir and bucket and test rsync -r.
    self.CreateTempFile(tmpdir=tmpdir, file_name='obj6', contents='obj6')
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj7',
                      contents='obj7')
    os.unlink(os.path.join(tmpdir, 'obj1'))
    self.RunGsUtil(['rm', suri(bucket_uri, '.obj2')])

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check5():
      self.RunGsUtil(['rsync', '-d', '-r', tmpdir, suri(bucket_uri)])
      listing1 = TailSet(tmpdir, self.FlatListDir(tmpdir))
      listing2 = TailSet(suri(bucket_uri), self.FlatListBucket(bucket_uri))
      # Dir should have un-altered content.
      self.assertEquals(listing1, set(['/.obj2', '/obj6', '/subdir/obj3']))
      # Bucket should have content like dir but without the subdir objects
      # synchronized.
      self.assertEquals(listing2, set(['/.obj2', '/obj6', '/subdir/obj3']))
    _Check5()

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check6():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-d', '-r', tmpdir, suri(bucket_uri)], return_stderr=True))
    _Check6()

  @unittest.skipUnless(UsingCrcmodExtension(crcmod),
                       'Test requires fast crcmod.')
  def test_dir_to_dir_mtime(self):
    """Tests that flat and recursive rsync dir to dir works correctly."""
    # Create 2 dirs with 1 overlapping file, 1 extra file at root
    # level in each, and 1 extra file 1 level down in each, where one of the
    # objects starts with "." to test that we don't skip those objects. Make the
    # overlapping files named the same but with different content, to test
    # that we detect and properly copy in that case.
    tmpdir1 = self.CreateTempDir()
    tmpdir2 = self.CreateTempDir()
    subdir1 = os.path.join(tmpdir1, 'subdir1')
    subdir2 = os.path.join(tmpdir2, 'subdir2')
    os.mkdir(subdir1)
    os.mkdir(subdir2)
    self.CreateTempFile(tmpdir=tmpdir1, file_name='obj1', contents='obj1',
                        mtime=10)
    self.CreateTempFile(tmpdir=tmpdir1, file_name='.obj2', contents='.obj2',
                        mtime=10)
    self.CreateTempFile(tmpdir=subdir1, file_name='obj3',
                        contents='subdir1/obj3', mtime=10)
    self.CreateTempFile(tmpdir=tmpdir1, file_name='obj6', contents='obj6',
                        mtime=100)
    self.CreateTempFile(tmpdir=tmpdir1, file_name='obj7', contents='obj7_',
                        mtime=100)
    self.CreateTempFile(tmpdir=tmpdir2, file_name='.obj2', contents='.OBJ2',
                        mtime=1000)
    self.CreateTempFile(tmpdir=tmpdir2, file_name='obj4', contents='obj4',
                        mtime=10)
    self.CreateTempFile(tmpdir=subdir2, file_name='obj5',
                        contents='subdir2/obj5', mtime=10)
    self.CreateTempFile(tmpdir=tmpdir2, file_name='obj6', contents='OBJ6',
                        mtime=100)
    self.CreateTempFile(tmpdir=tmpdir2, file_name='obj7', contents='obj7',
                        mtime=100)

    self.RunGsUtil(['rsync', '-r', '-d', tmpdir1, tmpdir2])
    listing1 = TailSet(tmpdir1, self.FlatListDir(tmpdir1))
    listing2 = TailSet(tmpdir2, self.FlatListDir(tmpdir2))
    # dir1 should have un-altered content.
    self.assertEquals(listing1, set(['/obj1', '/.obj2', '/subdir1/obj3',
                                     '/obj6', '/obj7']))
    # dir2 should now have content like dir1.
    self.assertEquals(listing2, set(['/obj1', '/.obj2', '/subdir1/obj3',
                                     '/obj6', '/obj7']))
    # Assert that the src/dest objects that had same length but different
    # checksums were synchronized properly according to mtime.
    with open(os.path.join(tmpdir2, '.obj2')) as f:
      self.assertEquals('.obj2', '\n'.join(f.readlines()))
    with open(os.path.join(tmpdir2, 'obj6')) as f:
      self.assertEquals('OBJ6', '\n'.join(f.readlines()))
    with open(os.path.join(tmpdir2, 'obj7')) as f:
      self.assertEquals('obj7_', '\n'.join(f.readlines()))

    def _Check1():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-d', tmpdir1, tmpdir2], return_stderr=True))
    _Check1()

    # Now rerun the rsync with the -c option.
    self.RunGsUtil(['rsync', '-r', '-d', '-c', tmpdir1, tmpdir2])
    listing1 = TailSet(tmpdir1, self.FlatListDir(tmpdir1))
    listing2 = TailSet(tmpdir2, self.FlatListDir(tmpdir2))
    # dir1 should have un-altered content.
    self.assertEquals(listing1, set(['/obj1', '/.obj2', '/subdir1/obj3',
                                     '/obj6', '/obj7']))
    # dir2 should now have content like dir1.
    self.assertEquals(listing2, set(['/obj1', '/.obj2', '/subdir1/obj3',
                                     '/obj6', '/obj7']))
    # Assert that the src/dst objects that had same length, mtime, but different
    # content were synchronized (dir to dir rsync with -c uses checksums).
    with open(os.path.join(tmpdir1, '.obj2')) as f:
      self.assertEquals('.obj2', '\n'.join(f.readlines()))
    with open(os.path.join(tmpdir1, '.obj2')) as f:
      self.assertEquals('.obj2', '\n'.join(f.readlines()))
    with open(os.path.join(tmpdir2, 'obj6')) as f:
      self.assertEquals('obj6', '\n'.join(f.readlines()))

    def _Check2():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-d', '-c', tmpdir1, tmpdir2], return_stderr=True))
    _Check2()

    # Now add and remove some objects in both dirs and test rsync -r.
    os.unlink(os.path.join(tmpdir1, 'obj7'))
    os.unlink(os.path.join(tmpdir2, 'obj7'))
    self.CreateTempFile(tmpdir=tmpdir1, file_name='obj6', contents='obj6',
                        mtime=10)
    self.CreateTempFile(tmpdir=tmpdir2, file_name='obj7', contents='obj7',
                        mtime=100)
    os.unlink(os.path.join(tmpdir1, 'obj1'))
    os.unlink(os.path.join(tmpdir2, '.obj2'))

    self.RunGsUtil(['rsync', '-d', '-r', tmpdir1, tmpdir2])
    listing1 = TailSet(tmpdir1, self.FlatListDir(tmpdir1))
    listing2 = TailSet(tmpdir2, self.FlatListDir(tmpdir2))
    # dir1 should have un-altered content.
    self.assertEquals(listing1, set(['/.obj2', '/obj6', '/subdir1/obj3']))
    # dir2 should have content like dir1.
    self.assertEquals(listing2, set(['/.obj2', '/obj6', '/subdir1/obj3']))

    def _Check3():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-d', '-r', tmpdir1, tmpdir2], return_stderr=True))
    _Check3()

  @unittest.skipUnless(UsingCrcmodExtension(crcmod),
                       'Test requires fast crcmod.')
  def test_dir_to_dir_minus_d(self):
    """Tests that flat and recursive rsync dir to dir works correctly."""
    # Create 2 dirs with 1 overlapping file, 1 extra file at root
    # level in each, and 1 extra file 1 level down in each, where one of the
    # objects starts with "." to test that we don't skip those objects. Make the
    # overlapping files named the same but with different content, to test
    # that we detect and properly copy in that case.
    tmpdir1 = self.CreateTempDir()
    tmpdir2 = self.CreateTempDir()
    subdir1 = os.path.join(tmpdir1, 'subdir1')
    subdir2 = os.path.join(tmpdir2, 'subdir2')
    os.mkdir(subdir1)
    os.mkdir(subdir2)
    self.CreateTempFile(tmpdir=tmpdir1, file_name='obj1', contents='obj1')
    self.CreateTempFile(tmpdir=tmpdir1, file_name='.obj2', contents='.obj2')
    self.CreateTempFile(
        tmpdir=subdir1, file_name='obj3', contents='subdir1/obj3')
    self.CreateTempFile(tmpdir=tmpdir2, file_name='.obj2', contents='.OBJ2')
    self.CreateTempFile(tmpdir=tmpdir2, file_name='obj4', contents='obj4')
    self.CreateTempFile(
        tmpdir=subdir2, file_name='obj5', contents='subdir2/obj5')

    self.RunGsUtil(['rsync', '-d', tmpdir1, tmpdir2])
    listing1 = TailSet(tmpdir1, self.FlatListDir(tmpdir1))
    listing2 = TailSet(tmpdir2, self.FlatListDir(tmpdir2))
    # dir1 should have un-altered content.
    self.assertEquals(listing1, set(['/obj1', '/.obj2', '/subdir1/obj3']))
    # dir2 should have content like dir1 but without the subdir1 objects
    # synchronized.
    self.assertEquals(listing2, set(['/obj1', '/.obj2', '/subdir2/obj5']))
    # Assert that the src/dest objects that had same length but different
    # checksums were not synchronized (dir to dir rsync doesn't use checksums
    # unless you specify -c).
    with open(os.path.join(tmpdir1, '.obj2')) as f:
      self.assertEquals('.obj2', '\n'.join(f.readlines()))
    with open(os.path.join(tmpdir2, '.obj2')) as f:
      self.assertEquals('.OBJ2', '\n'.join(f.readlines()))

    # Don't use @Retry since this is a dir-to-dir test, thus we don't need to
    # worry about eventual consistency of bucket listings. This also allows us
    # to make sure that we don't miss any unintended behavior when a first
    # attempt behaves incorrectly and a subsequent retry behaves correctly.
    def _Check1():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-d', tmpdir1, tmpdir2], return_stderr=True))
    _Check1()

    # Now rerun the rsync with the -c option.
    self.RunGsUtil(['rsync', '-d', '-c', tmpdir1, tmpdir2])
    listing1 = TailSet(tmpdir1, self.FlatListDir(tmpdir1))
    listing2 = TailSet(tmpdir2, self.FlatListDir(tmpdir2))
    # dir1 should have un-altered content.
    self.assertEquals(listing1, set(['/obj1', '/.obj2', '/subdir1/obj3']))
    # dir2 should have content like dir but without the subdir objects
    # synchronized.
    self.assertEquals(listing2, set(['/obj1', '/.obj2', '/subdir2/obj5']))
    # Assert that the src/dest objects that had same length but different
    # content were synchronized (dir to dir rsync with -c uses checksums).
    with open(os.path.join(tmpdir1, '.obj2')) as f:
      self.assertEquals('.obj2', '\n'.join(f.readlines()))
    with open(os.path.join(tmpdir1, '.obj2')) as f:
      self.assertEquals('.obj2', '\n'.join(f.readlines()))

    # Don't use @Retry since this is a dir-to-dir test, thus we don't need to
    # worry about eventual consistency of bucket listings.
    def _Check2():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-d', '-c', tmpdir1, tmpdir2], return_stderr=True))
    _Check2()

    # Now add and remove some objects in both dirs and test rsync -r.
    self.CreateTempFile(tmpdir=tmpdir1, file_name='obj6', contents='obj6')
    self.CreateTempFile(tmpdir=tmpdir2, file_name='obj7', contents='obj7')
    os.unlink(os.path.join(tmpdir1, 'obj1'))
    os.unlink(os.path.join(tmpdir2, '.obj2'))

    self.RunGsUtil(['rsync', '-d', '-r', tmpdir1, tmpdir2])
    listing1 = TailSet(tmpdir1, self.FlatListDir(tmpdir1))
    listing2 = TailSet(tmpdir2, self.FlatListDir(tmpdir2))
    # dir1 should have un-altered content.
    self.assertEquals(listing1, set(['/.obj2', '/obj6', '/subdir1/obj3']))
    # dir2 should have content like dir but without the subdir objects
    # synchronized.
    self.assertEquals(listing2, set(['/.obj2', '/obj6', '/subdir1/obj3']))

    # Don't use @Retry since this is a dir-to-dir test, thus we don't need to
    # worry about eventual consistency of bucket listings.
    def _Check3():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-d', '-r', tmpdir1, tmpdir2], return_stderr=True))
    _Check3()

    # Create 2 dirs and add a file to the first. Then create another file and
    # add it to both dirs, making sure its filename evaluates to greater than
    # the previous filename. Make sure both files are present in the second
    # dir after issuing an rsync -d command.
    tmpdir1 = self.CreateTempDir()
    tmpdir2 = self.CreateTempDir()
    self.CreateTempFile(tmpdir=tmpdir1, file_name='obj1', contents='obj1')
    self.CreateTempFile(tmpdir=tmpdir1, file_name='obj2', contents='obj2')
    self.CreateTempFile(tmpdir=tmpdir2, file_name='obj2', contents='obj2')

    self.RunGsUtil(['rsync', '-d', tmpdir1, tmpdir2])
    listing1 = TailSet(tmpdir1, self.FlatListDir(tmpdir1))
    listing2 = TailSet(tmpdir2, self.FlatListDir(tmpdir2))
    # First dir should have un-altered content.
    self.assertEquals(listing1, set(['/obj1', '/obj2']))
    # Second dir should have same content as first.
    self.assertEquals(listing2, set(['/obj1', '/obj2']))

    # Don't use @Retry since this is a dir-to-dir test, thus we don't need to
    # worry about eventual consistency of bucket listings.
    def _Check4():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-d', tmpdir1, tmpdir2], return_stderr=True))
    _Check4()

  def test_dir_to_dir_minus_d_more_files_than_bufsize(self):
    """Tests concurrently building listing from multiple tmp file ranges."""
    # Create 2 dirs, where each dir has 1000 objects and differing names.
    tmpdir1 = self.CreateTempDir()
    tmpdir2 = self.CreateTempDir()
    for i in range(0, 1000):
      self.CreateTempFile(tmpdir=tmpdir1, file_name='d1-%s' % i,
                          contents='x', mtime=(i+1))
      self.CreateTempFile(tmpdir=tmpdir2, file_name='d2-%s' % i,
                          contents='y', mtime=i)

    # We open a new temp file each time we reach rsync_buffer_lines of
    # listing output. On Windows, this will result in a 'too many open file
    # handles' error, so choose a larger value so as not to open so many files.
    rsync_buffer_config = [('GSUtil', 'rsync_buffer_lines',
                            '50' if IS_WINDOWS else '2')]
    # Run gsutil with config option to make buffer size << # files.
    with SetBotoConfigForTest(rsync_buffer_config):
      self.RunGsUtil(['rsync', '-d', tmpdir1, tmpdir2])
    listing1 = TailSet(tmpdir1, self.FlatListDir(tmpdir1))
    listing2 = TailSet(tmpdir2, self.FlatListDir(tmpdir2))
    self.assertEquals(listing1, listing2)
    for i in range(0, 1000):
      self.assertEquals(i + 1, long(os.path.getmtime((
          os.path.join(tmpdir2, 'd1-%s' % i)))))
      with open(os.path.join(tmpdir2, 'd1-%s' % i)) as f:
        self.assertEquals('x', '\n'.join(f.readlines()))

    # Don't use @Retry since this is a dir-to-dir test, thus we don't need to
    # worry about eventual consistency of bucket listings.
    def _Check():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-d', tmpdir1, tmpdir2], return_stderr=True))
    _Check()

  @unittest.skipUnless(UsingCrcmodExtension(crcmod),
                       'Test requires fast crcmod.')
  def test_bucket_to_dir_compressed_encoding(self):
    temp_file = self.CreateTempFile(contents='foo', file_name='bar')
    bucket_uri = self.CreateBucket()
    tmpdir = self.CreateTempDir()
    self.RunGsUtil(['cp', '-Z', temp_file, suri(bucket_uri)])
    stderr = self.RunGsUtil(['rsync', suri(bucket_uri), tmpdir],
                            return_stderr=True)
    # rsync should decompress the destination file.
    with open(os.path.join(tmpdir, 'bar'), 'rb') as fp:
      self.assertEqual('foo', fp.read())
    self.assertIn('bar has a compressed content-encoding', stderr)

  @SequentialAndParallelTransfer
  @unittest.skipUnless(UsingCrcmodExtension(crcmod),
                       'Test requires fast crcmod.')
  def test_bucket_to_dir_mtime(self):
    """Tests bucket to dir with mtime at the source."""
    # Create bucket and dir with overlapping content and other combinations of
    # mtime.
    bucket_uri = self.CreateBucket()
    tmpdir = self.CreateTempDir()
    subdir = os.path.join(tmpdir, 'subdir')
    os.mkdir(subdir)
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj1',
                      contents='obj1', mtime=5)
    self.CreateObject(bucket_uri=bucket_uri, object_name='.obj2',
                      contents='.obj2', mtime=5)
    self.CreateObject(bucket_uri=bucket_uri, object_name='subdir/obj3',
                      contents='subdir/obj3')
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj4',
                      contents='OBJ4')
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj6',
                      contents='obj6', mtime=50)
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj7',
                      contents='obj7', mtime=5)
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj8',
                      contents='obj8', mtime=100)
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj9',
                      contents='obj9', mtime=25)
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj10',
                      contents='obj10')
    time_created = ConvertDatetimeToPOSIX(self._GetMetadataAttribute(
        bucket_uri.bucket_name, 'obj10', 'timeCreated'))
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj11',
                      contents='obj11_', mtime=75)
    self.CreateTempFile(tmpdir=tmpdir, file_name='.obj2', contents='.OBJ2',
                        mtime=10)
    self.CreateTempFile(tmpdir=tmpdir, file_name='obj4', contents='obj4',
                        mtime=100)
    self.CreateTempFile(tmpdir=subdir, file_name='obj5', contents='subdir/obj5',
                        mtime=10)
    self.CreateTempFile(tmpdir=tmpdir, file_name='obj6',
                        contents='obj6', mtime=50)
    self.CreateTempFile(tmpdir=tmpdir, file_name='obj7',
                        contents='OBJ7', mtime=50)
    self.CreateTempFile(tmpdir=tmpdir, file_name='obj8',
                        contents='obj8', mtime=10)
    self.CreateTempFile(tmpdir=tmpdir, file_name='obj9',
                        contents='OBJ9', mtime=25)
    self.CreateTempFile(tmpdir=tmpdir, file_name='obj10',
                        contents='OBJ10', mtime=time_created)
    self.CreateTempFile(tmpdir=tmpdir, file_name='obj11',
                        contents='obj11', mtime=75)

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check1():
      """Tests rsync works as expected."""
      self.RunGsUtil(['rsync', '-d', suri(bucket_uri), tmpdir])
      listing1 = TailSet(suri(bucket_uri), self.FlatListBucket(bucket_uri))
      listing2 = TailSet(tmpdir, self.FlatListDir(tmpdir))
      # Bucket should have un-altered content.
      self.assertEquals(listing1, set(['/obj1', '/.obj2', '/subdir/obj3',
                                       '/obj4', '/obj6', '/obj7', '/obj8',
                                       '/obj9', '/obj10', '/obj11']))
      # Dir should have content like bucket except without sub-directories
      # synced.
      self.assertEquals(listing2, set(['/obj1', '/.obj2', '/obj4',
                                       '/subdir/obj5', '/obj6', '/obj7',
                                       '/obj8', '/obj9', '/obj10', '/obj11']))
      # Assert that the dst objects that had an earlier mtime were not
      # synchronized because the source didn't have an mtime.
      with open(os.path.join(tmpdir, '.obj2')) as f:
        self.assertEquals('.obj2', '\n'.join(f.readlines()))
      # Assert that obj4 was synchronized to dst because mtime cannot be used,
      # and the hashes are different.
      with open(os.path.join(tmpdir, 'obj4')) as f:
        self.assertEquals('OBJ4', '\n'.join(f.readlines()))
      # Verify obj9 and obj10 content didn't change because mtimes from src and
      # dst were equal. obj9 had mtimes that matched while the obj10 mtime was
      # equal to the creation time of the corresponding object.
      with open(os.path.join(tmpdir, 'obj9')) as f:
        self.assertEquals('OBJ9', '\n'.join(f.readlines()))
      # Also verifies if obj10 used time created to determine if a copy was
      # necessary.
      with open(os.path.join(tmpdir, 'obj10')) as f:
        self.assertEquals('OBJ10', '\n'.join(f.readlines()))
      with open(os.path.join(tmpdir, 'obj11')) as f:
        self.assertEquals('obj11_', '\n'.join(f.readlines()))
    _Check1()

    def _Check2():
      """Verify mtime was set for objects at destination."""
      self.assertEquals(long(os.path.getmtime(os.path.join(tmpdir, 'obj1'))), 5)
      self.assertEquals(long(os.path.getmtime(os.path.join(tmpdir, '.obj2'))),
                        5)
      self.assertEquals(long(os.path.getmtime(os.path.join(tmpdir, 'obj6'))),
                        50)
      self.assertEquals(long(os.path.getmtime(os.path.join(tmpdir, 'obj8'))),
                        100)
      self.assertEquals(long(os.path.getmtime(os.path.join(tmpdir, 'obj9'))),
                        25)
    _Check2()

    # Now rerun the rsync with the -c option.
    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check3():
      """Tests rsync -c works as expected."""
      self.RunGsUtil(['rsync', '-r', '-d', '-c', suri(bucket_uri), tmpdir])
      listing1 = TailSet(suri(bucket_uri), self.FlatListBucket(bucket_uri))
      listing2 = TailSet(tmpdir, self.FlatListDir(tmpdir))
      # Bucket should have un-altered content.
      self.assertEquals(listing1, set(['/obj1', '/.obj2', '/subdir/obj3',
                                       '/obj4', '/obj6', '/obj7', '/obj8',
                                       '/obj9', '/obj10', '/obj11']))
      # Dir should have content like bucket this time with subdirectories
      # synced.
      self.assertEquals(listing2, set(['/obj1', '/.obj2', '/subdir/obj3',
                                       '/obj4', '/obj6', '/obj7', '/obj8',
                                       '/obj9', '/obj10', '/obj11']))
      # Assert that the contents of obj7 have now changed because the -c flag
      # was used to force checksums.
      self.assertEquals('obj7', self.RunGsUtil(
          ['cat', suri(bucket_uri, 'obj7')], return_stdout=True))
      self._VerifyObjectMtime(bucket_uri.bucket_name, 'obj7', '5')
      # Check the mtime of obj7 in the destination to see that it changed.
      self.assertEquals(long(os.path.getmtime(os.path.join(tmpdir, 'obj7'))), 5)
      # Verify obj9 and obj10 content has changed because hashes were used in
      # comparisons.
      with open(os.path.join(tmpdir, 'obj9')) as f:
        self.assertEquals('obj9', '\n'.join(f.readlines()))
      with open(os.path.join(tmpdir, 'obj10')) as f:
        self.assertEquals('obj10', '\n'.join(f.readlines()))
    _Check3()

  @unittest.skipIf(IS_WINDOWS, 'POSIX attributes not available on Windows.')
  def test_bucket_to_dir_preserve_posix_errors(self):
    """Tests that rsync -P works properly with files that would be orphaned."""
    bucket_uri = self.CreateBucket()
    tmpdir = self.CreateTempDir()
    subdir = os.path.join(tmpdir, 'subdir')
    os.mkdir(subdir)
    # Create an object with an invalid mode. Must also specify a UID.
    obj1 = self.CreateObject(bucket_uri=bucket_uri, object_name='obj1',
                             contents='obj1', mode='222', uid=os.getuid())
    obj2 = self.CreateObject(bucket_uri=bucket_uri, object_name='.obj2',
                             contents='.obj2', gid=INVALID_GID(), mode='540')
    self.CreateObject(bucket_uri=bucket_uri, object_name='subdir/obj3',
                      contents='subdir/obj3')
    obj6 = self.CreateObject(bucket_uri=bucket_uri, object_name='obj6',
                             contents='obj6', gid=INVALID_GID(), mode='440')
    obj7 = self.CreateObject(bucket_uri=bucket_uri, object_name='obj7',
                             contents='obj7', gid=NON_PRIMARY_GID(), mode='333')
    obj8 = self.CreateObject(bucket_uri=bucket_uri, object_name='obj8',
                             contents='obj8', uid=INVALID_UID())
    obj9 = self.CreateObject(bucket_uri=bucket_uri, object_name='obj9',
                             contents='obj9', uid=INVALID_UID(), mode='777')
    obj10 = self.CreateObject(bucket_uri=bucket_uri, object_name='obj10',
                              contents='obj10', gid=INVALID_GID(),
                              uid=INVALID_UID())
    obj11 = self.CreateObject(bucket_uri=bucket_uri, object_name='obj11',
                              contents='obj11', gid=INVALID_GID(),
                              uid=INVALID_UID(), mode='544')
    obj12 = self.CreateObject(bucket_uri=bucket_uri, object_name='obj12',
                              contents='obj12', uid=INVALID_UID(), gid=USER_ID)
    obj13 = self.CreateObject(bucket_uri=bucket_uri, object_name='obj13',
                              contents='obj13', uid=INVALID_UID(),
                              gid=PRIMARY_GID, mode='644')
    obj14 = self.CreateObject(bucket_uri=bucket_uri, object_name='obj14',
                              contents='obj14', uid=USER_ID, gid=INVALID_GID())
    obj15 = self.CreateObject(bucket_uri=bucket_uri, object_name='obj15',
                              contents='obj15', uid=USER_ID, gid=INVALID_GID(),
                              mode='655')
    obj16 = self.CreateObject(bucket_uri=bucket_uri, object_name='obj16',
                              contents='obj16', uid=USER_ID, mode='244')
    obj17 = self.CreateObject(bucket_uri=bucket_uri, object_name='obj17',
                              contents='obj17', uid=USER_ID, gid=PRIMARY_GID,
                              mode='222')
    obj18 = self.CreateObject(bucket_uri=bucket_uri, object_name='obj18',
                              contents='obj18', uid=USER_ID,
                              gid=NON_PRIMARY_GID(), mode='333')
    obj19 = self.CreateObject(bucket_uri=bucket_uri, object_name='obj19',
                              contents='obj19', mode='222')
    self.CreateTempFile(tmpdir=tmpdir, file_name='.obj2', contents='.OBJ2')
    self.CreateTempFile(tmpdir=tmpdir, file_name='obj4', contents='obj4')
    self.CreateTempFile(tmpdir=subdir, file_name='obj5', contents='subdir/obj5')

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check1():
      """Tests that an exception is thrown because files will be orphaned."""
      stderr = self.RunGsUtil(['rsync', '-P', '-r', suri(bucket_uri), tmpdir],
                              expected_status=1, return_stderr=True)
      self.assertIn(ORPHANED_FILE, stderr)
      self.assertTrue(BuildErrorRegex(obj1, POSIX_MODE_ERROR).search(stderr))
      self.assertTrue(BuildErrorRegex(obj2, POSIX_GID_ERROR).search(stderr))
      self.assertTrue(BuildErrorRegex(obj6, POSIX_GID_ERROR).search(stderr))
      self.assertTrue(BuildErrorRegex(obj7, POSIX_MODE_ERROR).search(stderr))
      self.assertTrue(BuildErrorRegex(obj8, POSIX_UID_ERROR).search(stderr))
      self.assertTrue(BuildErrorRegex(obj9, POSIX_UID_ERROR).search(stderr))
      self.assertTrue(BuildErrorRegex(obj10, POSIX_UID_ERROR).search(stderr))
      self.assertTrue(BuildErrorRegex(obj11, POSIX_UID_ERROR).search(stderr))
      self.assertTrue(BuildErrorRegex(obj12, POSIX_UID_ERROR).search(stderr))
      self.assertTrue(BuildErrorRegex(obj13, POSIX_UID_ERROR).search(stderr))
      self.assertTrue(BuildErrorRegex(obj14, POSIX_GID_ERROR).search(stderr))
      self.assertTrue(BuildErrorRegex(obj15, POSIX_GID_ERROR).search(stderr))
      self.assertTrue(BuildErrorRegex(obj16, POSIX_INSUFFICIENT_ACCESS_ERROR)
                      .search(stderr))
      self.assertTrue(BuildErrorRegex(obj17, POSIX_MODE_ERROR).search(stderr))
      self.assertTrue(BuildErrorRegex(obj18, POSIX_MODE_ERROR).search(stderr))
      self.assertTrue(BuildErrorRegex(obj19, POSIX_MODE_ERROR).search(stderr))
      listing1 = TailSet(suri(bucket_uri), self.FlatListBucket(bucket_uri))
      listing2 = TailSet(tmpdir, self.FlatListDir(tmpdir))
      # Bucket should have un-altered content.
      self.assertEquals(listing1, set(['/obj1', '/.obj2', '/subdir/obj3',
                                       '/obj6', '/obj7', '/obj8', '/obj9',
                                       '/obj10', '/obj11', '/obj12', '/obj13',
                                       '/obj14', '/obj15', '/obj16', '/obj17',
                                       '/obj18', '/obj19']))
      # Dir should have un-altered content.
      self.assertEquals(listing2, set(['/.obj2', '/obj4', '/subdir/obj5']))
    _Check1()

    # Set a valid mode for another object.
    self._SetObjectCustomMetadataAttribute(self.default_provider,
                                           bucket_uri.bucket_name, '.obj2',
                                           MODE_ATTR, '640')

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check2():
      """Tests that a file with a valid mode in metadata, nothing changed."""
      # Test that even though a file now has a valid mode, no files were copied.
      stderr = self.RunGsUtil(['rsync', '-P', '-r', suri(bucket_uri), tmpdir],
                              expected_status=1, return_stderr=True)
      self.assertIn(ORPHANED_FILE, stderr)
      listing1 = TailSet(suri(bucket_uri), self.FlatListBucket(bucket_uri))
      listing2 = TailSet(tmpdir, self.FlatListDir(tmpdir))
      # Bucket should have un-altered content.
      self.assertEquals(listing1, set(['/obj1', '/.obj2', '/subdir/obj3',
                                       '/obj6', '/obj7', '/obj8', '/obj9',
                                       '/obj10', '/obj11', '/obj12', '/obj13',
                                       '/obj14', '/obj15', '/obj16', '/obj17',
                                       '/obj18', '/obj19']))
      # Dir should have un-altered content.
      self.assertEquals(listing2, set(['/.obj2', '/obj4', '/subdir/obj5']))
    _Check2()

  @SequentialAndParallelTransfer
  @unittest.skipIf(IS_WINDOWS, 'POSIX attributes not available on Windows.')
  @unittest.skipUnless(UsingCrcmodExtension(crcmod),
                       'Test requires fast crcmod.')
  def test_bucket_to_dir_preserve_posix_no_errors(self):
    """Tests that rsync -P works properly with default file attributes."""
    bucket_uri = self.CreateBucket()
    tmpdir = self.CreateTempDir()
    subdir = os.path.join(tmpdir, 'subdir')
    os.mkdir(subdir)
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj1',
                      contents='obj1', mode='444')
    self.CreateObject(bucket_uri=bucket_uri, object_name='.obj2',
                      contents='.obj2', gid=PRIMARY_GID)
    self.CreateObject(bucket_uri=bucket_uri, object_name='subdir/obj3',
                      contents='subdir/obj3', gid=NON_PRIMARY_GID())
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj6',
                      contents='obj6', gid=PRIMARY_GID, mode='555')
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj7',
                      contents='obj7', gid=NON_PRIMARY_GID(), mode='444')
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj8',
                      contents='obj8', uid=USER_ID)
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj9',
                      contents='obj9', uid=USER_ID, mode='422')
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj10',
                      contents='obj10', uid=USER_ID, gid=PRIMARY_GID)
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj11',
                      contents='obj11', uid=USER_ID, gid=NON_PRIMARY_GID())
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj12',
                      contents='obj12', uid=USER_ID, gid=PRIMARY_GID,
                      mode='400')
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj13',
                      contents='obj13', uid=USER_ID, gid=NON_PRIMARY_GID(),
                      mode='533')
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj14',
                      contents='obj14', uid=USER_ID, mode='444')
    self.CreateTempFile(tmpdir=tmpdir, file_name='.obj2', contents='.OBJ2')
    self.CreateTempFile(tmpdir=tmpdir, file_name='obj4', contents='obj4')
    self.CreateTempFile(tmpdir=subdir, file_name='obj5', contents='subdir/obj5')

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check1():
      """Verifies that all attributes were copied correctly when -P is used."""
      self.RunGsUtil(['rsync', '-P', '-r', suri(bucket_uri), tmpdir])
      listing1 = TailSet(suri(bucket_uri), self.FlatListBucket(bucket_uri))
      listing2 = TailSet(tmpdir, self.FlatListDir(tmpdir))
      # Bucket should have un-altered content.
      self.assertEquals(listing1, set(['/obj1', '/.obj2', '/subdir/obj3',
                                       '/obj6', '/obj7', '/obj8', '/obj9',
                                       '/obj10', '/obj11', '/obj12', '/obj13',
                                       '/obj14']))
      # Dir should have any new content from bucket.
      self.assertEquals(listing2, set(['/obj1', '/.obj2', '/subdir/obj3',
                                       '/obj4', '/subdir/obj5', '/obj6',
                                       '/obj7', '/obj8', '/obj9', '/obj10',
                                       '/obj11', '/obj12', '/obj13', '/obj14']))
    _Check1()

    self.VerifyLocalPOSIXPermissions(os.path.join(tmpdir, 'obj1'),
                                     uid=os.getuid(), mode=0o444)
    self.VerifyLocalPOSIXPermissions(os.path.join(tmpdir, '.obj2'),
                                     gid=PRIMARY_GID, uid=os.getuid(),
                                     mode=DEFAULT_MODE)
    self.VerifyLocalPOSIXPermissions(os.path.join(subdir, 'obj3'),
                                     gid=NON_PRIMARY_GID(), mode=DEFAULT_MODE)
    self.VerifyLocalPOSIXPermissions(os.path.join(tmpdir, 'obj6'),
                                     gid=PRIMARY_GID, mode=0o555)
    self.VerifyLocalPOSIXPermissions(os.path.join(tmpdir, 'obj7'),
                                     gid=NON_PRIMARY_GID(), mode=0o444)
    self.VerifyLocalPOSIXPermissions(os.path.join(tmpdir, 'obj8'),
                                     gid=PRIMARY_GID, mode=DEFAULT_MODE)
    self.VerifyLocalPOSIXPermissions(os.path.join(tmpdir, 'obj9'), uid=USER_ID,
                                     mode=0o422)
    self.VerifyLocalPOSIXPermissions(os.path.join(tmpdir, 'obj10'),
                                     uid=USER_ID, gid=PRIMARY_GID,
                                     mode=DEFAULT_MODE)
    self.VerifyLocalPOSIXPermissions(os.path.join(tmpdir, 'obj11'),
                                     uid=USER_ID, gid=NON_PRIMARY_GID(),
                                     mode=DEFAULT_MODE)
    self.VerifyLocalPOSIXPermissions(os.path.join(tmpdir, 'obj12'),
                                     uid=USER_ID, gid=PRIMARY_GID,
                                     mode=0o400)
    self.VerifyLocalPOSIXPermissions(os.path.join(tmpdir, 'obj13'),
                                     uid=USER_ID, gid=NON_PRIMARY_GID(),
                                     mode=0o533)
    self.VerifyLocalPOSIXPermissions(os.path.join(tmpdir, 'obj14'),
                                     uid=USER_ID, mode=0o444)

  @unittest.skipUnless(UsingCrcmodExtension(crcmod),
                       'Test requires fast crcmod.')
  def test_bucket_to_dir_minus_d(self):
    """Tests that flat and recursive rsync bucket to dir works correctly."""
    # Create bucket and dir with 1 overlapping object, 1 extra object at root
    # level in each, and 1 extra object 1 level down in each, where one of the
    # objects starts with "." to test that we don't skip those objects. Make the
    # overlapping objects named the same but with different content, to test
    # that we detect and properly copy in that case.
    bucket_uri = self.CreateBucket()
    tmpdir = self.CreateTempDir()
    subdir = os.path.join(tmpdir, 'subdir')
    os.mkdir(subdir)
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj1',
                      contents='obj1')
    # Set the mtime for obj2 because obj2 in the cloud and obj2 on the local
    # file system have the potential to be created during the same second.
    self.CreateObject(bucket_uri=bucket_uri, object_name='.obj2',
                      contents='.obj2', mtime=0)
    self.CreateObject(bucket_uri=bucket_uri, object_name='subdir/obj3',
                      contents='subdir/obj3')
    self.CreateTempFile(tmpdir=tmpdir, file_name='.obj2', contents='.OBJ2')
    self.CreateTempFile(tmpdir=tmpdir, file_name='obj4', contents='obj4')
    self.CreateTempFile(tmpdir=subdir, file_name='obj5', contents='subdir/obj5')

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check1():
      """Tests rsync works as expected."""
      self.RunGsUtil(['rsync', '-d', suri(bucket_uri), tmpdir])
      listing1 = TailSet(suri(bucket_uri), self.FlatListBucket(bucket_uri))
      listing2 = TailSet(tmpdir, self.FlatListDir(tmpdir))
      # Bucket should have un-altered content.
      self.assertEquals(listing1, set(['/obj1', '/.obj2', '/subdir/obj3']))
      # Dir should have content like bucket but without the subdir objects
      # synchronized.
      self.assertEquals(listing2, set(['/obj1', '/.obj2', '/subdir/obj5']))
      # Assert that the src/dest objects that had same length but different
      # content were synchronized (bucket to dir rsync uses checksums as a
      # backup to mtime unless you specify -c).
      self.assertEquals('.obj2', self.RunGsUtil(
          ['cat', suri(bucket_uri, '.obj2')], return_stdout=True))
      with open(os.path.join(tmpdir, '.obj2')) as f:
        self.assertEquals('.obj2', '\n'.join(f.readlines()))
    _Check1()

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check2():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-d', suri(bucket_uri), tmpdir], return_stderr=True))
    _Check2()

    # Now rerun the rsync with the -c option.
    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check3():
      """Tests rsync -c works as expected."""
      self.RunGsUtil(['rsync', '-d', '-c', suri(bucket_uri), tmpdir])
      listing1 = TailSet(suri(bucket_uri), self.FlatListBucket(bucket_uri))
      listing2 = TailSet(tmpdir, self.FlatListDir(tmpdir))
      # Bucket should have un-altered content.
      self.assertEquals(listing1, set(['/obj1', '/.obj2', '/subdir/obj3']))
      # Dir should have content like bucket but without the subdir objects
      # synchronized.
      self.assertEquals(listing2, set(['/obj1', '/.obj2', '/subdir/obj5']))
      # Assert that the src/dest objects that had same length but different
      # content were synchronized (bucket to dir rsync with -c uses checksums).
      self.assertEquals('.obj2', self.RunGsUtil(
          ['cat', suri(bucket_uri, '.obj2')], return_stdout=True))
      with open(os.path.join(tmpdir, '.obj2')) as f:
        self.assertEquals('.obj2', '\n'.join(f.readlines()))
    _Check3()

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check4():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-d', '-c', suri(bucket_uri), tmpdir], return_stderr=True))
    _Check4()

    # Now add and remove some objects in dir and bucket and test rsync -r.
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj6',
                      contents='obj6')
    self.CreateTempFile(tmpdir=tmpdir, file_name='obj7', contents='obj7')
    self.RunGsUtil(['rm', suri(bucket_uri, 'obj1')])
    os.unlink(os.path.join(tmpdir, '.obj2'))

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check5():
      self.RunGsUtil(['rsync', '-d', '-r', suri(bucket_uri), tmpdir])
      listing1 = TailSet(suri(bucket_uri), self.FlatListBucket(bucket_uri))
      listing2 = TailSet(tmpdir, self.FlatListDir(tmpdir))
      # Bucket should have un-altered content.
      self.assertEquals(listing1, set(['/.obj2', '/obj6', '/subdir/obj3']))
      # Dir should have content like bucket but without the subdir objects
      # synchronized.
      self.assertEquals(listing2, set(['/.obj2', '/obj6', '/subdir/obj3']))
    _Check5()

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check6():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-d', '-r', suri(bucket_uri), tmpdir], return_stderr=True))
    _Check6()

  @unittest.skipUnless(UsingCrcmodExtension(crcmod),
                       'Test requires fast crcmod.')
  def test_bucket_to_dir_minus_d_with_fname_case_change(self):
    """Tests that name case changes work correctly.

    Example:

    Windows filenames are case-preserving in what you wrote, but case-
    insensitive when compared. If you synchronize from FS to cloud and then
    change case-naming in local files, you could end up with this situation:

    Cloud copy is called .../TiVo/...
    FS copy is called      .../Tivo/...

    Then, if you rsync from cloud to FS, if rsync doesn't recognize that on
    Windows these names are identical, each rsync run will cause both a copy
    and a delete to be executed.
    """
    # Create bucket and dir with same objects, but dir copy has different name
    # case.
    bucket_uri = self.CreateBucket()
    tmpdir = self.CreateTempDir()
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj1',
                      contents='obj1')
    self.CreateTempFile(tmpdir=tmpdir, file_name='Obj1', contents='obj1')

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check1():
      """Tests rsync works as expected."""
      output = self.RunGsUtil(
          ['rsync', '-d', '-r', suri(bucket_uri), tmpdir], return_stderr=True)
      # Nothing should be copied or removed under Windows.
      if IS_WINDOWS:
        self.assertEquals(NO_CHANGES, output)
      else:
        self.assertNotEquals(NO_CHANGES, output)
    _Check1()

  @unittest.skipUnless(UsingCrcmodExtension(crcmod),
                       'Test requires fast crcmod.')
  def test_bucket_to_dir_minus_d_with_leftover_dir_placeholder(self):
    """Tests that we correctly handle leftover dir placeholders.

    See comments in gslib.commands.rsync._FieldedListingIterator for details.
    """
    bucket_uri = self.CreateBucket()
    tmpdir = self.CreateTempDir()
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj1',
                      contents='obj1')
    # Create a placeholder like what can be left over by web GUI tools.
    key_uri = bucket_uri.clone_replace_name('/')
    key_uri.set_contents_from_string('')

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check1():
      """Tests rsync works as expected."""
      self.RunGsUtil(
          ['rsync', '-d', '-r', suri(bucket_uri), tmpdir], return_stderr=True)
      listing1 = TailSet(suri(bucket_uri), self.FlatListBucket(bucket_uri))
      listing2 = TailSet(tmpdir, self.FlatListDir(tmpdir))
      # Bucket should have un-altered content.
      self.assertEquals(listing1, set(['/obj1', '//']))
      # Bucket should not have the placeholder object.
      self.assertEquals(listing2, set(['/obj1']))
    _Check1()

  @unittest.skipIf(IS_WINDOWS, 'os.symlink() is not available on Windows.')
  def test_rsync_minus_r_minus_e(self):
    """Tests that rsync -e -r ignores symlinks when recursing."""
    bucket_uri = self.CreateBucket()
    tmpdir = self.CreateTempDir()
    subdir = os.path.join(tmpdir, 'subdir')
    os.mkdir(subdir)
    os.mkdir(os.path.join(tmpdir, 'missing'))
    # Create a blank directory that is a broken symlink to ensure that we
    # don't fail recursive enumeration with a bad symlink.
    os.symlink(os.path.join(tmpdir, 'missing'),
               os.path.join(subdir, 'missing'))
    os.rmdir(os.path.join(tmpdir, 'missing'))
    self.RunGsUtil(['rsync', '-r', '-e', tmpdir, suri(bucket_uri)])

  @unittest.skipIf(IS_WINDOWS, 'os.symlink() is not available on Windows.')
  def test_rsync_minus_d_minus_e(self):
    """Tests that rsync -e ignores symlinks."""
    tmpdir = self.CreateTempDir()
    subdir = os.path.join(tmpdir, 'subdir')
    os.mkdir(subdir)
    bucket_uri = self.CreateBucket()
    fpath1 = self.CreateTempFile(
        tmpdir=tmpdir, file_name='obj1', contents='obj1')
    self.CreateTempFile(tmpdir=tmpdir, file_name='.obj2', contents='.obj2')
    self.CreateTempFile(tmpdir=subdir, file_name='obj3', contents='subdir/obj3')
    good_symlink_path = os.path.join(tmpdir, 'symlink1')
    os.symlink(fpath1, good_symlink_path)
    # Make a symlink that points to a non-existent path to test that -e also
    # handles that case.
    bad_symlink_path = os.path.join(tmpdir, 'symlink2')
    os.symlink(os.path.join('/', 'non-existent'), bad_symlink_path)
    self.CreateObject(bucket_uri=bucket_uri, object_name='.obj2',
                      contents='.OBJ2')
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj4',
                      contents='obj4')
    self.CreateObject(bucket_uri=bucket_uri, object_name='subdir/obj5',
                      contents='subdir/obj5')

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check1():
      """Ensure listings match the commented expectations."""
      self.RunGsUtil(['rsync', '-d', '-e', tmpdir, suri(bucket_uri)])
      listing1 = TailSet(tmpdir, self.FlatListDir(tmpdir))
      listing2 = TailSet(suri(bucket_uri), self.FlatListBucket(bucket_uri))
      # Dir should have un-altered content.
      self.assertEquals(
          listing1,
          set(['/obj1', '/.obj2', '/subdir/obj3', '/symlink1', '/symlink2']))
      # Bucket should have content like dir but without the symlink, and
      # without subdir objects synchronized.
      self.assertEquals(listing2, set(['/obj1', '/.obj2', '/subdir/obj5']))
    _Check1()

    # Now remove invalid symlink and run without -e, and see that symlink gets
    # copied (as file to which it points). Use @Retry as hedge against bucket
    # listing eventual consistency.
    os.unlink(bad_symlink_path)
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check2():
      """Tests rsync works as expected."""
      self.RunGsUtil(['rsync', '-d', tmpdir, suri(bucket_uri)])
      listing1 = TailSet(tmpdir, self.FlatListDir(tmpdir))
      listing2 = TailSet(suri(bucket_uri), self.FlatListBucket(bucket_uri))
      # Dir should have un-altered content.
      self.assertEquals(
          listing1, set(['/obj1', '/.obj2', '/subdir/obj3', '/symlink1']))
      # Bucket should have content like dir but without the symlink, and
      # without subdir objects synchronized.
      self.assertEquals(
          listing2, set(['/obj1', '/.obj2', '/subdir/obj5', '/symlink1']))
      self.assertEquals('obj1', self.RunGsUtil(
          ['cat', suri(bucket_uri, 'symlink1')], return_stdout=True))
    _Check2()

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check3():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-d', tmpdir, suri(bucket_uri)], return_stderr=True))
    _Check3()

  @SkipForS3('S3 does not support composite objects')
  def test_bucket_to_bucket_minus_d_with_composites(self):
    """Tests that rsync works with composite objects (which don't have MD5s)."""
    bucket1_uri = self.CreateBucket()
    bucket2_uri = self.CreateBucket()
    self.CreateObject(bucket_uri=bucket1_uri, object_name='obj1',
                      contents='obj1')
    self.CreateObject(bucket_uri=bucket1_uri, object_name='.obj2',
                      contents='.obj2')
    self.RunGsUtil(
        ['compose', suri(bucket1_uri, 'obj1'), suri(bucket1_uri, '.obj2'),
         suri(bucket1_uri, 'obj3')])
    self.CreateObject(bucket_uri=bucket2_uri, object_name='.obj2',
                      contents='.OBJ2')
    self.CreateObject(bucket_uri=bucket2_uri, object_name='obj4',
                      contents='obj4')

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check1():
      self.RunGsUtil(['rsync', '-d', suri(bucket1_uri), suri(bucket2_uri)])
      listing1 = TailSet(suri(bucket1_uri), self.FlatListBucket(bucket1_uri))
      listing2 = TailSet(suri(bucket2_uri), self.FlatListBucket(bucket2_uri))
      # First bucket should have un-altered content.
      self.assertEquals(listing1, set(['/obj1', '/.obj2', '/obj3']))
      # Second bucket should have content like first bucket but without the
      # subdir objects synchronized.
      self.assertEquals(listing2, set(['/obj1', '/.obj2', '/obj3']))
    _Check1()

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check2():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-d', suri(bucket1_uri), suri(bucket2_uri)],
          return_stderr=True))
    _Check2()

  def test_bucket_to_bucket_minus_d_empty_dest(self):
    """Tests working with empty dest bucket (iter runs out before src iter)."""
    bucket1_uri = self.CreateBucket()
    bucket2_uri = self.CreateBucket()
    self.CreateObject(bucket_uri=bucket1_uri, object_name='obj1',
                      contents='obj1')
    self.CreateObject(bucket_uri=bucket1_uri, object_name='.obj2',
                      contents='.obj2')

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check1():
      self.RunGsUtil(['rsync', '-d', suri(bucket1_uri), suri(bucket2_uri)])
      listing1 = TailSet(suri(bucket1_uri), self.FlatListBucket(bucket1_uri))
      listing2 = TailSet(suri(bucket2_uri), self.FlatListBucket(bucket2_uri))
      self.assertEquals(listing1, set(['/obj1', '/.obj2']))
      self.assertEquals(listing2, set(['/obj1', '/.obj2']))
    _Check1()

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check2():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-d', suri(bucket1_uri), suri(bucket2_uri)],
          return_stderr=True))
    _Check2()

  def test_bucket_to_bucket_minus_d_empty_src(self):
    """Tests working with empty src bucket (iter runs out before dst iter)."""
    bucket1_uri = self.CreateBucket()
    bucket2_uri = self.CreateBucket()
    self.CreateObject(bucket_uri=bucket2_uri, object_name='obj1',
                      contents='obj1')
    self.CreateObject(bucket_uri=bucket2_uri, object_name='.obj2',
                      contents='.obj2')

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check1():
      self.RunGsUtil(['rsync', '-d', suri(bucket1_uri), suri(bucket2_uri)])
      stderr = self.RunGsUtil(['ls', suri(bucket1_uri, '**')],
                              expected_status=1, return_stderr=True)
      self.assertIn('One or more URLs matched no objects', stderr)
      stderr = self.RunGsUtil(['ls', suri(bucket2_uri, '**')],
                              expected_status=1, return_stderr=True)
      self.assertIn('One or more URLs matched no objects', stderr)
    _Check1()

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check2():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-d', suri(bucket1_uri), suri(bucket2_uri)],
          return_stderr=True))
    _Check2()

  def test_rsync_minus_d_minus_p(self):
    """Tests that rsync -p preserves ACLs."""
    bucket1_uri = self.CreateBucket()
    bucket2_uri = self.CreateBucket()
    self.CreateObject(bucket_uri=bucket1_uri, object_name='obj1',
                      contents='obj1')
    # Set public-read (non-default) ACL so we can verify that rsync -p works.
    self.RunGsUtil(['acl', 'set', 'public-read', suri(bucket1_uri, 'obj1')])

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check1():
      """Tests rsync -p works as expected."""
      self.RunGsUtil(['rsync', '-d', '-p', suri(bucket1_uri),
                      suri(bucket2_uri)])
      listing1 = TailSet(suri(bucket1_uri), self.FlatListBucket(bucket1_uri))
      listing2 = TailSet(suri(bucket2_uri), self.FlatListBucket(bucket2_uri))
      self.assertEquals(listing1, set(['/obj1']))
      self.assertEquals(listing2, set(['/obj1']))
      acl1_json = self.RunGsUtil(['acl', 'get', suri(bucket1_uri, 'obj1')],
                                 return_stdout=True)
      acl2_json = self.RunGsUtil(['acl', 'get', suri(bucket2_uri, 'obj1')],
                                 return_stdout=True)
      self.assertEquals(acl1_json, acl2_json)
    _Check1()

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check2():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-d', '-p', suri(bucket1_uri), suri(bucket2_uri)],
          return_stderr=True))
    _Check2()

  def test_rsync_canned_acl(self):
    """Tests that rsync -a applies ACLs."""
    bucket1_uri = self.CreateBucket()
    bucket2_uri = self.CreateBucket()
    self.CreateObject(bucket_uri=bucket1_uri, object_name='obj1',
                      contents='obj1')

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check():
      """Tests rsync -a works as expected."""
      self.RunGsUtil(['rsync', '-d', '-a', 'public-read', suri(bucket1_uri),
                      suri(bucket2_uri)])
      listing1 = TailSet(suri(bucket1_uri), self.FlatListBucket(bucket1_uri))
      listing2 = TailSet(suri(bucket2_uri), self.FlatListBucket(bucket2_uri))
      self.assertEquals(listing1, set(['/obj1']))
      self.assertEquals(listing2, set(['/obj1']))
      # Set public-read on the original key after the rsync so we can compare
      # the ACLs.
      self.RunGsUtil(['acl', 'set', 'public-read', suri(bucket1_uri, 'obj1')])
      acl1_json = self.RunGsUtil(['acl', 'get', suri(bucket1_uri, 'obj1')],
                                 return_stdout=True)
      acl2_json = self.RunGsUtil(['acl', 'get', suri(bucket2_uri, 'obj1')],
                                 return_stdout=True)
      self.assertEquals(acl1_json, acl2_json)
    _Check()

  def test_rsync_to_nonexistent_bucket_subdir(self):
    """Tests that rsync to non-existent bucket subdir works."""
    # Create dir with some objects and empty bucket.
    tmpdir = self.CreateTempDir()
    subdir = os.path.join(tmpdir, 'subdir')
    os.mkdir(subdir)
    bucket_url = self.CreateBucket()
    self.CreateTempFile(tmpdir=tmpdir, file_name='obj1', contents='obj1')
    self.CreateTempFile(tmpdir=tmpdir, file_name='.obj2', contents='.obj2')
    self.CreateTempFile(tmpdir=subdir, file_name='obj3', contents='subdir/obj3')

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check1():
      """Tests rsync works as expected."""
      self.RunGsUtil(['rsync', '-r', tmpdir, suri(bucket_url, 'subdir')])
      listing1 = TailSet(tmpdir, self.FlatListDir(tmpdir))
      listing2 = TailSet(
          suri(bucket_url, 'subdir'),
          self.FlatListBucket(bucket_url.clone_replace_name('subdir')))
      # Dir should have un-altered content.
      self.assertEquals(listing1, set(['/obj1', '/.obj2', '/subdir/obj3']))
      # Bucket subdir should have content like dir.
      self.assertEquals(listing2, set(['/obj1', '/.obj2', '/subdir/obj3']))
    _Check1()

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check2():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-r', tmpdir, suri(bucket_url, 'subdir')],
          return_stderr=True))
    _Check2()

  def test_rsync_from_nonexistent_bucket(self):
    """Tests that rsync from a non-existent bucket subdir fails gracefully."""
    tmpdir = self.CreateTempDir()
    self.CreateTempFile(tmpdir=tmpdir, file_name='obj1', contents='obj1')
    self.CreateTempFile(tmpdir=tmpdir, file_name='.obj2', contents='.obj2')
    bucket_url_str = '%s://%s' % (
        self.default_provider, self.nonexistent_bucket_name)
    stderr = self.RunGsUtil(['rsync', '-d', bucket_url_str, tmpdir],
                            expected_status=1, return_stderr=True)
    self.assertIn('Caught non-retryable exception', stderr)
    listing = TailSet(tmpdir, self.FlatListDir(tmpdir))
    # Dir should have un-altered content.
    self.assertEquals(listing, set(['/obj1', '/.obj2']))

  def test_rsync_to_nonexistent_bucket(self):
    """Tests that rsync from a non-existent bucket subdir fails gracefully."""
    tmpdir = self.CreateTempDir()
    self.CreateTempFile(tmpdir=tmpdir, file_name='obj1', contents='obj1')
    self.CreateTempFile(tmpdir=tmpdir, file_name='.obj2', contents='.obj2')
    bucket_url_str = '%s://%s' % (
        self.default_provider, self.nonexistent_bucket_name)
    stderr = self.RunGsUtil(['rsync', '-d', bucket_url_str, tmpdir],
                            expected_status=1, return_stderr=True)
    self.assertIn('Caught non-retryable exception', stderr)
    listing = TailSet(tmpdir, self.FlatListDir(tmpdir))
    # Dir should have un-altered content.
    self.assertEquals(listing, set(['/obj1', '/.obj2']))

  def test_bucket_to_bucket_minus_d_with_overwrite_and_punc_chars(self):
    """Tests that punc chars in filenames don't confuse sort order."""
    bucket1_uri = self.CreateBucket()
    bucket2_uri = self.CreateBucket()
    # Create 2 objects in each bucket, with one overwritten with a name that's
    # less than the next name in destination bucket when encoded, but not when
    # compared without encoding.
    self.CreateObject(bucket_uri=bucket1_uri, object_name='e/obj1',
                      contents='obj1')
    self.CreateObject(bucket_uri=bucket1_uri, object_name='e-1/.obj2',
                      contents='.obj2')
    self.CreateObject(bucket_uri=bucket2_uri, object_name='e/obj1',
                      contents='OBJ1')
    self.CreateObject(bucket_uri=bucket2_uri, object_name='e-1/.obj2',
                      contents='.obj2')
    # Need to make sure the bucket listings are caught-up, otherwise the
    # rsync may not see all objects and fail to synchronize correctly.
    self.AssertNObjectsInBucket(bucket1_uri, 2)
    self.AssertNObjectsInBucket(bucket2_uri, 2)

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check1():
      """Tests rsync works as expected."""
      self.RunGsUtil(['rsync', '-rd', suri(bucket1_uri), suri(bucket2_uri)])
      listing1 = TailSet(suri(bucket1_uri), self.FlatListBucket(bucket1_uri))
      listing2 = TailSet(suri(bucket2_uri), self.FlatListBucket(bucket2_uri))
      # First bucket should have un-altered content.
      self.assertEquals(listing1, set(['/e/obj1', '/e-1/.obj2']))
      self.assertEquals(listing2, set(['/e/obj1', '/e-1/.obj2']))
      # Assert correct contents.
      self.assertEquals('obj1', self.RunGsUtil(
          ['cat', suri(bucket2_uri, 'e/obj1')], return_stdout=True))
      self.assertEquals('.obj2', self.RunGsUtil(
          ['cat', suri(bucket2_uri, 'e-1/.obj2')], return_stdout=True))
    _Check1()

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check2():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-d', suri(bucket1_uri), suri(bucket2_uri)],
          return_stderr=True))
    _Check2()

  def test_dir_to_bucket_minus_x(self):
    """Tests that rsync -x option works correctly."""
    # Create dir and bucket with 1 overlapping and 2 extra objects in each.
    tmpdir = self.CreateTempDir()
    bucket_uri = self.CreateBucket()
    self.CreateTempFile(tmpdir=tmpdir, file_name='obj1', contents='obj1')
    self.CreateTempFile(tmpdir=tmpdir, file_name='.obj2', contents='.obj2')
    self.CreateTempFile(tmpdir=tmpdir, file_name='obj3', contents='obj3')
    self.CreateObject(bucket_uri=bucket_uri, object_name='.obj2',
                      contents='.obj2')
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj4',
                      contents='obj4')
    self.CreateObject(bucket_uri=bucket_uri, object_name='obj5',
                      contents='obj5')

    # Need to make sure the bucket listing is caught-up, otherwise the
    # first rsync may not see .obj2 and overwrite it.
    self.AssertNObjectsInBucket(bucket_uri, 3)

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check1():
      """Tests rsync works as expected."""
      self.RunGsUtil(['rsync', '-d', '-x', 'obj[34]', tmpdir, suri(bucket_uri)])
      listing1 = TailSet(tmpdir, self.FlatListDir(tmpdir))
      listing2 = TailSet(suri(bucket_uri), self.FlatListBucket(bucket_uri))
      # Dir should have un-altered content.
      self.assertEquals(listing1, set(['/obj1', '/.obj2', '/obj3']))
      # Bucket should have content like dir but ignoring obj3 from dir and not
      # deleting obj4 from bucket (per exclude regex).
      self.assertEquals(listing2, set(['/obj1', '/.obj2', '/obj4']))
    _Check1()

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check2():
      # Check that re-running the same rsync command causes no more changes.
      self.assertEquals(NO_CHANGES, self.RunGsUtil(
          ['rsync', '-d', '-x', 'obj[34]', tmpdir, suri(bucket_uri)],
          return_stderr=True))
    _Check2()

  @unittest.skipIf(IS_WINDOWS,
                   "os.chmod() won't make file unreadable on Windows.")
  def test_dir_to_bucket_minus_C(self):
    """Tests that rsync -C option works correctly."""
    # Create dir with 3 objects, the middle of which is unreadable.
    tmpdir = self.CreateTempDir()
    bucket_uri = self.CreateBucket()
    self.CreateTempFile(tmpdir=tmpdir, file_name='obj1', contents='obj1')
    path = self.CreateTempFile(tmpdir=tmpdir, file_name='obj2', contents='obj2')
    os.chmod(path, 0)
    self.CreateTempFile(tmpdir=tmpdir, file_name='obj3', contents='obj3')

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check():
      """Tests rsync works as expected."""
      stderr = self.RunGsUtil(['rsync', '-C', tmpdir, suri(bucket_uri)],
                              expected_status=1, return_stderr=True)
      self.assertIn('1 files/objects could not be copied/removed.', stderr)
      listing1 = TailSet(tmpdir, self.FlatListDir(tmpdir))
      listing2 = TailSet(suri(bucket_uri), self.FlatListBucket(bucket_uri))
      # Dir should have un-altered content.
      self.assertEquals(listing1, set(['/obj1', '/obj2', '/obj3']))
      # Bucket should have obj1 and obj3 even though obj2 was unreadable.
      self.assertEquals(listing2, set(['/obj1', '/obj3']))
    _Check()

  @unittest.skipIf(IS_WINDOWS,
                   'Windows Unicode support is problematic in Python 2.x.')
  def test_dir_to_bucket_with_unicode_chars(self):
    """Tests that rsync -r works correctly with unicode filenames."""

    tmpdir = self.CreateTempDir()
    bucket_uri = self.CreateBucket()
    # The Ì character is unicode 00CC, but OSX translates this to the second
    # entry below.
    self.CreateTempFile(tmpdir=tmpdir, file_name=u'morales_suenÌƒos.jpg')
    # The Ì character is unicode 0049+0300; OSX uses this value in both cases.
    self.CreateTempFile(tmpdir=tmpdir, file_name=u'morales_suenÌƒos.jpg')
    self.CreateTempFile(tmpdir=tmpdir, file_name=u'fooꝾoo')

    expected_list_results = (
        frozenset(['/morales_suenÌƒos.jpg', '/fooꝾoo'])
        if IS_OSX else
        frozenset(['/morales_suenÌƒos.jpg', '/morales_suenÌƒos.jpg',
                   '/fooꝾoo']))

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check():
      """Tests rsync works as expected."""
      self.RunGsUtil(['rsync', '-r', tmpdir, suri(bucket_uri)])
      listing1 = TailSet(tmpdir, self.FlatListDir(tmpdir))
      listing2 = TailSet(suri(bucket_uri), self.FlatListBucket(bucket_uri))
      self.assertEquals(listing1, expected_list_results)
      self.assertEquals(listing2, expected_list_results)
    _Check()

  @SkipForS3('No compressed transport encoding support for S3.')
  @SkipForXML('No compressed transport encoding support for the XML API.')
  @SequentialAndParallelTransfer
  def test_gzip_transport_encoded_all_upload(self):
    """Test gzip encoded files upload correctly."""
    # Setup the bucket and local data.
    file_names = ('test', 'test.txt', 'test.xml')
    local_uris = []
    bucket_uri = self.CreateBucket()
    tmpdir = self.CreateTempDir()
    contents = 'x' * 10000
    # Create local files.
    for file_name in file_names:
      local_uris.append(self.CreateTempFile(tmpdir, contents, file_name))
    # Upload the data.
    stderr = self.RunGsUtil(
        ['-D', 'rsync', '-J', '-r', tmpdir, suri(bucket_uri)],
        return_stderr=True)
    self.AssertNObjectsInBucket(bucket_uri, len(local_uris))
    # Ensure the correct files were marked for compression.
    for local_uri in local_uris:
      self.assertIn(
          'Using compressed transport encoding for file://%s.' % (local_uri),
          stderr)
    # Ensure the progress logger sees a gzip encoding.
    self.assertIn('send: Using gzip transport encoding for the request.',
                  stderr)

  @SkipForS3('No compressed transport encoding support for S3.')
  @SkipForXML('No compressed transport encoding support for the XML API.')
  @SequentialAndParallelTransfer
  def test_gzip_transport_encoded_filtered_upload(self):
    """Test gzip encoded files upload correctly."""
    # Setup the bucket and local data.
    file_names_valid = ('test.txt', 'photo.txt')
    file_names_invalid = ('file', 'test.png', 'test.xml')
    local_uris_valid = []
    local_uris_invalid = []
    bucket_uri = self.CreateBucket()
    tmpdir = self.CreateTempDir()
    contents = 'x' * 10000
    # Create local files.
    for file_name in file_names_valid:
      local_uris_valid.append(self.CreateTempFile(tmpdir, contents, file_name))
    for file_name in file_names_invalid:
      local_uris_invalid.append(
          self.CreateTempFile(tmpdir, contents, file_name))
    # Upload the data.
    stderr = self.RunGsUtil(
        ['-D', 'rsync', '-j', 'txt', '-r', tmpdir, suri(bucket_uri)],
        return_stderr=True)
    self.AssertNObjectsInBucket(
        bucket_uri, len(file_names_valid) + len(file_names_invalid))
    # Ensure the correct files were marked for compression.
    for local_uri in local_uris_valid:
      self.assertIn(
          'Using compressed transport encoding for file://%s.' % (local_uri),
          stderr)
    for local_uri in local_uris_invalid:
      self.assertNotIn(
          'Using compressed transport encoding for file://%s.' % (local_uri),
          stderr)
    # Ensure the progress logger sees a gzip encoding.
    self.assertIn('send: Using gzip transport encoding for the request.',
                  stderr)

  @SkipForS3('No compressed transport encoding support for S3.')
  @SkipForXML('No compressed transport encoding support for the XML API.')
  @SequentialAndParallelTransfer
  def test_gzip_transport_encoded_all_upload_parallel(self):
    """Test gzip encoded files upload correctly."""
    # Setup the bucket and local data.
    file_names = ('test', 'test.txt', 'test.xml')
    local_uris = []
    bucket_uri = self.CreateBucket()
    tmpdir = self.CreateTempDir()
    contents = 'x' * 10000
    for file_name in file_names:
      local_uris.append(self.CreateTempFile(tmpdir, contents, file_name))
    # Upload the data.
    stderr = self.RunGsUtil(
        ['-D', '-m', 'rsync', '-J', '-r', tmpdir, suri(bucket_uri)],
        return_stderr=True)
    self.AssertNObjectsInBucket(bucket_uri, len(local_uris))
    # Ensure the correct files were marked for compression.
    for local_uri in local_uris:
      self.assertIn(
          'Using compressed transport encoding for file://%s.' % (local_uri),
          stderr)
    # Ensure the progress logger sees a gzip encoding.
    self.assertIn('send: Using gzip transport encoding for the request.',
                  stderr)

  def authorize_project_to_use_testing_kms_key(
      self, key_name=testcase.KmsTestingResources.CONSTANT_KEY_NAME):
    # Make sure our keyRing and cryptoKey exist.
    keyring_fqn = self.kms_api.CreateKeyRing(
        PopulateProjectId(None), testcase.KmsTestingResources.KEYRING_NAME,
        location=testcase.KmsTestingResources.KEYRING_LOCATION)
    key_fqn = self.kms_api.CreateCryptoKey(keyring_fqn, key_name)
    # Make sure that the service account for our default project is authorized
    # to use our test KMS key.
    self.RunGsUtil(['kms', 'authorize', '-k', key_fqn])
    return key_fqn

  @SkipForS3('Test uses gs-specific KMS encryption')
  def test_kms_key_applied_to_dest_objects(self):
    bucket_uri = self.CreateBucket()
    cloud_container_suri = suri(bucket_uri) + '/foo'
    obj_name = 'bar'
    tmp_dir = self.CreateTempDir()
    self.CreateTempFile(tmpdir=tmp_dir, file_name=obj_name, contents=obj_name)
    key_fqn = self.authorize_project_to_use_testing_kms_key()

    # Rsync the object from our tmpdir to a GCS bucket, specifying a KMS key.
    with SetBotoConfigForTest([('GSUtil', 'encryption_key', key_fqn)]):
      self.RunGsUtil(['rsync', tmp_dir, cloud_container_suri])

    # Make sure the new object is encrypted with the specified KMS key.
    with SetBotoConfigForTest([('GSUtil', 'prefer_api', 'json')]):
      stdout = self.RunGsUtil(
          ['ls', '-L', '%s/%s' % (cloud_container_suri, obj_name)],
          return_stdout=True)
    self.assertRegexpMatches(stdout, r'KMS key:\s+%s' % key_fqn)

  @SkipForGS('Tests that gs-specific encryption settings are skipped for s3.')
  def test_kms_key_specified_will_not_prevent_non_kms_copy_to_s3(self):
    tmp_dir = self.CreateTempDir()
    self.CreateTempFile(tmpdir=tmp_dir, contents='foo')
    bucket_uri = self.CreateBucket()
    dummy_key = ('projects/myproject/locations/global/keyRings/mykeyring/'
                 'cryptoKeys/mykey')

    # Would throw an exception if the command failed because of invalid
    # formatting (i.e. specifying KMS key in a request to S3's API).
    with SetBotoConfigForTest([('GSUtil', 'prefer_api', 'json')]):
      self.RunGsUtil(['rsync', tmp_dir, suri(bucket_uri)])
