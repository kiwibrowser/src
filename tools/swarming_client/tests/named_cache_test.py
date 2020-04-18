#!/usr/bin/env python
# Copyright 2016 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

import logging
import os
import sys
import tempfile
import unittest

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(
    __file__.decode(sys.getfilesystemencoding()))))
sys.path.insert(0, ROOT_DIR)
sys.path.insert(0, os.path.join(ROOT_DIR, 'third_party'))

from depot_tools import fix_encoding
from utils import file_path
from utils import fs

import local_caching
import named_cache


def write_file(path, contents):
  with open(path, 'wb') as f:
    f.write(contents)


def read_file(path):
  with open(path, 'rb') as f:
    return f.read()


class CacheManagerTest(unittest.TestCase):
  def setUp(self):
    self.tempdir = tempfile.mkdtemp(prefix=u'named_cache_test')
    self.policies = local_caching.CachePolicies(
        max_cache_size=1024*1024*1024,
        min_free_space=1024,
        max_items=50,
        max_age_secs=21*24*60*60)
    self.manager = named_cache.CacheManager(self.tempdir, self.policies)

  def tearDown(self):
    try:
      file_path.rmtree(self.tempdir)
    finally:
      super(CacheManagerTest, self).tearDown()

  def make_caches(self, names):
    dest_dir = tempfile.mkdtemp(prefix=u'named_cache_test')
    try:
      names = map(unicode, names)
      for n in names:
        self.manager.install(os.path.join(dest_dir, n), n)
      self.assertEqual(set(names), set(os.listdir(dest_dir)))
      for n in names:
        self.manager.uninstall(os.path.join(dest_dir, n), n)
      self.assertEqual([], os.listdir(dest_dir))
      self.assertTrue(self.manager.available.issuperset(names))
    finally:
      file_path.rmtree(dest_dir)

  def test_get_oldest(self):
    with self.manager.open():
      self.assertIsNone(self.manager.get_oldest())
      self.make_caches(range(10))
      self.assertEqual(self.manager.get_oldest(), u'0')

  def test_get_timestamp(self):
    now = 0
    time_fn = lambda: now
    with self.manager.open(time_fn=time_fn):
      for i in xrange(10):
        self.make_caches([i])
        now += 1
      for i in xrange(10):
        self.assertEqual(i, self.manager.get_timestamp(str(i)))

  def test_clean_cache(self):
    dest_dir = tempfile.mkdtemp(prefix=u'named_cache_test')
    with self.manager.open():
      self.assertEqual([], os.listdir(self.manager.root_dir))

      a_path = os.path.join(dest_dir, u'a')
      b_path = os.path.join(dest_dir, u'b')

      self.manager.install(a_path, u'1')
      self.manager.install(b_path, u'2')

      self.assertEqual({u'a', u'b'}, set(os.listdir(dest_dir)))
      self.assertFalse(self.manager.available)
      self.assertEqual([], os.listdir(self.manager.root_dir))

      write_file(os.path.join(a_path, u'x'), u'x')
      write_file(os.path.join(b_path, u'y'), u'y')

      self.manager.uninstall(a_path, u'1')
      self.manager.uninstall(b_path, u'2')

      self.assertEqual(3, len(os.listdir(self.manager.root_dir)))
      path1 = os.path.join(self.manager.root_dir, self.manager._lru['1'])
      path2 = os.path.join(self.manager.root_dir, self.manager._lru['2'])

      self.assertEqual('x', read_file(os.path.join(path1, u'x')))
      self.assertEqual('y', read_file(os.path.join(path2, u'y')))
      self.assertEqual(os.readlink(self.manager._get_named_path('1')), path1)
      self.assertEqual(os.readlink(self.manager._get_named_path('2')), path2)

  def test_existing_cache(self):
    dest_dir = tempfile.mkdtemp(prefix=u'named_cache_test')
    with self.manager.open():
      # Assume test_clean passes.
      a_path = os.path.join(dest_dir, u'a')
      b_path = os.path.join(dest_dir, u'b')

      self.manager.install(a_path, u'1')
      write_file(os.path.join(dest_dir, u'a', u'x'), u'x')
      self.manager.uninstall(a_path, u'1')

      # Test starts here.
      self.manager.install(a_path, u'1')
      self.manager.install(b_path, u'2')
      self.assertEqual({'a', 'b'}, set(os.listdir(dest_dir)))
      self.assertFalse(self.manager.available)
      self.assertEqual(['named'], os.listdir(self.manager.root_dir))

      self.assertEqual(
          'x', read_file(os.path.join(os.path.join(dest_dir, u'a', u'x'))))
      write_file(os.path.join(a_path, 'x'), 'x2')
      write_file(os.path.join(b_path, 'y'), 'y')

      self.manager.uninstall(a_path, '1')
      self.manager.uninstall(b_path, '2')

      self.assertEqual(3, len(os.listdir(self.manager.root_dir)))
      path1 = os.path.join(self.manager.root_dir, self.manager._lru['1'])
      path2 = os.path.join(self.manager.root_dir, self.manager._lru['2'])

      self.assertEqual('x2', read_file(os.path.join(path1, 'x')))
      self.assertEqual('y', read_file(os.path.join(path2, 'y')))
      self.assertEqual(os.readlink(self.manager._get_named_path('1')), path1)
      self.assertEqual(os.readlink(self.manager._get_named_path('2')), path2)

  def test_trim(self):
    with self.manager.open():
      item_count = self.policies.max_items + 10
      self.make_caches(range(item_count))
      self.assertEqual(len(self.manager), item_count)
      self.manager.trim()
      self.assertEqual(len(self.manager), self.policies.max_items)
      self.assertEqual(
          set(map(str, xrange(10, 10 + self.policies.max_items))),
          set(os.listdir(os.path.join(self.tempdir, 'named'))))

  def test_corrupted(self):
    with open(os.path.join(self.tempdir, u'state.json'), 'w') as f:
      f.write('}}}}')
    fs.makedirs(os.path.join(self.tempdir, 'a'), 0777)
    with self.manager.open():
      self.assertFalse(os.path.isdir(self.tempdir))
      self.make_caches(['a'])
    self.assertTrue(fs.islink(os.path.join(self.tempdir, 'named', 'a')))

if __name__ == '__main__':
  fix_encoding.fix_encoding()
  VERBOSE = '-v' in sys.argv
  logging.basicConfig(level=logging.DEBUG if VERBOSE else logging.ERROR)
  unittest.main()
