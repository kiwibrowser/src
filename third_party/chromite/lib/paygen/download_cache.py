# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Downloads files upon request in a thread/process safe way.

DEPRECATED: Should be merged into chromite.lib.cache.
"""

from __future__ import print_function

import md5
import os
import shutil
import stat
import tempfile
import time

from chromite.lib import locking
from chromite.lib import osutils
from chromite.lib.paygen import urilib
from chromite.lib.paygen import utils


FETCH_RETRY_COUNT = 10
DEFAULT_DAYS_TO_KEEP = 1
ONE_DAY = 24 * 60 * 60


class RetriesExhaustedError(Exception):
  """Raised when we make too many attempts to download the same file."""


def _DefaultFetchFunc(uri, cache_file):
  """The default fetch function.

  This simply downloads the uri into the cache file using urilib

  Args:
    uri: The URI to download.
    cache_file: The path to put the downloaded file in.
  """
  urilib.Copy(uri, cache_file)


class DownloadCache(object):
  """This class downloads files into a local directory upon request.

  This classes uses locking to make this safe across processes, and
  threads.

  Example usage:

    # This will create the cache dir, and purge old contents.
    cache = DownloadCache('/tmp/my_cache')

    # file is copied into file, blocking for download if needed.
    cache.GetFileCopy('gs://bucket/foo', '/tmp/foo')

    # file is loaded into cache, but not locked.
    tempfile = cache.GetFileInTempFile('gs://bucket/foo')
    tempfile.close()
  """

  # Name of the purge management lock over the entire cache.
  _CACHE_LOCK = 'cache.lock'
  _FILE_DIR = 'cache'
  _LOCK_DIR = 'lock'

  _GET_FILE_SPIN_DELAY = 2

  def __init__(self, cache_dir, max_age=ONE_DAY, cache_size=None):
    """Create a DownloadCache.

    Since Purging is not performed very often, we can exceed max_age or
    cache_size.

    Args:
      cache_dir: The directory in which to create the cache.
      max_age: Purge files not used for this number of seconds. None for no
               max_age.
      cache_size: Purge the least recently used files until the cache is
                  below this size in bytes. None for no size limit.

      If no condition is provided, we purge all files unused for one full day.
    """
    # One directory for cached files, one for lock files.
    self._cache_dir = os.path.realpath(cache_dir)
    self._file_dir = os.path.join(self._cache_dir, self._FILE_DIR)
    self._lock_dir = os.path.join(self._cache_dir, self._LOCK_DIR)

    self._max_age = max_age
    self._cache_size = cache_size

    self._SetupCache()

  def _SetupCache(self):
    """Make sure that our cache contains only files/directories we expect."""
    try:
      osutils.SafeMakedirs(self._cache_dir)
      # The purge lock ensures nobody else is modifying the cache in any way.
      with self._PurgeLock(blocking=False, shared=False):
        # We have changed the layout of our cache directories over time.
        # Clean up any left over files.
        expected = (self._CACHE_LOCK, self._FILE_DIR, self._LOCK_DIR)
        unexpected = set(os.listdir(self._cache_dir)).difference(expected)

        for name in unexpected:
          filename = os.path.join(self._cache_dir, name)
          if os.path.isdir(filename):
            shutil.rmtree(filename)
          else:
            os.unlink(filename)

        # Create the cache file dir if needed.
        if not os.path.exists(self._file_dir):
          os.makedirs(self._file_dir)

        # Create the lock dir if needed.
        if not os.path.exists(self._lock_dir):
          os.makedirs(self._lock_dir)
    except locking.LockNotAcquiredError:
      # If we can't get an exclusive lock on the cache, someone else set it up.
      pass

  def _UriToCacheFile(self, uri):
    """Convert a URI to an cache file (full path).

    Args:
      uri: The uri of the file to be cached locally.

    Returns:
      The full path file name of the cache file associated with a given URI.
    """
    # We use the md5 hash of the URI as our file name. This allows us to
    # store all cache files in a single directory, which removes race
    # conditions around directories.
    m = md5.new(uri)
    return os.path.join(self._file_dir, m.digest().encode('hex'))

  def _PurgeLock(self, blocking=False, shared=False):
    """Acquire a lock on the cache as a whole.

    An exclusive lock proves nobody else will modify anything, and nobody
    else will hold any _CacheFileLocks. A shared lock is required before
    getting any kind of _CacheFileLock.

    Args:
      blocking: Block until the lock is available?
      shared: Get a shared lock, or an exclusive lock?

    Returns:
      Locking.FileLock (acquired)
    """
    lock_file = os.path.join(self._cache_dir, self._CACHE_LOCK)
    lock = locking.FileLock(lock_file, locktype=locking.FLOCK,
                            blocking=blocking)
    return lock.lock(shared)

  def _CacheFileLock(self, cache_file, blocking=False, shared=False):
    """Acquire a lock on a file in the cache.

    A shared lock will ensure no other processes are modifying the file, but
    getting it does not ensure that the file in question actually exists.

    An exclusive lock is required to modify a cache file, this usually means
    downloading it.

    A shared _PurgeLock should be held before trying to acquire any type
    of cache file lock.

    Args:
      cache_file: The full path of file in cache to lock.
      blocking: Block until the lock is available?
      shared: Get a shared lock, or an exclusive lock?

    Returns:
      Locking.FileLock (acquired)
    """
    lock_file = os.path.join(self._lock_dir, os.path.basename(cache_file))
    lock = locking.FileLock(lock_file, locktype=locking.FLOCK,
                            blocking=blocking)
    return lock.lock(shared)

  def Purge(self, max_age=None, cache_size=None):
    """Attempts to clean up the cache contents.

    Is a no-op if cache lock is not acquirable.

    Args:
      max_age: Overrides the __init__ max_age for this one
                       purge. Mostly intended for unittests.
      cache_size: Overrides the __init__ cache_size for this one
                       purge. Mostly intended for unittests.
    """
    max_age = self._max_age if max_age is None else max_age
    cache_size = self._cache_size if cache_size is None else cache_size

    try:
      # Prevent other changes while we purge the cache.
      with self._PurgeLock(shared=False, blocking=False):

        # Purge files based on age, if specified.
        if max_age is not None:
          now = time.time()
          for f in utils.ListdirFullpath(self._file_dir):
            if (now - os.path.getmtime(f)) > max_age:
              os.unlink(f)

        # Purge files based on size, if specified.
        if cache_size is not None:
          # Find cache files, and sort them so the oldest are first.
          # This defines which ones we will purge first.
          cache_files = utils.ListdirFullpath(self._file_dir)
          cache_files.sort(key=os.path.getmtime)

          sizes = [os.path.getsize(f) for f in cache_files]
          total_size = sum(sizes)

          # Remove files until we are small enough to fit.
          for f, size in zip(cache_files, sizes):
            if total_size < cache_size:
              break
            total_size -= size
            os.unlink(f)

        # Just remove all lock files. They will be recreated as needed.
        shutil.rmtree(self._lock_dir)
        os.makedirs(self._lock_dir)

    except locking.LockNotAcquiredError:
      # If we can't get an exclusive lock on the file, it's in use, leave it.
      pass

  def _FetchIntoCache(self, uri, cache_file, fetch_func=_DefaultFetchFunc):
    """This function downloads the specified file (if not already local).

    You must hold the PurgeLock when calling this method.

    If it can't get an exclusive lock, or if the file is already present,
    it does nothing.

    Args:
      uri: The uri of the file.
      cache_file: The location in the cache to download too.
      fetch_func: Function to get the file.

    Returns:
      True if a file was downloaded, False otherwise. (used in unittests)

    Raises:
      May raise any download error associated with the URI's protocol.
    """
    try:
      # Write protect the file before modifying it.
      with self._CacheFileLock(cache_file, shared=False, blocking=False):
        if os.path.exists(cache_file):
          return False

        try:
          fetch_func(uri, cache_file)
          # Make the file read-only by everyone.
          os.chmod(cache_file, stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH)
        except:
          # If there was any error with the download, make sure no partial
          # file was left behind.
          if os.path.exists(cache_file):
            os.unlink(cache_file)
          raise

    except locking.LockNotAcquiredError:
      # In theory, if it's already locked, that either means a download is in
      # progress, or there is a shared lock which means it's already present.
      return False

    # Try to cleanup the cache after we just grew it.
    self.Purge()
    return True

  # TODO: Instead of hooking in fetch functions in the cache here, we could
  # set up protocol handlers which would know how to handle special cases
  # generally, identified by a protocol prefix like "prepimage://" or
  # "decompress://". That would help make sure they're handled consistently.
  def GetFileObject(self, uri, fetch_func=_DefaultFetchFunc):
    """Get an open readonly File object for the file in the cache.

    This method will populate the cache with the requested file if it's
    not already present, and will return an already opened read only file
    object for the cache contents.

    Even if the file is purged, this File object will remain valid until
    closed. Since this method is the only legitimate way to get access to
    a file in the cache, and it returns read only Files, cache files should
    never be modified.

    This method may block while trying to download and/or lock the file.

    Args:
      uri: The uri of the file to access.
      fetch_func: A function to produce the file if it isn't already in the
                  cache.

    Returns:
      File object opened with 'rb' mode.

    Raises:
      Exceptions from a failed download are passed through 'as is' from
      the underlying download mechanism.

      RetriesExhaustedError if we need a large number of attempts to
      download the same file.
    """
    cache_file = self._UriToCacheFile(uri)

    # We keep trying until we succeed, or throw an exception.
    for _ in xrange(FETCH_RETRY_COUNT):
      with self._PurgeLock(shared=True, blocking=True):
        # Attempt to download the file, if needed.
        self._FetchIntoCache(uri, cache_file, fetch_func)

        # Get a shared lock on the file. This can block if another process
        # has a non-shared lock (ie: they are downloading).
        with self._CacheFileLock(cache_file, shared=True, blocking=True):

          if os.path.exists(cache_file):
            fd = open(cache_file, 'rb')

            # Touch the timestamp on cache file to help purging logic.
            os.utime(cache_file, None)

            return fd
          else:
            # We don't have the file in our cache. There are three ways this
            # can happen:
            #
            # A) Another process was trying to download, blocked our download,
            #    then got a download error.
            # B) Another process removed the file(illegally). We will recover as
            #    soon as all read-only locks are released.
            # C) Our download failed without throwing an exception. We will
            #    block forever if this continues to happen.

            # Sleep so we don't spin too quickly, then try again.
            time.sleep(self._GET_FILE_SPIN_DELAY)

    raise RetriesExhaustedError(uri)

  def GetFileCopy(self, uri, filepath):
    """Copy a cache file into your file (downloading as needed).

    Copy the file into your specified filename (creating or overridding). It
    will be downloaded into the cache first, if needed. It is your
    responsibility to manage filepath after it is populated.

    Args:
      uri: The uri of the file to access.
      filepath: The name of the file to copy uri contents into.

    Raises:
      Exceptions from a failed download are passed through 'as is' from
      the underlying download mechanism.
    """
    with self.GetFileObject(uri) as src:
      with open(filepath, 'w+b') as dest:
        shutil.copyfileobj(src, dest)

  def GetFileInTempFile(self, uri):
    """Copy a cache file into a tempfile (downloading as needed).

    The cache file is copied into a tempfile.NamedTemporaryFile.

    This file is owned strictly by the caller and can be modified/deleted as
    needed. Closing the NamedTemporaryFile will delete it.

    Args:
      uri: The uri of the file to access.

    Returns:
      tempfile.NamedTemporaryFile containing the requested file.
      NamedTemporaryFile.name will contain the file's name.

    Raises:
      Exceptions from a failed download are passed through 'as is' from
      the underlying download mechanism.
    """
    temp = tempfile.NamedTemporaryFile()
    self.GetFileCopy(uri, temp.name)
    return temp

  # Cache objects can be used with "with" statements.
  def __enter__(self):
    return self

  def __exit__(self, _type, _value, _traceback):
    self.Purge()
