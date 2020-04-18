#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Cache accesses to GSDStorage locally.

Operations are provided to read/write whole files and to
read/write strings.
Read from GSDStorage if nothing exists locally.
"""

import os
import re

import file_tools


KEY_PATTERN = re.compile('^[A-Za-z0-9_/.]+$')


def ValidateKey(key):
  if KEY_PATTERN.match(key) is None:
    raise KeyError('Invalid storage key "%s"' % key)

def LocalFileURL(local_file):
  abs_path = os.path.abspath(local_file)
  if not abs_path.startswith('/'):
    # Windows paths needs an extra slash for the file protocol.
    return 'file:///' + abs_path
  else:
    return 'file://' + abs_path

class LocalStorageCache(object):
  """A caching wrapper for reading a GSDStorage object or storing locally.

  Allow reading/writing to key, value pairs in local files.
  Reads fall back to remote storage.
  Restricts keys to a limited regex.
  Is not atomic in the face of concurrent writers / readers on Windows.
  """
  def __init__(self, cache_path, storage):
    """Init for this class.

    Args:
      cache_path: Path to a database to store a local cache in.
      storage: A GSDStorage style object to fallback to for reads.
    """
    self._cache_path = os.path.abspath(cache_path)
    file_tools.MakeDirectoryIfAbsent(self._cache_path)
    self._storage = storage

  def Exists(self, key):
    """Queries whether or not a key exists.

    Args:
      key: Key file is stored under.
    Returns:
      URL of existing key, or False if file does not exist.
    """
    ValidateKey(key)
    cache_file = os.path.join(self._cache_path, key)
    if os.path.exists(cache_file):
      return LocalFileURL(cache_file)
    return False

  def PutFile(self, path, key):
    """Write a file to storage.

    Args:
      path: Path of the file to write.
      key: Key to store file under.
    Returns:
      URL written to.
    """
    return self.PutData(file_tools.ReadFile(path), key)

  def PutData(self, data, key):
    """Write data to storage.

    Args:
      data: Data to store.
      key: Key to store file under.
    Returns:
      URL written to.
    """
    ValidateKey(key)
    cache_file = os.path.join(self._cache_path, key)
    cache_dir = os.path.dirname(cache_file)
    if not os.path.exists(cache_dir):
      os.makedirs(cache_dir)
    file_tools.AtomicWriteFile(data, cache_file)
    return LocalFileURL(cache_file)

  def GetFile(self, key, path):
    """Read a file from storage.

    Args:
      key: Key to store file under.
      path: Destination filename.
    Returns:
      URL used on success or None for failure.
    """
    ValidateKey(key)
    cache_file = os.path.join(self._cache_path, key)
    if os.path.exists(cache_file):
      data = file_tools.ReadFile(cache_file)
      file_tools.WriteFile(data, path)
      return LocalFileURL(cache_file)
    else:
      return self._storage.GetFile(key, path)

  def GetData(self, key):
    """Read data from global storage.

    Args:
      key: Key to store file under.
    Returns:
      Data from storage, or None for failure.
    """
    ValidateKey(key)
    cache_file = os.path.join(self._cache_path, key)
    if os.path.exists(cache_file):
      return file_tools.ReadFile(cache_file)
    else:
      return self._storage.GetData(key)
