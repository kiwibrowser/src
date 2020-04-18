# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys

from docs_server_utils import StringIdentity
from file_system import FileSystem, FileNotFoundError, StatInfo
from future import Future
from path_util import AssertIsDirectory, AssertIsValid
from test_util import ChromiumPath


def _ConvertToFilepath(path):
  return path.replace('/', os.sep)


def _ConvertFromFilepath(path):
  return path.replace(os.sep, '/')


def _ReadFile(filename):
  try:
    with open(filename, 'rb') as f:
      return f.read()
  except IOError as e:
    raise FileNotFoundError('Read failed for %s: %s' % (filename, e))


def _ListDir(dir_name):
  all_files = []
  try:
    files = os.listdir(dir_name)
  except OSError as e:
    raise FileNotFoundError('os.listdir failed for %s: %s' % (dir_name, e))
  for os_path in files:
    posix_path = _ConvertFromFilepath(os_path)
    if os_path.startswith('.'):
      continue
    if os.path.isdir(os.path.join(dir_name, os_path)):
      all_files.append(posix_path + '/')
    else:
      all_files.append(posix_path)
  return all_files


def _CreateStatInfo(path):
  try:
    path_mtime = os.stat(path).st_mtime
    if os.path.isdir(path):
      child_versions = dict((_ConvertFromFilepath(filename),
                             os.stat(os.path.join(path, filename)).st_mtime)
          for filename in os.listdir(path))
      # This file system stat mimics subversion, where the stat of directories
      # is max(file stats). That means we need to recursively check the whole
      # file system tree :\ so approximate that by just checking this dir.
      version = max([path_mtime] + child_versions.values())
    else:
      child_versions = None
      version = path_mtime
    return StatInfo(version, child_versions)
  except OSError as e:
    raise FileNotFoundError('os.stat failed for %s: %s' % (path, e))


class LocalFileSystem(FileSystem):
  '''FileSystem implementation which fetches resources from the local
  filesystem.
  '''
  def __init__(self, base_path):
    # Enforce POSIX path, so path validity checks pass for Windows.
    base_path = base_path.replace(os.sep, '/')
    AssertIsDirectory(base_path)
    self._base_path = _ConvertToFilepath(base_path)

  @staticmethod
  def Create(*path):
    return LocalFileSystem(ChromiumPath(*path))

  def Read(self, paths, skip_not_found=False):
    def resolve():
      result = {}
      for path in paths:
        AssertIsValid(path)
        full_path = os.path.join(self._base_path,
                                 _ConvertToFilepath(path).lstrip(os.sep))
        if path == '' or path.endswith('/'):
          result[path] = _ListDir(full_path)
        else:
          try:
            result[path] = _ReadFile(full_path)
          except FileNotFoundError:
            if skip_not_found:
              continue
            return Future(exc_info=sys.exc_info())
      return result
    return Future(callback=resolve)

  def Refresh(self):
    return Future(value=())

  def Stat(self, path):
    AssertIsValid(path)
    full_path = os.path.join(self._base_path,
                             _ConvertToFilepath(path).lstrip(os.sep))
    return _CreateStatInfo(full_path)

  def GetIdentity(self):
    return '@'.join((self.__class__.__name__, StringIdentity(self._base_path)))

  def __repr__(self):
    return 'LocalFileSystem(%s)' % self._base_path
