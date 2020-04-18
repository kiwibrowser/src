# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from file_system import FileSystem, FileNotFoundError
from future import Future


class OfflineFileSystem(FileSystem):
  '''An offline FileSystem which masquerades as another file system. It throws
  FileNotFound error for all operations, and overrides GetIdentity.
  '''
  def __init__(self, fs):
    self._fs = fs

  def Read(self, paths, skip_not_found=False):
    if skip_not_found: return Future(value={})
    def raise_file_not_found():
      raise FileNotFoundError('File system is offline, cannot read %s' % paths)
    return Future(callback=raise_file_not_found)

  def Stat(self, path):
    raise FileNotFoundError('File system is offline, cannot read %s' % path)

  def GetIdentity(self):
    return self._fs.GetIdentity()

  def GetVersion(self):
    return self._fs.GetVersion()
