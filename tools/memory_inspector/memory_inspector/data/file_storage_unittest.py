# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This unittest covers both file_storage and serialization modules."""

import os
import tempfile
import time
import unittest

from memory_inspector.core import memory_map
from memory_inspector.core import native_heap
from memory_inspector.core import stacktrace
from memory_inspector.core import symbol
from memory_inspector.data import file_storage


class FileStorageTest(unittest.TestCase):
  def setUp(self):
    self._storage_path = tempfile.mkdtemp()
    self._storage = file_storage.Storage(self._storage_path)

  def tearDown(self):
    os.removedirs(self._storage_path)

  def testSettings(self):
    settings_1 = { 'foo' : 1, 'bar' : 2 }
    settings_2 = { 'foo' : 1, 'bar' : 2 }
    self._storage.StoreSettings('one', settings_1)
    self._storage.StoreSettings('two', settings_2)
    self._DeepCompare(settings_1, self._storage.LoadSettings('one'))
    self._DeepCompare(settings_2, self._storage.LoadSettings('two'))
    self._storage.StoreSettings('one', {})
    self._storage.StoreSettings('two', {})

  def testArchives(self):
    self._storage.OpenArchive('foo', create=True)
    self._storage.OpenArchive('bar', create=True)
    self._storage.OpenArchive('baz', create=True)
    self._storage.DeleteArchive('bar')
    self.assertTrue('foo' in self._storage.ListArchives())
    self.assertFalse('bar' in self._storage.ListArchives())
    self.assertTrue('baz' in self._storage.ListArchives())
    self._storage.DeleteArchive('foo')
    self._storage.DeleteArchive('baz')

  def testSnapshots(self):
    archive = self._storage.OpenArchive('snapshots', create=True)
    t1 = archive.StartNewSnapshot()
    archive.StoreMemMaps(memory_map.Map())
    time.sleep(0.01) # Max snapshot resolution is in the order of usecs.
    t2 = archive.StartNewSnapshot()
    archive.StoreMemMaps(memory_map.Map())
    archive.StoreNativeHeap(native_heap.NativeHeap())
    self.assertIn(t1, archive.ListSnapshots())
    self.assertIn(t2, archive.ListSnapshots())
    self.assertTrue(archive.HasMemMaps(t1))
    self.assertFalse(archive.HasNativeHeap(t1))
    self.assertTrue(archive.HasMemMaps(t2))
    self.assertTrue(archive.HasNativeHeap(t2))
    self._storage.DeleteArchive('snapshots')

  def testMmap(self):
    archive = self._storage.OpenArchive('mmap', create=True)
    timestamp = archive.StartNewSnapshot()
    mmap = memory_map.Map()
    map_entry1 = memory_map.MapEntry(4096, 8191, 'rw--', '/foo', 0)
    map_entry2 = memory_map.MapEntry(65536, 81919, 'rw--', '/bar', 4096)
    map_entry2.resident_pages = [5]
    mmap.Add(map_entry1)
    mmap.Add(map_entry2)
    archive.StoreMemMaps(mmap)
    mmap_deser = archive.LoadMemMaps(timestamp)
    self._DeepCompare(mmap, mmap_deser)
    self._storage.DeleteArchive('mmap')

  def testNativeHeap(self):
    archive = self._storage.OpenArchive('nheap', create=True)
    timestamp = archive.StartNewSnapshot()
    nh = native_heap.NativeHeap()
    for i in xrange(1, 4):
      stack_trace = stacktrace.Stacktrace()
      frame = nh.GetStackFrame(i * 10 + 1)
      frame.SetExecFileInfo('foo.so', 1)
      stack_trace.Add(frame)
      frame = nh.GetStackFrame(i * 10 + 2)
      frame.SetExecFileInfo('bar.so', 2)
      stack_trace.Add(frame)
      nh.Add(native_heap.Allocation(size=i * 10,
                                    stack_trace=stack_trace,
                                    start=i * 20,
                                    flags=i * 30))
    archive.StoreNativeHeap(nh)
    nh_deser = archive.LoadNativeHeap(timestamp)
    self._DeepCompare(nh, nh_deser)
    self._storage.DeleteArchive('nheap')

  def testSymbols(self):
    archive = self._storage.OpenArchive('symbols', create=True)
    symbols = symbol.Symbols()
    # Symbol db is global per archive, no need to StartNewSnapshot.
    symbols.Add('foo.so', 1, symbol.Symbol('sym1', 'file1.c', 11))
    symbols.Add('bar.so', 2, symbol.Symbol('sym2', 'file2.c', 12))
    sym3 = symbol.Symbol('sym3', 'file2.c', 13)
    sym3.AddSourceLineInfo('outer_file.c', 23)
    symbols.Add('baz.so', 3, sym3)
    archive.StoreSymbols(symbols)
    symbols_deser = archive.LoadSymbols()
    self._DeepCompare(symbols, symbols_deser)
    self._storage.DeleteArchive('symbols')

  def _DeepCompare(self, a, b, prefix=''):
    """Recursively compares two objects (original and deserialized)."""

    self.assertEqual(a is None, b is None)
    if a is None:
      return

    _BASICTYPES = (long, int, basestring, float)
    if isinstance(a, _BASICTYPES) and isinstance(b, _BASICTYPES):
      return self.assertEqual(a, b, prefix)

    self.assertEqual(type(a), type(b), prefix + ' type (%s vs %s' % (
        type(a), type(b)))

    if isinstance(a, list):
      self.assertEqual(len(a), len(b), prefix + ' len (%d vs %d)' % (
          len(a), len(b)))
      for i in range(len(a)):
        self._DeepCompare(a[i], b[i], prefix + '[%d]' % i)
      return

    if isinstance(a, dict):
      self.assertEqual(a.keys(), b.keys(), prefix + ' keys (%s vs %s)' % (
        str(a.keys()), str(b.keys())))
      for k in a.iterkeys():
        self._DeepCompare(a[k], b[k], prefix + '.' + str(k))
      return

    return self._DeepCompare(a.__dict__, b.__dict__, prefix)