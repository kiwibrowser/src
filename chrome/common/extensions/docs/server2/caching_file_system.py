# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import posixpath
import sys

from file_system import FileSystem, StatInfo, FileNotFoundError
from future import All, Future
from path_util import AssertIsDirectory, IsDirectory, ToDirectory
from third_party.json_schema_compiler.memoize import memoize


class CachingFileSystem(FileSystem):
  '''FileSystem which implements a caching layer on top of |file_system|. If
  |fail_on_miss| is True then cache misses throw a FileNotFoundError rather than
  falling back onto the underlying FileSystem.
  '''
  def __init__(self, file_system, object_store_creator, fail_on_miss=False):
    self._file_system = file_system
    self._fail_on_miss = fail_on_miss
    def create_object_store(category, start_empty=True):
      return object_store_creator.Create(
          CachingFileSystem,
          category='%s/%s' % (file_system.GetIdentity(), category),
          start_empty=start_empty)
    # We only start the stat cache empty if |fail_on_miss| is False, i.e. if
    # we're NOT running on a live instance and we can afford to fall back onto
    # the underlying FileSystem impl.
    self._stat_cache = create_object_store('stat', start_empty=not fail_on_miss)
    self._read_cache = create_object_store('read', start_empty=False)
    self._walk_cache = create_object_store('walk', start_empty=False)

  def Refresh(self):
    return self._file_system.Refresh()

  def StatAsync(self, path):
    '''Stats the directory given, or if a file is given, stats the file's parent
    directory to get info about the file.
    '''
    # Always stat the parent directory, since it will have the stat of the child
    # anyway, and this gives us an entire directory's stat info at once.
    dir_path, file_path = posixpath.split(path)
    dir_path = ToDirectory(dir_path)

    def make_stat_info(dir_stat):
      '''Converts a dir stat into the correct resulting StatInfo; if the Stat
      was for a file, the StatInfo should just contain that file.
      '''
      if path == dir_path:
        return dir_stat
      # Was a file stat. Extract that file.
      file_version = dir_stat.child_versions.get(file_path)
      if file_version is None:
        raise FileNotFoundError('No stat found for %s in %s (found %s)' %
                                (path, dir_path, dir_stat.child_versions))
      return StatInfo(file_version)

    def raise_cache_miss(path):
      raise FileNotFoundError('Got cache miss when trying to stat %s' % path)

    dir_stat = self._stat_cache.Get(dir_path).Get()
    if dir_stat is not None:
      return Future(callback=lambda: make_stat_info(dir_stat))

    if self._fail_on_miss:
      logging.warning('Bailing on stat cache miss for %s on %s' %
                      (dir_path, self.GetIdentity()))
      return Future(callback=lambda: raise_cache_miss(dir_path))

    def next(dir_stat):
      assert dir_stat is not None  # should have raised a FileNotFoundError
      # We only ever need to cache the dir stat.
      self._stat_cache.Set(dir_path, dir_stat)
      return make_stat_info(dir_stat)
    return self._MemoizedStatAsyncFromFileSystem(dir_path).Then(next)

  @memoize
  def _MemoizedStatAsyncFromFileSystem(self, dir_path):
    '''This is a simple wrapper to memoize Futures to directory stats, since
    StatAsync makes heavy use of it. Only cache directories so that the
    memoized cache doesn't blow up.
    '''
    assert IsDirectory(dir_path)
    return self._file_system.StatAsync(dir_path)

  def Read(self, paths, skip_not_found=False):
    '''Reads a list of files. If a file is cached and it is not out of
    date, it is returned. Otherwise, the file is retrieved from the file system.
    '''
    # Files which aren't found are cached in the read object store as
    # (path, None, None). This is to prevent re-reads of files we know
    # do not exist.
    cached_read_values = self._read_cache.GetMulti(paths).Get()
    cached_stat_values = self._stat_cache.GetMulti(paths).Get()

    # Populate a map of paths to Futures to their stat. They may have already
    # been cached in which case their Future will already have been constructed
    # with a value.
    stat_futures = {}

    def handle(error):
      if isinstance(error, FileNotFoundError):
        return None
      raise error

    for path in paths:
      stat_value = cached_stat_values.get(path)
      if stat_value is None:
        stat_future = self.StatAsync(path)
        if skip_not_found:
          stat_future = stat_future.Then(lambda x: x, handle)
      else:
        stat_future = Future(value=stat_value)
      stat_futures[path] = stat_future

    # Filter only the cached data which is up to date by comparing to the latest
    # stat. The cached read data includes the cached version. Remove it for
    # the result returned to callers. |version| == None implies a non-existent
    # file, so skip it.
    up_to_date_data = dict(
        (path, data) for path, (data, version) in cached_read_values.iteritems()
        if version is not None and stat_futures[path].Get().version == version)

    if skip_not_found:
      # Filter out paths which we know do not exist, i.e. if |path| is in
      # |cached_read_values| *and* has a None version, then it doesn't exist.
      # See the above declaration of |cached_read_values| for more information.
      paths = [path for path in paths
               if cached_read_values.get(path, (None, True))[1]]

    remaining_paths = set(paths) - set(up_to_date_data.iterkeys())
    if len(remaining_paths) == 0:
      # Everything was cached and up to date.
      return Future(value=up_to_date_data)

    def raise_cache_miss(paths):
      raise FileNotFoundError('Got cache miss when trying to stat %s' % paths)

    if self._fail_on_miss:
      # Ignore missing values and return anyway.
      logging.warn('Read cache miss for %s on %s' %
                   (remaining_paths, self.GetIdentity()))
      return Future(callback=lambda: raise_cache_miss(remaining_paths))

    def next(new_results):
      # Update the cache. This is a path -> (data, version) mapping.
      self._read_cache.SetMulti(
          dict((path, (new_result, stat_futures[path].Get().version))
               for path, new_result in new_results.iteritems()))
      # Update the read cache to include files that weren't found, to prevent
      # constantly trying to read a file we now know doesn't exist.
      self._read_cache.SetMulti(
          dict((path, (None, None)) for path in paths
               if stat_futures[path].Get() is None))
      new_results.update(up_to_date_data)
      return new_results

    # Read in the values that were uncached or old.
    return self._file_system.Read(remaining_paths,
                                  skip_not_found=skip_not_found).Then(next)

  def GetCommitID(self):
    return self._file_system.GetCommitID()

  def GetPreviousCommitID(self):
    return self._file_system.GetPreviousCommitID()

  def Walk(self, root, depth=-1):
    '''Overrides FileSystem.Walk() to provide caching functionality.
    '''
    def file_lister(root):
      res, root_stat = All((self._walk_cache.Get(root),
                            self.StatAsync(root))).Get()

      if res and res[2] == root_stat.version:
        dirs, files = res[0], res[1]
      else:
        # Wasn't cached, or not up to date.
        dirs, files = [], []
        for f in self.ReadSingle(root).Get():
          if IsDirectory(f):
            dirs.append(f)
          else:
            files.append(f)
        # Update the cache. This is a root -> (dirs, files, version) mapping.
        self._walk_cache.Set(root, (dirs, files, root_stat.version))
      return dirs, files
    return self._file_system.Walk(root, depth=depth, file_lister=file_lister)

  def GetIdentity(self):
    return self._file_system.GetIdentity()

  def GetVersion(self):
    return self._file_system.GetVersion()

  def __repr__(self):
    return '%s of <%s>' % (type(self).__name__, repr(self._file_system))
