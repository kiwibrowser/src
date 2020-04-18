#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests of a local storage cache.

Tests a local disk cache wrapping access to storage with a interface like that
of gsd_storage.GSDStorage.
"""

import os
import unittest

import fake_storage
import file_tools
import local_storage_cache
import working_directory


class TestLocalStorageCache(unittest.TestCase):

  def CanBeReadBothWays(self, storage, key, out_file, expected):
    # Check that reading key with both GetData and GetFile yields expected.
    # out_file is used for GetFile output.
    self.assertEquals(expected, storage.GetData(key))
    url = storage.GetFile(key, out_file)
    self.assertNotEquals(None, url)
    self.assertEquals(expected, file_tools.ReadFile(out_file))

  def test_WriteRead(self):
    # Check that things written with PutData can be read back.
    with working_directory.TemporaryWorkingDirectory() as work_dir:
      mem_storage = fake_storage.FakeStorage()
      storage = local_storage_cache.LocalStorageCache(
          cache_path=os.path.join(work_dir, 'db'),
          storage=mem_storage)
      storage.PutData('bar', 'foo')
      self.CanBeReadBothWays(
          storage, 'foo', os.path.join(work_dir, 'out'), 'bar')

  def test_WriteFileRead(self):
    # Check that things written with PutFile can be read back.
    with working_directory.TemporaryWorkingDirectory() as work_dir:
      mem_storage = fake_storage.FakeStorage()
      storage = local_storage_cache.LocalStorageCache(
          cache_path=os.path.join(work_dir, 'db'),
          storage=mem_storage)
      bar = os.path.join(work_dir, 'bar_file')
      file_tools.WriteFile('bar', bar)
      storage.PutFile(bar, 'foo')
      self.CanBeReadBothWays(
          storage, 'foo', os.path.join(work_dir, 'out'), 'bar')

  def test_WriteOnlyToLocal(self):
    # Check that things written hit local storage, not the network.
    with working_directory.TemporaryWorkingDirectory() as work_dir:
      mem_storage = fake_storage.FakeStorage()
      storage = local_storage_cache.LocalStorageCache(
          cache_path=os.path.join(work_dir, 'db'),
          storage=mem_storage)
      storage.PutData('bar', 'foo')
      self.assertEquals(None, mem_storage.GetData('foo'))
      bar = os.path.join(work_dir, 'bar_file')
      file_tools.WriteFile('bar', bar)
      storage.PutFile(bar, 'foo')
      self.assertEquals(None, mem_storage.GetData('foo'))

  def test_Exists(self):
    # Checks that exists works properly.
    with working_directory.TemporaryWorkingDirectory() as work_dir:
      mem_storage = fake_storage.FakeStorage()
      storage = local_storage_cache.LocalStorageCache(
          cache_path=os.path.join(work_dir, 'db'),
          storage=mem_storage)
      storage.PutData('bar', 'foo')
      self.assertTrue(storage.Exists('foo'))
      self.assertFalse(storage.Exists('bad_foo'))

  def test_BadRead(self):
    # Check that reading from a non-existant key, fails.
    with working_directory.TemporaryWorkingDirectory() as work_dir:
      mem_storage = fake_storage.FakeStorage()
      storage = local_storage_cache.LocalStorageCache(
          cache_path=os.path.join(work_dir, 'db'),
          storage=mem_storage)
      self.assertEquals(None, storage.GetData('foo'))

  def test_HitWrappedStorage(self):
    # Check that if something isn't locally cached primary storage is hit.
    with working_directory.TemporaryWorkingDirectory() as work_dir:
      mem_storage = fake_storage.FakeStorage()
      storage = local_storage_cache.LocalStorageCache(
          cache_path=os.path.join(work_dir, 'db'),
          storage=mem_storage)
      mem_storage.PutData('hello', 'foo')
      self.assertEquals('hello', storage.GetData('foo'))

  def test_HitLocalFirst(self):
    # Check that reading hits local storage first.
    with working_directory.TemporaryWorkingDirectory() as work_dir:
      mem_storage = fake_storage.FakeStorage()
      storage = local_storage_cache.LocalStorageCache(
          cache_path=os.path.join(work_dir, 'db'),
          storage=mem_storage)
      storage.PutData('there', 'foo')
      mem_storage.PutData('hello', 'foo')
      self.assertEquals('there', storage.GetData('foo'))

  def test_AcceptSlashesAndDots(self):
    # Check that keys with slashes and dots are okay.
    with working_directory.TemporaryWorkingDirectory() as work_dir:
      mem_storage = fake_storage.FakeStorage()
      storage = local_storage_cache.LocalStorageCache(
          cache_path=os.path.join(work_dir, 'db'),
          storage=mem_storage)
      storage.PutData('hello', 'this/is/a/cool_test.txt')
      self.assertEquals('hello', storage.GetData('this/is/a/cool_test.txt'))

  def test_InvalidKey(self):
    # Check that an invalid key asserts.
    with working_directory.TemporaryWorkingDirectory() as work_dir:
      mem_storage = fake_storage.FakeStorage()
      storage = local_storage_cache.LocalStorageCache(
          cache_path=os.path.join(work_dir, 'db'),
          storage=mem_storage)
      bar = os.path.join(work_dir, 'bar_file')
      file_tools.WriteFile('bar', bar)
      self.assertRaises(KeyError, storage.PutData, 'bar', 'foo$')
      self.assertRaises(KeyError, storage.GetData, 'foo^')
      self.assertRaises(KeyError, storage.PutFile, bar, 'foo#')
      self.assertRaises(KeyError, storage.GetFile, 'foo!', 'bar')


if __name__ == '__main__':
  unittest.main()
