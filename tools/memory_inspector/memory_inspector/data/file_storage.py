# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module handles file-backed storage of the core classes.

The storage is logically organized as follows:
Storage -> N Archives -> 1 Symbol index
                         N Snapshots     -> 1 Mmaps dump.
                                         -> 0/1 Native heap dump.

Where an "archive" is essentially a collection of snapshots taken for a given
app at a given point in time.
"""

import datetime
import json
import os

from memory_inspector.core import memory_map
from memory_inspector.core import native_heap
from memory_inspector.core import symbol
from memory_inspector.data import serialization


class Storage(object):

  _SETTINGS_FILE = 'settings-%s.json'

  def __init__(self, root_path):
    """Creates a file-backed storage. Files will be placed in |root_path|."""
    self._root = root_path
    if not os.path.exists(self._root):
      os.makedirs(self._root)

  def LoadSettings(self, name):
    """Loads a key-value dict from the /settings-name.json file.

    This is for backend and device settings (e.g., symbols path, adb path)."""
    file_path = os.path.join(self._root, Storage._SETTINGS_FILE % name)
    if not os.path.exists(file_path):
      return {}
    with open(file_path) as f:
      return json.load(f)

  def StoreSettings(self, name, settings):
    """Stores a key-value dict into /settings-name.json file."""
    assert(isinstance(settings, dict))
    file_path = os.path.join(self._root, Storage._SETTINGS_FILE % name)
    if not settings:
      if os.path.exists(file_path):
        os.unlink(file_path)
      return
    with open(file_path, 'w') as f:
      return json.dump(settings, f)

  def ListArchives(self):
    """Lists archives. Each of them is a sub-folder inside the |root_path|."""
    return sorted(
        [name for name in os.listdir(self._root)
            if os.path.isdir(os.path.join(self._root, name))])

  def OpenArchive(self, archive_name, create=False):
    """Returns an instance of |Archive|."""
    archive_path = os.path.join(self._root, archive_name)
    if not os.path.exists(archive_path) and create:
      os.makedirs(archive_path)
    return Archive(archive_name, archive_path)

  def DeleteArchive(self, archive_name):
    """Deletes the archive (removing its folder)."""
    archive_path = os.path.join(self._root, archive_name)
    for f in os.listdir(archive_path):
      os.unlink(os.path.join(archive_path, f))
    os.rmdir(archive_path)


class Archive(object):
  """A collection of snapshots, each one holding one memory dump (per kind)."""

  _MMAP_EXT = '-mmap.json'
  _NHEAP_EXT = '-nheap.json'
  _SNAP_EXT = '.snapshot'
  _SYM_FILE = 'syms.json'
  _TIME_FMT = '%Y-%m-%d_%H-%M-%S-%f'

  def __init__(self, name, path):
    assert(os.path.isdir(path))
    self._name = name
    self._path = path
    self._cur_snapshot = None

  def StoreSymbols(self, symbols):
    """Stores the symbol db (one per the overall archive)."""
    assert(isinstance(symbols, symbol.Symbols))
    file_path = os.path.join(self._path, Archive._SYM_FILE)
    with open(file_path, 'w') as f:
      json.dump(symbols, f, cls=serialization.Encoder)

  def HasSymbols(self):
    return os.path.exists(os.path.join(self._path, Archive._SYM_FILE))

  def LoadSymbols(self):
    assert(self.HasSymbols())
    file_path = os.path.join(self._path, Archive._SYM_FILE)
    with open(file_path) as f:
      return json.load(f, cls=serialization.SymbolsDecoder)

  def StartNewSnapshot(self):
    """Creates a 2014-01-01_02:03:04.snapshot marker (an empty file)."""
    self._cur_snapshot = Archive.TimestampToStr(datetime.datetime.now())
    file_path = os.path.join(self._path,
                            self._cur_snapshot + Archive._SNAP_EXT)
    assert(not os.path.exists(file_path))
    open(file_path, 'w').close()
    return datetime.datetime.strptime(self._cur_snapshot, Archive._TIME_FMT)

  def ListSnapshots(self):
    """Returns a list of timestamps (datetime.datetime instances)."""
    file_names = sorted(
        [name[:-(len(Archive._SNAP_EXT))] for name in os.listdir(self._path)
            if name.endswith(Archive._SNAP_EXT)])
    timestamps = [datetime.datetime.strptime(x, Archive._TIME_FMT)
                  for x in file_names]
    return timestamps

  def StoreMemMaps(self, mmaps):
    assert(isinstance(mmaps, memory_map.Map))
    assert(self._cur_snapshot), 'Must call StartNewSnapshot first'
    file_path = os.path.join(self._path, self._cur_snapshot + Archive._MMAP_EXT)
    with open(file_path, 'w') as f:
      json.dump(mmaps, f, cls=serialization.Encoder)

  def HasMemMaps(self, timestamp):
    return self._HasSnapshotFile(timestamp, Archive._MMAP_EXT)

  def LoadMemMaps(self, timestamp):
    assert(self.HasMemMaps(timestamp))
    snapshot_name = Archive.TimestampToStr(timestamp)
    file_path = os.path.join(self._path, snapshot_name + Archive._MMAP_EXT)
    with open(file_path) as f:
      return json.load(f, cls=serialization.MmapDecoder)

  def StoreNativeHeap(self, nheap):
    assert(isinstance(nheap, native_heap.NativeHeap))
    assert(self._cur_snapshot), 'Must call StartNewSnapshot first'
    file_path = os.path.join(self._path,
                             self._cur_snapshot + Archive._NHEAP_EXT)
    with open(file_path, 'w') as f:
      json.dump(nheap, f, cls=serialization.Encoder)

  def HasNativeHeap(self, timestamp):
    return self._HasSnapshotFile(timestamp, Archive._NHEAP_EXT)

  def LoadNativeHeap(self, timestamp):
    assert(self.HasNativeHeap(timestamp))
    snapshot_name = Archive.TimestampToStr(timestamp)
    file_path = os.path.join(self._path, snapshot_name + Archive._NHEAP_EXT)
    with open(file_path) as f:
      return json.load(f, cls=serialization.NativeHeapDecoder)

  def _HasSnapshotFile(self, timestamp, ext):
    name = Archive.TimestampToStr(timestamp)
    return os.path.exists(os.path.join(self._path, name + ext))

  @staticmethod
  def TimestampToStr(timestamp):
    return timestamp.strftime(Archive._TIME_FMT)

  @staticmethod
  def StrToTimestamp(timestr):
    return datetime.datetime.strptime(timestr, Archive._TIME_FMT)
