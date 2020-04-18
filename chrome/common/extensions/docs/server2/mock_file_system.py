# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import posixpath

from file_system import FileSystem, FileNotFoundError
from future import Future
from test_file_system import _List, _StatTracker, TestFileSystem
from path_util import IsDirectory


class MockFileSystem(FileSystem):
  '''Wraps FileSystems to add a selection of mock behaviour:
  - asserting how often Stat/Read calls are being made to it.
  - primitive changes/versioning via applying object "diffs", mapping paths to
    new content (similar to how TestFileSystem works).
  '''
  def __init__(self, file_system):
    self._file_system = file_system
    # Updates are stored as TestFileSystems because it already implements a
    # bunch of logic to intepret paths into dictionaries.
    self._updates = []
    self._stat_tracker = _StatTracker()
    self._read_count = 0
    self._read_resolve_count = 0
    self._stat_count = 0
    self._version = None

  @staticmethod
  def Create(file_system, updates):
    mock_file_system = MockFileSystem(file_system)
    for update in updates:
      mock_file_system.Update(update)
    return mock_file_system

  #
  # FileSystem implementation.
  #

  def Read(self, paths, skip_not_found=False):
    '''Reads |paths| from |_file_system|, then applies the most recent update
    from |_updates|, if any.
    '''
    self._read_count += 1
    def next(result):
      self._read_resolve_count += 1
      for path in result.iterkeys():
        update = self._GetMostRecentUpdate(path)
        if update is not None:
          result[path] = update
      return result
    return self._file_system.Read(paths,
                                  skip_not_found=skip_not_found).Then(next)

  def Refresh(self):
    return self._file_system.Refresh()

  def _GetMostRecentUpdate(self, path):
    '''Returns the latest update for the file at |path|, or None if |path|
    has never been updated.
    '''
    for update in reversed(self._updates):
      try:
        return update.ReadSingle(path).Get()
      except FileNotFoundError:
        pass
    return None

  def Stat(self, path):
    self._stat_count += 1

    # This only supports numeric stat values since we need to add to it.  In
    # reality the logic here could just be to randomly mutate the stat values
    # every time there's an Update but that's less meaningful for testing.
    def stradd(a, b):
      return str(int(a) + b)

    stat = self._file_system.Stat(path)
    stat.version = stradd(stat.version, self._stat_tracker.GetVersion(path))
    if stat.child_versions:
      for child_path, child_version in stat.child_versions.iteritems():
        stat.child_versions[child_path] = stradd(
            stat.child_versions[child_path],
            self._stat_tracker.GetVersion(posixpath.join(path, child_path)))

    return stat

  def GetCommitID(self):
    return Future(value=str(self._stat_tracker.GetVersion('')))

  def GetPreviousCommitID(self):
    return Future(value=str(self._stat_tracker.GetVersion('') - 1))

  def GetIdentity(self):
    return self._file_system.GetIdentity()

  def GetVersion(self):
    return self._version

  def __str__(self):
    return repr(self)

  def __repr__(self):
    return 'MockFileSystem(read_count=%s, stat_count=%s, updates=%s)' % (
        self._read_count, self._stat_count, len(self._updates))

  #
  # Testing methods.
  #

  def GetStatCount(self):
    return self._stat_count

  def CheckAndReset(self, stat_count=0, read_count=0, read_resolve_count=0):
    '''Returns a tuple (success, error). Use in tests like:
    self.assertTrue(*object_store.CheckAndReset(...))
    '''
    errors = []
    for desc, expected, actual in (
        ('read_count', read_count, self._read_count),
        ('read_resolve_count', read_resolve_count, self._read_resolve_count),
        ('stat_count', stat_count, self._stat_count)):
      if actual != expected:
        errors.append('%s: expected %s got %s' % (desc, expected, actual))
    try:
      return (len(errors) == 0, ', '.join(errors))
    finally:
      self.Reset()

  def Reset(self):
    self._read_count = 0
    self._read_resolve_count = 0
    self._stat_count = 0

  def Update(self, update):
    self._updates.append(TestFileSystem(update))
    for path in _List(update).iterkeys():
      # Any files (not directories) which changed are now at the version
      # derived from |_updates|.
      if not IsDirectory(path):
        self._stat_tracker.SetVersion(path, len(self._updates))

  def SetVersion(self, version):
    '''Override the reported FileSystem version (default None) for testing.'''
    self._version = version
