#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests of directory storage adapter."""

import os
import posixpath
import unittest

import directory_storage
import fake_storage
import gsd_storage
import hashing_tools
import hashing_tools_test
import working_directory


class TestDirectoryStorage(unittest.TestCase):

  def setUp(self):
    storage = fake_storage.FakeStorage()
    self._dir_storage = directory_storage.DirectoryStorageAdapter(storage)

  def test_WriteRead(self):
    # Check that a directory can be written and then read back.
    with working_directory.TemporaryWorkingDirectory() as work_dir:
      temp1 = os.path.join(work_dir, 'temp1')
      temp2 = os.path.join(work_dir, 'temp2')
      hashing_tools_test.GenerateTestTree('write_read', temp1)
      self._dir_storage.PutDirectory(temp1, 'foo')
      self._dir_storage.GetDirectory('foo', temp2)
      self.assertEqual(hashing_tools.StableHashPath(temp1),
                       hashing_tools.StableHashPath(temp2))

  def test_InputUntouched(self):
    # Check that PutDirectory doesn't alter its inputs.
    with working_directory.TemporaryWorkingDirectory() as work_dir:
      temp1 = os.path.join(work_dir, 'temp1')
      hashing_tools_test.GenerateTestTree('input_untouched', temp1)
      h1 = hashing_tools.StableHashPath(temp1)
      self._dir_storage.PutDirectory(temp1, 'hello')
      h2 = hashing_tools.StableHashPath(temp1)
      self.assertEqual(h1, h2)

  def test_URLsPropagate(self):
    # Check that consistent non-None URLs come from get and put.
    with working_directory.TemporaryWorkingDirectory() as work_dir:
      temp1 = os.path.join(work_dir, 'temp1')
      temp2 = os.path.join(work_dir, 'temp2')
      hashing_tools_test.GenerateTestTree('url_propagate', temp1)
      url1 = self._dir_storage.PutDirectory(temp1, 'me').url
      url2 = self._dir_storage.GetDirectory('me', temp2).url
      self.assertEqual(url1, url2)
      self.assertNotEqual(None, url1)

  def test_DirectoryName(self):
    with working_directory.TemporaryWorkingDirectory() as work_dir:
      temp1 = os.path.join(work_dir, 'temp1')
      temp2 = os.path.join(work_dir, 'temp2')
      hashing_tools_test.GenerateTestTree('directory_name', temp1)
      key_name = 'name_test'
      key_location = posixpath.join('name_parent', key_name)
      result1 = self._dir_storage.PutDirectory(temp1, key_location)
      result2 = self._dir_storage.GetDirectory(key_location, temp2)
      self.assertNotEqual(None, result1)
      self.assertNotEqual(None, result2)
      self.assertEqual(key_name, result1.name)
      self.assertEqual(key_name, result2.name)

  def test_HashStable(self):
    with working_directory.TemporaryWorkingDirectory() as work_dir:
      temp1 = os.path.join(work_dir, 'temp1')
      temp2 = os.path.join(work_dir, 'temp2')
      hashing_tools_test.GenerateTestTree('stable_hash', temp1)
      result1 = self._dir_storage.PutDirectory(temp1, 'hash_test')
      result2 = self._dir_storage.GetDirectory('hash_test', temp2)
      self.assertNotEqual(None, result1)
      self.assertNotEqual(None, result2)
      self.assertEqual(result1.hash, result2.hash)

  def test_CustomHasher(self):
    hash_value = 'test_hash_value'
    custom_hasher = lambda filename : hash_value
    with working_directory.TemporaryWorkingDirectory() as work_dir:
      temp = os.path.join(work_dir, 'temp')
      hashing_tools_test.GenerateTestTree('custom_hasher', temp)
      result = self._dir_storage.PutDirectory(
          temp,
          'custom_hasher',
          hasher=custom_hasher
      )
      self.assertNotEqual(None, result)
      self.assertEqual(result.hash, hash_value)

  def test_BadWrite(self):
    def call(cmd):
      return 1
    storage = gsd_storage.GSDStorage(
        gsutil=['mygsutil'],
        write_bucket='mybucket',
        read_buckets=[],
        call=call)
    dir_storage = directory_storage.DirectoryStorageAdapter(storage)
    # Check that storage exceptions come thru on failure.
    with working_directory.TemporaryWorkingDirectory() as work_dir:
      temp1 = os.path.join(work_dir, 'temp1')
      hashing_tools_test.GenerateTestTree('bad_write', temp1)
      self.assertRaises(gsd_storage.GSDStorageError,
                        dir_storage.PutDirectory, temp1, 'bad')

  def test_BadRead(self):
    # Check that storage exceptions come thru on failure.
    with working_directory.TemporaryWorkingDirectory() as work_dir:
      temp1 = os.path.join(work_dir, 'temp1')
      self.assertEqual(None, self._dir_storage.GetDirectory('foo', temp1))


if __name__ == '__main__':
  unittest.main()
