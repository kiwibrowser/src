# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Contains on-disk caching functionality."""

from __future__ import print_function

import errno
import os
import shutil
import tempfile
import urlparse

from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import locking
from chromite.lib import osutils
from chromite.lib import retry_util


# pylint: disable=protected-access


def EntryLock(f):
  """Decorator that provides monitor access control."""

  def new_f(self, *args, **kwargs):
    # Ensure we don't have a read lock before potentially blocking while trying
    # to access the monitor.
    if self.read_locked:
      raise AssertionError(
          'Cannot call %s while holding a read lock.' % f.__name__)

    with self._entry_lock:
      self._entry_lock.write_lock()
      return f(self, *args, **kwargs)
  return new_f


def WriteLock(f):
  """Decorator that takes a write lock."""

  def new_f(self, *args, **kwargs):
    with self._lock.write_lock():
      return f(self, *args, **kwargs)
  return new_f


class CacheReference(object):
  """Encapsulates operations on a cache key reference.

  CacheReferences are returned by the DiskCache.Lookup() function.  They are
  used to read from and insert into the cache.

  A typical example of using a CacheReference:

  @contextlib.contextmanager
  def FetchFromCache()
    with cache.Lookup(key) as ref:
       # If entry doesn't exist in cache already, generate it ourselves, and
       # insert it into the cache, acquiring a read lock on it in the process.
       # If the entry does exist, we grab a read lock on it.
      if not ref.Exists(lock=True):
        path = PrepareItem()
        ref.SetDefault(path, lock=True)

      # yield the path to the cached entry to consuming code.
      yield ref.path
  """

  def __init__(self, cache, key):
    self._cache = cache
    self.key = key
    self.acquired = False
    self.read_locked = False
    self._lock = cache._LockForKey(key)
    self._entry_lock = cache._LockForKey(key, suffix='.entry_lock')

  @property
  def path(self):
    """Returns on-disk path to the cached item."""
    return self._cache._GetKeyPath(self.key)

  def Acquire(self):
    """Prepare the cache reference for operation.

    This must be called (either explicitly or through entering a 'with'
    context) before calling any methods that acquire locks, or mutates
    reference.
    """
    if self.acquired:
      raise AssertionError(
          'Attempting to acquire an already acquired reference.')

    self.acquired = True
    self._lock.__enter__()

  def Release(self):
    """Release the cache reference.  Causes any held locks to be released."""
    if not self.acquired:
      raise AssertionError(
          'Attempting to release an unacquired reference.')

    self.acquired = False
    self._lock.__exit__(None, None, None)

  def __enter__(self):
    self.Acquire()
    return self

  def __exit__(self, *args):
    self.Release()

  def _ReadLock(self):
    self._lock.read_lock()
    self.read_locked = True

  @WriteLock
  def _Assign(self, path):
    self._cache._Insert(self.key, path)

  @WriteLock
  def _AssignText(self, text):
    self._cache._InsertText(self.key, text)

  @WriteLock
  def _Remove(self):
    self._cache._Remove(self.key)

  def _Exists(self):
    return self._cache._KeyExists(self.key)

  @EntryLock
  def Assign(self, path):
    """Insert a file or a directory into the cache at the referenced key."""
    self._Assign(path)

  @EntryLock
  def AssignText(self, text):
    """Create a file containing |text| and assign it to the key.

    Args:
      text: Can be a string or an iterable.
    """
    self._AssignText(text)

  @EntryLock
  def Remove(self):
    """Removes the entry from the cache."""
    self._Remove()

  @EntryLock
  def Exists(self, lock=False):
    """Tests for existence of entry.

    Args:
      lock: If the entry exists, acquire and maintain a read lock on it.
    """
    if self._Exists():
      if lock:
        self._ReadLock()
      return True
    return False

  @EntryLock
  def SetDefault(self, default_path, lock=False):
    """Assigns default_path if the entry doesn't exist.

    Args:
      default_path: The path to assign if the entry doesn't exist.
      lock: Acquire and maintain a read lock on the entry.
    """
    if not self._Exists():
      self._Assign(default_path)
    if lock:
      self._ReadLock()


