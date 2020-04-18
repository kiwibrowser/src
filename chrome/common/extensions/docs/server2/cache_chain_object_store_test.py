#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from cache_chain_object_store import CacheChainObjectStore
from test_object_store import TestObjectStore
import unittest

class CacheChainObjectStoreTest(unittest.TestCase):
  def setUp(self):
    self._first = TestObjectStore('first', init={
      'storage.html': 'storage',
    })
    self._second = TestObjectStore('second', init={
      'runtime.html': 'runtime',
      'storage.html': 'storage',
    })
    self._third = TestObjectStore('third', init={
      'commands.html': 'commands',
      'runtime.html': 'runtime',
      'storage.html': 'storage',
    })
    self._store = CacheChainObjectStore(
        (self._first, self._second, self._third))

  def testGetFromFirstLayer(self):
    self.assertEqual('storage', self._store.Get('storage.html').Get())
    self.assertTrue(*self._first.CheckAndReset(get_count=1))
    # Found in first layer, stop.
    self.assertTrue(*self._second.CheckAndReset())
    self.assertTrue(*self._third.CheckAndReset())
    # Cached in memory, won't re-query.
    self.assertEqual('storage', self._store.Get('storage.html').Get())
    self.assertTrue(*self._first.CheckAndReset())
    self.assertTrue(*self._second.CheckAndReset())
    self.assertTrue(*self._third.CheckAndReset())

  def testGetFromSecondLayer(self):
    self.assertEqual('runtime', self._store.Get('runtime.html').Get())
    # Not found in first layer but found in second.
    self.assertTrue(*self._first.CheckAndReset(get_count=1, set_count=1))
    self.assertTrue(*self._second.CheckAndReset(get_count=1))
    self.assertTrue(*self._third.CheckAndReset())
    # First will now have it cached.
    self.assertEqual('runtime', self._first.Get('runtime.html').Get())
    self._first.Reset()
    # Cached in memory, won't re-query.
    self.assertEqual('runtime', self._store.Get('runtime.html').Get())
    self.assertTrue(*self._first.CheckAndReset())
    self.assertTrue(*self._second.CheckAndReset())
    self.assertTrue(*self._third.CheckAndReset())

  def testGetFromThirdLayer(self):
    self.assertEqual('commands', self._store.Get('commands.html').Get())
    # As above but for third.
    self.assertTrue(*self._first.CheckAndReset(get_count=1, set_count=1))
    self.assertTrue(*self._second.CheckAndReset(get_count=1, set_count=1))
    self.assertTrue(*self._third.CheckAndReset(get_count=1))
    # First and second will now have it cached.
    self.assertEqual('commands', self._first.Get('commands.html').Get())
    self.assertEqual('commands', self._second.Get('commands.html').Get())
    self._first.Reset()
    self._second.Reset()
    # Cached in memory, won't re-query.
    self.assertEqual('commands', self._store.Get('commands.html').Get())
    self.assertTrue(*self._first.CheckAndReset())
    self.assertTrue(*self._second.CheckAndReset())
    self.assertTrue(*self._third.CheckAndReset())

  def testGetFromAllLayers(self):
    self.assertEqual({
      'commands.html': 'commands',
      'runtime.html': 'runtime',
      'storage.html': 'storage',
    }, self._store.GetMulti(('commands.html',
                             'runtime.html',
                             'storage.html')).Get())
    self.assertTrue(*self._first.CheckAndReset(get_count=1, set_count=1))
    self.assertTrue(*self._second.CheckAndReset(get_count=1, set_count=1))
    self.assertTrue(*self._third.CheckAndReset(get_count=1))
    # First and second will have it all cached.
    self.assertEqual('runtime', self._first.Get('runtime.html').Get())
    self.assertEqual('commands', self._first.Get('commands.html').Get())
    self.assertEqual('commands', self._second.Get('commands.html').Get())
    self._first.Reset()
    self._second.Reset()
    # Cached in memory.
    self.assertEqual({
      'commands.html': 'commands',
      'runtime.html': 'runtime',
      'storage.html': 'storage',
    }, self._store.GetMulti(('commands.html',
                             'runtime.html',
                             'storage.html')).Get())
    self.assertTrue(*self._first.CheckAndReset())
    self.assertTrue(*self._second.CheckAndReset())
    self.assertTrue(*self._third.CheckAndReset())

  def testPartiallyCachedInMemory(self):
    self.assertEqual({
      'commands.html': 'commands',
      'storage.html': 'storage',
    }, self._store.GetMulti(('commands.html', 'storage.html')).Get())
    self.assertTrue(*self._first.CheckAndReset(get_count=1, set_count=1))
    self.assertTrue(*self._second.CheckAndReset(get_count=1, set_count=1))
    self.assertTrue(*self._third.CheckAndReset(get_count=1))
    # runtime wasn't cached in memory, so stores should still be queried.
    self.assertEqual({
      'commands.html': 'commands',
      'runtime.html': 'runtime',
    }, self._store.GetMulti(('commands.html', 'runtime.html')).Get())
    self.assertTrue(*self._first.CheckAndReset(get_count=1, set_count=1))
    self.assertTrue(*self._second.CheckAndReset(get_count=1))
    self.assertTrue(*self._third.CheckAndReset())

  def testNotFound(self):
    self.assertEqual(None, self._store.Get('notfound.html').Get())
    self.assertTrue(*self._first.CheckAndReset(get_count=1))
    self.assertTrue(*self._second.CheckAndReset(get_count=1))
    self.assertTrue(*self._third.CheckAndReset(get_count=1))
    # Not-foundedness shouldn't be cached.
    self.assertEqual(None, self._store.Get('notfound.html').Get())
    self.assertTrue(*self._first.CheckAndReset(get_count=1))
    self.assertTrue(*self._second.CheckAndReset(get_count=1))
    self.assertTrue(*self._third.CheckAndReset(get_count=1))
    # Test some things not found, some things found.
    self.assertEqual({
      'runtime.html': 'runtime',
    }, self._store.GetMulti(('runtime.html', 'notfound.html')).Get())
    self.assertTrue(*self._first.CheckAndReset(get_count=1, set_count=1))
    self.assertTrue(*self._second.CheckAndReset(get_count=1))
    self.assertTrue(*self._third.CheckAndReset(get_count=1))

  def testSet(self):
    self._store.Set('hello.html', 'hello')
    self.assertTrue(*self._first.CheckAndReset(set_count=1))
    self.assertTrue(*self._second.CheckAndReset(set_count=1))
    self.assertTrue(*self._third.CheckAndReset(set_count=1))
    # Should have cached it.
    self.assertEqual('hello', self._store.Get('hello.html').Get())
    self.assertTrue(*self._first.CheckAndReset())
    self.assertTrue(*self._second.CheckAndReset())
    self.assertTrue(*self._third.CheckAndReset())
    # Should have the new content.
    self.assertEqual('hello', self._first.Get('hello.html').Get())
    self.assertEqual('hello', self._second.Get('hello.html').Get())
    self.assertEqual('hello', self._third.Get('hello.html').Get())

  def testDel(self):
    # Cache it.
    self.assertEqual('storage', self._store.Get('storage.html').Get())
    self.assertTrue(*self._first.CheckAndReset(get_count=1))
    # Delete it.
    self._store.Del('storage.html')
    self.assertTrue(*self._first.CheckAndReset(del_count=1))
    self.assertTrue(*self._second.CheckAndReset(del_count=1))
    self.assertTrue(*self._third.CheckAndReset(del_count=1))
    # Not cached anymore.
    self.assertEqual(None, self._store.Get('storage.html').Get())
    self.assertTrue(*self._first.CheckAndReset(get_count=1))
    self.assertTrue(*self._second.CheckAndReset(get_count=1))
    self.assertTrue(*self._third.CheckAndReset(get_count=1))

  def testStartEmpty(self):
    store = CacheChainObjectStore((self._first, self._second, self._third),
                                  start_empty=True)
    # Won't query delegate file systems because it starts empty.
    self.assertEqual(None, store.Get('storage.html').Get())
    self.assertTrue(*self._first.CheckAndReset())
    self.assertTrue(*self._second.CheckAndReset())
    self.assertTrue(*self._third.CheckAndReset())
    # Setting values will set on all delegates, though.
    store.Set('storage.html', 'new content')
    self.assertEqual('new content', store.Get('storage.html').Get())
    self.assertTrue(*self._first.CheckAndReset(set_count=1))
    self.assertTrue(*self._second.CheckAndReset(set_count=1))
    self.assertTrue(*self._third.CheckAndReset(set_count=1))
    self.assertEqual('new content', self._first.Get('storage.html').Get())
    self.assertEqual('new content', self._second.Get('storage.html').Get())
    self.assertEqual('new content', self._third.Get('storage.html').Get())

if __name__ == '__main__':
  unittest.main()
