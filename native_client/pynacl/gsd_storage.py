#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Provide Google Storage access.

Provide an high-level interface to Google Storage.
Operations are provided to read/write whole files and to
read/write strings. This allows google storage to be treated
more or less like a key+value data-store.
"""


import logging
import os
import posixpath
import random
import re
import shutil
import subprocess
import string
import sys
import tempfile

import file_tools
import http_download
import platform


GS_PATTERN = 'gs://%s'
GS_HTTPS_PATTERN = 'https://storage.googleapis.com/%s'


def LegalizeName(name):
  """ Return a file name suitable for uploading to Google Storage.

  The names of such files cannot contain dashes or other non-identifier
  characters.
  """
  return re.sub(r'[^A-Za-z0-9_/.]', '_', name)


def HttpDownload(url, target):
  """Default download route."""
  http_download.HttpDownload(url, os.path.abspath(target), verbose=False,
                             logger=logging.debug)


class GSDStorageError(Exception):
  """Error indicating writing to storage failed."""
  pass


class GSDStorage(object):
  """A wrapper for reading and writing to GSD buckets.

  Multiple read buckets may be specified, and the wrapper will sequentially try
  each and fall back to the next if the previous fails.
  Writing is to a single bucket.
  """
  def __init__(self,
               write_bucket,
               read_buckets,
               gsutil=None,
               call=subprocess.call,
               download=HttpDownload):
    """Init for this class.

    Args:
      write_bucket: Google storage location to write to.
      read_buckets: Google storage locations to read from in preferred order.
      gsutil: List of cmd components needed to call gsutil.
      call: Testing hook to intercept command invocation.
      download: Testing hook to intercept download.
    """
    if gsutil is None:
      gsutil_script = platform.CygPath(os.environ.get('GSUTIL', 'gsutil'))
      try:
        # Require that gsutil be Python if it is specified in the environment.
        gsutil = [sys.executable,
                  file_tools.Which(gsutil_script,
                                   require_executable=False)]
      except file_tools.ExecutableNotFound:
        gsutil = [gsutil_script]
    assert isinstance(gsutil, list)
    assert isinstance(read_buckets, list)
    self._gsutil = gsutil
    self._write_bucket = write_bucket
    self._read_buckets = read_buckets
    self._call = call
    self._download = download

  def Exists(self, key):
    """Queries whether or not a key exists.

    Args:
      key: Key file is stored under.
    Returns:
      URL of existing key, or False if file does not exist.
    """
    for bucket in set(self._read_buckets + [self._write_bucket]):
      obj = posixpath.join(bucket, key)
      cmd = self._gsutil + ['ls', GS_PATTERN % obj]
      logging.info('Running: %s', str(cmd))
      if self._call(cmd) == 0:
        return GS_HTTPS_PATTERN % obj

    return False

  def PutFile(self, path, key, clobber=True):
    """Write a file to global storage.

    Args:
      path: Path of the file to write.
      key: Key to store file under.
    Raises:
      GSDStorageError if the underlying storage fails.
    Returns:
      URL written to.
    """
    if self._write_bucket is None:
      raise GSDStorageError('no bucket when storing %s to %s' % (path, key))
    obj = self._write_bucket + '/' + key
    cp_cmd = ['cp', '-a', 'public-read']
    if not clobber:
      cp_cmd.append('-n')

    gs_path = GS_PATTERN % obj
    gs_tmp_path = gs_path + '.tmp.' + ''.join(
        random.choice(string.lowercase) for x in range(10))
    # Using file://c:/foo/bar form of path as gsutil does not like drive
    # letters without it.
    file_path ='file://' + os.path.abspath(path).replace(os.sep, '/')

    # Store to temporary location.
    cmd = self._gsutil + cp_cmd + [file_path, gs_tmp_path]
    logging.info('Running: %s' % str(cmd))
    if self._call(cmd) != 0:
      raise GSDStorageError('failed when storing %s to %s (%s)' % (
        path, key, cmd))

    # Copy to final location (so the window of time is short).
    cmd = self._gsutil + cp_cmd + [gs_tmp_path, gs_path]
    logging.info('Running: %s' % str(cmd))
    if self._call(cmd) != 0:
      raise GSDStorageError('failed when storing %s to %s (%s)' % (
        path, key, cmd))

    # Cleanup.
    cmd = self._gsutil + ['rm', gs_tmp_path]
    logging.info('Running: %s' % str(cmd))
    if self._call(cmd) != 0:
      raise GSDStorageError('failed when storing %s to %s (%s)' % (
        path, key, cmd))

    return GS_HTTPS_PATTERN % obj

  def PutData(self, data, key, clobber=True):
    """Write data to global storage.

    Args:
      data: Data to store.
      key: Key to store file under.
    Raises:
      GSDStorageError if the underlying storage fails.
    Returns:
      URL written to.
    """
    handle, path = tempfile.mkstemp(prefix='gdstore', suffix='.tmp')
    try:
      os.close(handle)
      file_tools.WriteFile(data, path)
      return self.PutFile(path, key, clobber=clobber)
    finally:
      os.remove(path)

  def GetFile(self, key, path):
    """Read a file from global storage.

    Args:
      key: Key to store file under.
      path: Destination filename.
    Returns:
      URL used on success or None for failure.
    """
    for bucket in self._read_buckets:
      try:
        obj = bucket + '/' + key
        uri = GS_HTTPS_PATTERN % obj
        logging.debug('Downloading: %s to %s' % (uri, path))
        self._download(uri, path)
        return uri
      except:
        logging.debug('Failed downloading: %s to %s' % (uri, path))
    return None

  def GetSecureFile(self, key, path):
    """ Read a non-publicly-accessible file from global storage.

    Args:
      key: Key file is stored under.
      path: Destination filename
    Returns:
      command used on success or None on failure.
    """
    for bucket in self._read_buckets:
      try:
        obj = bucket + '/' + key
        cmd = self._gsutil + [
            'cp', GS_PATTERN % obj,
            'file://' + os.path.abspath(path).replace(os.sep, '/')]
        logging.info('Running: %s' % str(cmd))
        if self._call(cmd) == 0:
          return cmd
      except:
        logging.debug('Failed to fetch %s from %s (%s)' % (key, path, cmd))
    return None

  def GetData(self, key):
    """Read data from global storage.

    Args:
      key: Key to store file under.
    Returns:
      Data from storage, or None for failure.
    """
    work_dir = tempfile.mkdtemp(prefix='gdstore', suffix='.tmp')
    try:
      path = os.path.join(work_dir, 'data')
      if self.GetFile(key, path) is not None:
        return file_tools.ReadFile(path)
      return None
    finally:
      shutil.rmtree(work_dir)
