# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from file_system import FileNotFoundError, FileSystem, StatInfo
from future import Future
from path_util import IsDirectory


class EmptyDirFileSystem(FileSystem):
  '''A FileSystem with empty directories. Useful to inject places to disable
  features such as samples.
  '''
  def Read(self, paths, skip_not_found=False):
    result = {}
    for path in paths:
      if not IsDirectory(path):
        if skip_not_found: continue
        raise FileNotFoundError('EmptyDirFileSystem cannot read %s' % path)
      result[path] = []
    return Future(value=result)

  def Refresh(self):
    return Future(value=())

  def Stat(self, path):
    if not IsDirectory(path):
      raise FileNotFoundError('EmptyDirFileSystem cannot stat %s' % path)
    return StatInfo(0, child_versions=[])

  def GetIdentity(self):
    return self.__class__.__name__
