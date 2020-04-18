#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests of a Google Storage reader/writer."""

import os
import unittest

import file_tools
import gsd_storage


class TestGSDStorage(unittest.TestCase):

  def test_PutData(self):
    # Check that command line is as expected.
    # Special handling around the destination file,
    # as it's a temporary name and unknown to us.
    step = [0]
    def call(cmd):
      if step[0] == 0:
        self.assertEqual(
            ['mygsutil', 'cp', '-a', 'public-read'], cmd[0:4])
        self.assertEqual('foo', file_tools.ReadFile(cmd[4][len('file://'):]))
        self.assertEqual('gs://mybucket/bar', cmd[5].split('.')[0])
      elif step[0] == 1:
        self.assertEqual(
            ['mygsutil', 'cp', '-a', 'public-read'], cmd[0:4])
        self.assertEqual('gs://mybucket/bar', cmd[4].split('.')[0])
        self.assertEqual('gs://mybucket/bar', cmd[5])
      elif step[0] == 2:
        self.assertEqual(
            ['mygsutil', 'rm'], cmd[0:2])
        self.assertEqual('gs://mybucket/bar', cmd[2].split('.')[0])
      else:
        self.assertTrue(False)
      step[0] += 1
      return 0
    storage = gsd_storage.GSDStorage(
        write_bucket='mybucket',
        read_buckets=[],
        gsutil=['mygsutil'], call=call)
    url = storage.PutData('foo', 'bar')
    self.assertEquals('https://storage.googleapis.com/mybucket/bar', url)

  def test_PutFile(self):
    path = 'my/path'
    step = [0]
    def call(cmd):
      if step[0] == 0:
        self.assertEqual(
            ['mygsutil', 'cp', '-a', 'public-read'], cmd[0:4])
        self.assertEqual('gs://mybucket/bar', cmd[5].split('.')[0])
      elif step[0] == 1:
        self.assertEqual(
            ['mygsutil', 'cp', '-a', 'public-read'], cmd[0:4])
        self.assertEqual('gs://mybucket/bar', cmd[4].split('.')[0])
        self.assertEqual('gs://mybucket/bar', cmd[5])
      elif step[0] == 2:
        self.assertEqual(
            ['mygsutil', 'rm'], cmd[0:2])
        self.assertEqual('gs://mybucket/bar', cmd[2].split('.')[0])
      else:
        self.assertTrue(False)
      step[0] += 1
      return 0
    storage = gsd_storage.GSDStorage(
        write_bucket='mybucket',
        read_buckets=[],
        gsutil=['mygsutil'], call=call)
    url = storage.PutFile(path, 'bar')
    self.assertEquals('https://storage.googleapis.com/mybucket/bar', url)

  def test_PutFails(self):
    def call(cmd):
      return 1
    # Mock out running gsutil, have it fail, and check that it does.
    storage = gsd_storage.GSDStorage(
        gsutil=['mygsutil'],
        write_bucket='mybucket',
        read_buckets=[],
        call=call)
    self.assertRaises(gsd_storage.GSDStorageError,
                      storage.PutFile, 'foo', 'bar')
    self.assertRaises(gsd_storage.GSDStorageError,
                      storage.PutData, 'foo', 'bar')

  def test_PutNoBucket(self):
    # Check that we raise when writing an no bucket is provided.
    storage = gsd_storage.GSDStorage(
        gsutil=['mygsutil'],
        write_bucket=None,
        read_buckets=[])
    self.assertRaises(gsd_storage.GSDStorageError,
                      storage.PutFile, 'foo', 'bar')
    self.assertRaises(gsd_storage.GSDStorageError,
                      storage.PutData, 'foo', 'bar')

  def test_GetFile(self):
    path = 'my/path'
    def download(url, target):
      self.assertEqual(path, target)
      self.assertEqual('https://storage.googleapis.com/mybucket/bar', url)
    # Mock out download and confirm we download the expected URL.
    storage = gsd_storage.GSDStorage(
        gsutil=['mygsutil'],
        write_bucket='mybucket',
        read_buckets=['mybucket'],
        download=download)
    storage.GetFile('bar', path)

  def test_GetData(self):
    def download(url, target):
      self.assertEqual('https://storage.googleapis.com/mybucket/bar', url)
      file_tools.WriteFile('baz', target)
    # Mock out download and confirm we download the expected URL.
    storage = gsd_storage.GSDStorage(
        gsutil=['mygsutil'],
        write_bucket='mybucket',
        read_buckets=['mybucket'],
        download=download)
    self.assertEqual('baz', storage.GetData('bar'))

  def test_GetFails(self):
    def download(url, target):
      raise Exception('fail download %s to %s' % (url, target))
    # Make download raise and confirm this gets intercepted.
    storage = gsd_storage.GSDStorage(
        gsutil=['mygsutil'],
        write_bucket='mybucket',
        read_buckets=['mybucket'],
        download=download)
    self.assertFalse(storage.GetFile('foo', 'bar'))
    self.assertEquals(None, storage.GetData('foo'))

  def test_GetFallback(self):
    # Fail reading from the first bucket, and check fallback to the second.
    def download(url, target):
      if 'badbucket' in url:
        raise Exception('fail download %s to %s' % (url, target))
      else:
        file_tools.WriteFile('bar', target)
    storage = gsd_storage.GSDStorage(
        gsutil=['mygsutil'],
        write_bucket='mybucket',
        read_buckets=['badbucket', 'goodbucket'],
        download=download)
    self.assertEquals('bar', storage.GetData('foo'))

  def test_Exists(self):
    stored_keys = set()
    def call(cmd):
      self.assertTrue(len(cmd) >= 3)
      self.assertTrue(cmd[1] in ['cp', 'ls', 'rm'])
      if cmd[1] == 'cp':
        # Add the key into stored_keys
        copy_key = cmd[-1]
        stored_keys.add(copy_key)
        return 0
      elif cmd[1] == 'ls':
        query_key = cmd[-1]
        if query_key in stored_keys:
          return 0
        else:
          return 1
      elif cmd[1] == 'rm':
        return 0

    write_storage = gsd_storage.GSDStorage(
        gsutil=['mygsutil'],
        write_bucket='mybucket',
        read_buckets=[],
        call=call)

    read_storage = gsd_storage.GSDStorage(
        gsutil=['mygsutil'],
        write_bucket='',
        read_buckets=['mybucket'],
        call=call)

    self.assertNotEquals(None, write_storage.PutData('data', 'foo_key'))
    self.assertTrue(write_storage.Exists('foo_key'))
    self.assertFalse(write_storage.Exists('bad_key'))
    self.assertTrue(read_storage.Exists('foo_key'))
    self.assertFalse(read_storage.Exists('bad_key'))

if __name__ == '__main__':
  unittest.main()
