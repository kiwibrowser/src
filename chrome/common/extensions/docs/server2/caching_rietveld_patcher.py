# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from datetime import datetime, timedelta
from file_system import FileNotFoundError
from future import Future
from patcher import Patcher

_VERSION_CACHE_MAXAGE = timedelta(seconds=5)

''' Append @version for keys to distinguish between different patchsets of
an issue.
'''
def _MakeKey(path, version):
  return '%s@%s' % (path, version)

def _ToObjectStoreValue(raw_value, version):
  return dict((_MakeKey(key, version), raw_value[key])
              for key in raw_value)

def _FromObjectStoreValue(raw_value):
  return dict((key[0:key.rfind('@')], raw_value[key]) for key in raw_value)

class _AsyncUncachedFuture(object):
  def __init__(self,
               version,
               paths,
               cached_value,
               missing_paths,
               fetch_delegate,
               object_store):
    self._version = version
    self._paths = paths
    self._cached_value = cached_value
    self._missing_paths = missing_paths
    self._fetch_delegate = fetch_delegate
    self._object_store = object_store

  def Get(self):
    uncached_value = self._fetch_delegate.Get()
    self._object_store.SetMulti(_ToObjectStoreValue(uncached_value,
                                                    self._version))

    for path in self._missing_paths:
      if uncached_value.get(path) is None:
        raise FileNotFoundError('File %s was not found in the patch.' % path)
      self._cached_value[path] = uncached_value[path]

    return self._cached_value

class CachingRietveldPatcher(Patcher):
  ''' CachingRietveldPatcher implements a caching layer on top of |patcher|.
  In theory, it can be used with any class that implements Patcher. But this
  class assumes that applying to all patched files at once is more efficient
  than applying to individual files.
  '''
  def __init__(self,
               rietveld_patcher,
               object_store_creator,
               test_datetime=datetime):
    self._patcher = rietveld_patcher
    def create_object_store(category):
      return object_store_creator.Create(
          CachingRietveldPatcher,
          category='%s/%s' % (rietveld_patcher.GetIdentity(), category))
    self._version_object_store = create_object_store('version')
    self._list_object_store = create_object_store('list')
    self._file_object_store = create_object_store('file')
    self._datetime = test_datetime

  def GetVersion(self):
    key = 'version'
    value = self._version_object_store.Get(key).Get()
    if value is not None:
      version, time = value
      if self._datetime.now() - time < _VERSION_CACHE_MAXAGE:
        return version

    version = self._patcher.GetVersion()
    self._version_object_store.Set(key,
                                (version, self._datetime.now()))
    return version

  def GetPatchedFiles(self, version=None):
    if version is None:
      version = self.GetVersion()
    patched_files = self._list_object_store.Get(version).Get()
    if patched_files is not None:
      return patched_files

    patched_files = self._patcher.GetPatchedFiles(version)
    self._list_object_store.Set(version, patched_files)
    return patched_files

  def Apply(self, paths, file_system, version=None):
    if version is None:
      version = self.GetVersion()
    added, deleted, modified = self.GetPatchedFiles(version)
    cached_value = _FromObjectStoreValue(self._file_object_store.
        GetMulti([_MakeKey(path, version) for path in paths]).Get())
    missing_paths = list(set(paths) - set(cached_value.keys()))
    if len(missing_paths) == 0:
      return Future(value=cached_value)

    return _AsyncUncachedFuture(version,
                                paths,
                                cached_value,
                                missing_paths,
                                self._patcher.Apply(set(added) | set(modified),
                                                    None,
                                                    version),
                                self._file_object_store)

  def GetIdentity(self):
    return self._patcher.GetIdentity()
