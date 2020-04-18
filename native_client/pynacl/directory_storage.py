#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Implement directory storage on top of file only storage.

Given a storage object capable of storing and retrieving files,
embellish with methods for storing and retrieving directories (using tar).
"""

import collections
import gzip
import os
import posixpath
import subprocess
import sys
import tempfile

import file_tools
import hashing_tools


PYNACL_DIR = os.path.dirname(os.path.abspath(__file__))
NACL_DIR = os.path.dirname(PYNACL_DIR)
BUILD_DIR = os.path.join(NACL_DIR, 'build')
CYGTAR_PATH = os.path.join(BUILD_DIR, 'cygtar.py')


DirectoryStorageItem = collections.namedtuple(
    'DirectoryStorageItem',
    ['name', 'hash', 'url']
)


class DirectoryStorageAdapter(object):
  """Adapter that implements directory storage on top of file storage.

  Tars directories as needed to keep operations on the data-store atomic.
  """

  def __init__(self, storage):
    """Init for this class.

    Args:
      storage: File storage object supporting GetFile and PutFile.
    """
    self._storage = storage

  def PutDirectory(self, path, key, hasher=None):
    """Write a directory to storage.

    Args:
      path: Path of the directory to write.
      key: Key to store under.
    Returns:
      DirectoryStorageItem of the item stored, or None on errors.
    """
    if hasher is None:
      hasher = hashing_tools.HashFileContents

    tar_hnd, tmptar = tempfile.mkstemp(prefix='dirstore', suffix='.tmp.tar')
    tgz_hnd, tmptgz = tempfile.mkstemp(prefix='dirstore', suffix='.tmp.tar.tgz')
    try:
      os.close(tar_hnd)
      os.close(tgz_hnd)
      # Calling cygtar thru subprocess as it's cwd handling is not currently
      # usable.
      subprocess.check_call([sys.executable, CYGTAR_PATH,
                             '-c', '-f', os.path.abspath(tmptar), '.'],
                             cwd=os.path.abspath(path))

      # To make gzip deterministic, modify the timestamp to a constant value.
      with gzip.GzipFile(tmptgz, 'wb', mtime=1000000000) as f_tgz:
        with open(tmptar, 'rb') as f_tar:
          f_tgz.write(f_tar.read())

      url = self._storage.PutFile(tmptgz, key)

      name = posixpath.basename(key)
      hash_value = hasher(tmptgz)

      return DirectoryStorageItem(name, hash_value, url)
    finally:
      os.remove(tmptar)
      os.remove(tmptgz)

  def GetDirectory(self, key, path, hasher=None):
    """Read a directory from storage.

    Clobbers anything at the destination currently.
    Args:
      key: Key to fetch from.
      path: Path of the directory to write.
    Returns:
      DirectoryStorageItem of item retrieved, or None on errors.
    """
    if hasher is None:
      hasher = hashing_tools.HashFileContents

    file_tools.RemoveDirectoryIfPresent(path)
    os.mkdir(path)
    handle, tmp_tgz = tempfile.mkstemp(prefix='dirstore', suffix='.tmp.tgz')
    try:
      os.close(handle)
      url = self._storage.GetFile(key, tmp_tgz)
      if url is None:
        return None
      # Calling cygtar thru subprocess as it's cwd handling is not currently
      # usable.
      subprocess.check_call([sys.executable, CYGTAR_PATH,
                             '-x', '-z', '-f', os.path.abspath(tmp_tgz)],
                             cwd=os.path.abspath(path))

      name = posixpath.basename(key)
      hash_value = hasher(tmp_tgz)

      return DirectoryStorageItem(name, hash_value, url)
    finally:
      os.remove(tmp_tgz)
