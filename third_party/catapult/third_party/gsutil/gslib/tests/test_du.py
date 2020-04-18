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
"""Tests for du command."""

from __future__ import absolute_import

import gslib.tests.testcase as testcase
from gslib.tests.testcase.integration_testcase import SkipForS3
from gslib.tests.util import GenerationFromURI as urigen
from gslib.tests.util import ObjectToURI as suri
from gslib.util import Retry


class TestDu(testcase.GsUtilIntegrationTestCase):
  """Integration tests for du command."""

  def _create_nested_subdir(self):
    """Creates a nested subdirectory for use by tests in this module."""
    bucket_uri = self.CreateBucket()
    obj_uris = []
    obj_uris.append(self.CreateObject(
        bucket_uri=bucket_uri, object_name='sub1材/five', contents='5five'))
    obj_uris.append(self.CreateObject(
        bucket_uri=bucket_uri, object_name='sub1材/four', contents='four'))
    obj_uris.append(self.CreateObject(
        bucket_uri=bucket_uri, object_name='sub1材/sub2/five', contents='5five'))
    obj_uris.append(self.CreateObject(
        bucket_uri=bucket_uri, object_name='sub1材/sub2/four', contents='four'))
    self.AssertNObjectsInBucket(bucket_uri, 4)
    return bucket_uri, obj_uris

  def test_object(self):
    obj_uri = self.CreateObject(contents='foo')
    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check():
      stdout = self.RunGsUtil(['du', suri(obj_uri)], return_stdout=True)
      self.assertEqual(stdout, '%-10s  %s\n' % (3, suri(obj_uri)))
    _Check()

  def test_bucket(self):
    bucket_uri = self.CreateBucket()
    obj_uri = self.CreateObject(bucket_uri=bucket_uri, contents='foo')
    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check():
      stdout = self.RunGsUtil(['du', suri(bucket_uri)], return_stdout=True)
      self.assertEqual(stdout, '%-10s  %s\n' % (3, suri(obj_uri)))
    _Check()

  def test_subdirs(self):
    """Tests that subdirectory sizes are correctly calculated and listed."""
    bucket_uri, obj_uris = self._create_nested_subdir()

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check():
      stdout = self.RunGsUtil(['du', suri(bucket_uri)], return_stdout=True)
      self.assertSetEqual(set(stdout.splitlines()), set([
          '%-10s  %s' % (5, suri(obj_uris[0])),
          '%-10s  %s' % (4, suri(obj_uris[1])),
          '%-10s  %s' % (5, suri(obj_uris[2])),
          '%-10s  %s' % (4, suri(obj_uris[3])),
          '%-10s  %s/sub1材/sub2/' % (9, suri(bucket_uri)),
          '%-10s  %s/sub1材/' % (18, suri(bucket_uri)),
      ]))
    _Check()

  def test_multi_args(self):
    """Tests running du with multiple command line arguments."""
    bucket_uri = self.CreateBucket()
    obj_uri1 = self.CreateObject(bucket_uri=bucket_uri, contents='foo')
    obj_uri2 = self.CreateObject(bucket_uri=bucket_uri, contents='foo2')
    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check():
      stdout = self.RunGsUtil(['du', suri(obj_uri1), suri(obj_uri2)],
                              return_stdout=True)
      self.assertSetEqual(set(stdout.splitlines()), set([
          '%-10s  %s' % (3, suri(obj_uri1)),
          '%-10s  %s' % (4, suri(obj_uri2)),
      ]))
    _Check()

  def test_total(self):
    """Tests total size listing via the -c flag."""
    bucket_uri = self.CreateBucket()
    obj_uri1 = self.CreateObject(bucket_uri=bucket_uri, contents='foo')
    obj_uri2 = self.CreateObject(bucket_uri=bucket_uri, contents='zebra')
    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check():
      stdout = self.RunGsUtil(['du', '-c', suri(bucket_uri)],
                              return_stdout=True)
      self.assertSetEqual(set(stdout.splitlines()), set([
          '%-10s  %s' % (3, suri(obj_uri1)),
          '%-10s  %s' % (5, suri(obj_uri2)),
          '%-10s  total' % 8,
      ]))
    _Check()

  def test_human_readable(self):
    obj_uri = self.CreateObject(contents='x' * 2048)
    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check():
      stdout = self.RunGsUtil(['du', '-h', suri(obj_uri)], return_stdout=True)
      self.assertEqual(stdout, '%-10s  %s\n' % ('2 KiB', suri(obj_uri)))
    _Check()

  def test_summary(self):
    """Tests summary listing with the -s flag."""
    bucket_uri1, _ = self._create_nested_subdir()
    bucket_uri2, _ = self._create_nested_subdir()

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check():
      stdout = self.RunGsUtil([
          'du', '-s', suri(bucket_uri1), suri(bucket_uri2)], return_stdout=True)
      self.assertSetEqual(set(stdout.splitlines()), set([
          '%-10s  %s' % (18, suri(bucket_uri1)),
          '%-10s  %s' % (18, suri(bucket_uri2)),
      ]))
    _Check()

  def test_subdir_summary(self):
    """Tests summary listing with the -s flag on a subdirectory."""
    bucket_uri1, _ = self._create_nested_subdir()
    bucket_uri2, _ = self._create_nested_subdir()
    subdir1 = suri(bucket_uri1, 'sub1材')
    subdir2 = suri(bucket_uri2, 'sub1材')

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check():
      stdout = self.RunGsUtil(
          ['du', '-s', subdir1, subdir2], return_stdout=True)
      self.assertSetEqual(set(stdout.splitlines()), set([
          '%-10s  %s' % (18, subdir1),
          '%-10s  %s' % (18, subdir2),
      ]))
    _Check()

  @SkipForS3('S3 lists versions in reverse order.')
  def test_versioned(self):
    """Tests listing all versions with the -a flag."""
    bucket_uri = self.CreateVersionedBucket()
    object_uri1 = self.CreateObject(
        bucket_uri=bucket_uri, object_name='foo', contents='foo')
    object_uri2 = self.CreateObject(
        bucket_uri=bucket_uri, object_name='foo', contents='foo2',
        gs_idempotent_generation=urigen(object_uri1))

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check1():
      stdout = self.RunGsUtil(['du', suri(bucket_uri)], return_stdout=True)
      self.assertEqual(stdout, '%-10s  %s\n' % (4, suri(object_uri2)))
    _Check1()

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check2():
      stdout = self.RunGsUtil(['du', '-a', suri(bucket_uri)],
                              return_stdout=True)
      self.assertSetEqual(set(stdout.splitlines()), set([
          '%-10s  %s#%s' % (
              3, suri(object_uri1), object_uri1.generation),
          '%-10s  %s#%s' % (
              4, suri(object_uri2), object_uri2.generation),
      ]))
    _Check2()

  def test_null_endings(self):
    """Tests outputting 0-endings with the -0 flag."""
    bucket_uri = self.CreateBucket()
    obj_uri1 = self.CreateObject(bucket_uri=bucket_uri, contents='foo')
    obj_uri2 = self.CreateObject(bucket_uri=bucket_uri, contents='zebra')
    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check():
      stdout = self.RunGsUtil(['du', '-0c', suri(bucket_uri)],
                              return_stdout=True)
      self.assertSetEqual(set(stdout.split('\0')), set([
          '%-10s  %s' % (3, suri(obj_uri1)),
          '%-10s  %s' % (5, suri(obj_uri2)),
          '%-10s  total' % 8,
          ''
      ]))
    _Check()

  def test_excludes(self):
    """Tests exclude pattern excluding certain file paths."""
    bucket_uri, obj_uris = self._create_nested_subdir()

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check():
      stdout = self.RunGsUtil([
          'du', '-e', '*sub2/five*', '-e', '*sub1材/four',
          suri(bucket_uri)], return_stdout=True)
      self.assertSetEqual(set(stdout.splitlines()), set([
          '%-10s  %s' % (5, suri(obj_uris[0])),
          '%-10s  %s' % (4, suri(obj_uris[3])),
          '%-10s  %s/sub1材/sub2/' % (4, suri(bucket_uri)),
          '%-10s  %s/sub1材/' % (9, suri(bucket_uri)),
      ]))
    _Check()

  def test_excludes_file(self):
    """Tests file exclusion with the -X flag."""
    bucket_uri, obj_uris = self._create_nested_subdir()
    fpath = self.CreateTempFile(contents='*sub2/five*\n*sub1材/four')

    # Use @Retry as hedge against bucket listing eventual consistency.
    @Retry(AssertionError, tries=3, timeout_secs=1)
    def _Check():
      stdout = self.RunGsUtil([
          'du', '-X', fpath, suri(bucket_uri)], return_stdout=True)
      self.assertSetEqual(set(stdout.splitlines()), set([
          '%-10s  %s' % (5, suri(obj_uris[0])),
          '%-10s  %s' % (4, suri(obj_uris[3])),
          '%-10s  %s/sub1材/sub2/' % (4, suri(bucket_uri)),
          '%-10s  %s/sub1材/' % (9, suri(bucket_uri)),
      ]))
    _Check()