class DiskCache(object):
  """Locked file system cache keyed by tuples.

  Key entries can be files or directories.  Access to the cache is provided
  through CacheReferences, which are retrieved by using the cache Lookup()
  method.
  """
  # TODO(rcui): Add LRU cleanup functionality.

  _STAGING_DIR = 'staging'

  def __init__(self, cache_dir, cache_user=None):
    self._cache_dir = cache_dir
    self._cache_user = cache_user
    self.staging_dir = os.path.join(cache_dir, self._STAGING_DIR)

    osutils.SafeMakedirsNonRoot(self._cache_dir, user=self._cache_user)
    osutils.SafeMakedirsNonRoot(self.staging_dir, user=self._cache_user)

  def _KeyExists(self, key):
    return os.path.exists(self._GetKeyPath(key))

  def _GetKeyPath(self, key):
    """Get the on-disk path of a key."""
    return os.path.join(self._cache_dir, '+'.join(key))

  def _LockForKey(self, key, suffix='.lock'):
    """Returns an unacquired lock associated with a key."""
    key_path = self._GetKeyPath(key)
    osutils.SafeMakedirsNonRoot(os.path.dirname(key_path),
                                user=self._cache_user)
    lock_path = os.path.join(self._cache_dir, os.path.dirname(key_path),
                             os.path.basename(key_path) + suffix)
    return locking.FileLock(lock_path)

  def _TempDirContext(self):
    return osutils.TempDir(base_dir=self.staging_dir)

  def _Insert(self, key, path):
    """Insert a file or a directory into the cache at a given key."""
    self._Remove(key)
    key_path = self._GetKeyPath(key)
    osutils.SafeMakedirsNonRoot(os.path.dirname(key_path),
                                user=self._cache_user)
    shutil.move(path, key_path)

  def _InsertText(self, key, text):
    """Inserts a file containing |text| into the cache."""
    with self._TempDirContext() as tempdir:
      file_path = os.path.join(tempdir, 'tempfile')
      osutils.WriteFile(file_path, text)
      self._Insert(key, file_path)

  def _Remove(self, key):
    """Remove a key from the cache."""
    if self._KeyExists(key):
      with self._TempDirContext() as tempdir:
        shutil.move(self._GetKeyPath(key), tempdir)

  def Lookup(self, key):
    """Get a reference to a given key."""
    return CacheReference(self, key)


class RemoteCache(DiskCache):
  """Supports caching of remote objects via URI."""

  def _Fetch(self, url, local_path):
    """Fetch a remote file."""
    # We have to nest the import because gs.GSContext uses us to cache its own
    # gsutil tarball.  We know we won't get into a recursive loop though as it
    # only fetches files via non-gs URIs.
    from chromite.lib import gs

    if gs.PathIsGs(url):
      ctx = gs.GSContext()
      ctx.Copy(url, local_path)
    else:
      # Note: unittests assume local_path is at the end.
      retry_util.RunCurl(['--fail', url, '-o', local_path],
                         debug_level=logging.DEBUG, capture_output=True)

  def _Insert(self, key, url):
    """Insert a remote file into the cache."""
    o = urlparse.urlparse(url)
    if o.scheme in ('file', ''):
      DiskCache._Insert(self, key, o.path)
      return

    with tempfile.NamedTemporaryFile(dir=self.staging_dir,
                                     delete=False) as local_path:
      self._Fetch(url, local_path.name)
      DiskCache._Insert(self, key, local_path.name)


def Untar(path, cwd, sudo=False):
  """Untar a tarball."""
  functor = cros_build_lib.SudoRunCommand if sudo else cros_build_lib.RunCommand
  functor(['tar', '-xpf', path], cwd=cwd, debug_level=logging.DEBUG)


class TarballCache(RemoteCache):
  """Supports caching of extracted tarball contents."""

  def _Insert(self, key, tarball_path):
    """Insert a tarball and its extracted contents into the cache.

    Download the tarball first if a URL is provided as tarball_path.
    """
    with osutils.TempDir(prefix='tarball-cache',
                         base_dir=self.staging_dir) as tempdir:

      o = urlparse.urlsplit(tarball_path)
      if o.scheme == 'file':
        tarball_path = o.path
      elif o.scheme:
        url = tarball_path
        tarball_path = os.path.join(tempdir, os.path.basename(o.path))
        self._Fetch(url, tarball_path)

      extract_path = os.path.join(tempdir, 'extract')
      os.mkdir(extract_path)
      Untar(tarball_path, extract_path)
      DiskCache._Insert(self, key, extract_path)

  def _KeyExists(self, key):
    """Specialized DiskCache._KeyExits that ignores empty directories.

    The normal _KeyExists just checks to see if the key path exists in the cache
    directory. Many tests mock out RunCommand then fetch a tarball. The mock
    blocks untarring into it. This leaves behind an empty dir which blocks
    future untarring in non-test scripts.

    See crbug.com/468838
    """
    # Wipe out empty directories before testing for existence.
    key_path = self._GetKeyPath(key)

    try:
      os.rmdir(key_path)
    except OSError as ex:
      if ex.errno not in (errno.ENOTEMPTY, errno.ENOENT):
        raise

    return os.path.exists(key_path)
