# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from compiled_file_system import CompiledFileSystem
from docs_server_utils import StringIdentity
from file_system import FileNotFoundError
from future import Future
from path_util import ToDirectory


class ChainedCompiledFileSystem(object):
  '''A CompiledFileSystem implementation that fetches data from a chain of
  possible FileSystems. The chain consists of some number of FileSystems which
  may have cached data for their CompiledFileSystem instances (injected on
  Factory construction) + the main FileSystem (injected at Creation time).

  The expected configuration is that the main FileSystem is a PatchedFileSystem
  and the chain the FileSystem which it patches, but with file systems
  constructed via the HostFileSystemIterator the main FileSystems could be
  anything.

  This slightly unusual configuration is primarily needed to avoid re-compiling
  data for PatchedFileSystems, which are very similar to the FileSystem which
  they patch. Re-compiling data is expensive and a waste of memory resources.
  ChainedCompiledFileSystem shares the data.
  '''
  class Factory(CompiledFileSystem.Factory):
    def __init__(self, file_system_chain, object_store):
      self._file_system_chain = file_system_chain
      self._object_store = object_store

    def Create(self, file_system, populate_function, cls, category=None):
      return ChainedCompiledFileSystem(
          # Chain of CompiledFileSystem instances.
          tuple(CompiledFileSystem.Factory(self._object_store).Create(
                    fs, populate_function, cls, category=category)
                for fs in [file_system] + self._file_system_chain),
          # Identity, as computed by all file systems.
          StringIdentity(*(fs.GetIdentity() for fs in self._file_system_chain)))

  def __init__(self, compiled_fs_chain, identity):
    '''|compiled_fs_chain| is a list of tuples (compiled_fs, file_system).
    '''
    assert len(compiled_fs_chain) > 0
    self._compiled_fs_chain = compiled_fs_chain
    self._identity = identity

  def GetFromFile(self, path, skip_not_found=False):
    return self._GetImpl(
        path,
        lambda cfs: cfs.GetFromFile(path, skip_not_found=skip_not_found),
        lambda cfs: cfs._GetFileVersionFromCache(path))

  def GetFromFileListing(self, path):
    path = ToDirectory(path)
    return self._GetImpl(
        path,
        lambda compiled_fs: compiled_fs.GetFromFileListing(path),
        lambda compiled_fs: compiled_fs._GetFileListingVersionFromCache(path))

  def _GetImpl(self, path, reader, version_getter):
    # Strategy: Get the current version of |path| in main FileSystem, then run
    # through |_compiled_fs_chain| in *reverse* to find the "oldest" FileSystem
    # with an up-to-date version of that file.
    #
    # Obviously, if files have been added in the main FileSystem then none of
    # the older FileSystems will be able to find it.
    read_and_version_futures = [(reader(fs), version_getter(fs), fs)
                                for fs in self._compiled_fs_chain]

    def resolve():
      try:
        # The first file system contains both files of a newer version and
        # files shared with other compiled file systems. We are going to try
        # each compiled file system in the reverse order and return the data
        # when version matches. Data cached in other compiled file system will
        # be reused whenever possible so that we don't need to recompile things
        # that are not changed across these file systems.
        first_version = read_and_version_futures[0][1].Get()
        for (read_future,
             version_future,
             compiled_fs) in reversed(read_and_version_futures):
          if version_future.Get() == first_version:
            return read_future.Get()
      except FileNotFoundError:
        pass
      # Try an arbitrary operation again to generate a realistic stack trace.
      return read_and_version_futures[0][0].Get()

    return Future(callback=resolve)

  def GetIdentity(self):
    return self._identity
