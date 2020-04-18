# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from future import Future
from patcher import Patcher

class TestPatcher(Patcher):
  def __init__(self, version, patched_files, patch_data):
    self._version = version
    self._patched_files = patched_files
    self._patch_data = patch_data

    self.get_version_count = 0
    self.get_patched_files_count = 0
    self.apply_count = 0

  def GetVersion(self):
    self.get_version_count += 1
    return self._version

  def GetPatchedFiles(self, version=None):
    self.get_patched_files_count += 1
    return self._patched_files

  def Apply(self, paths, file_system, version=None):
    self.apply_count += 1
    try:
      return Future(value=dict((path, self._patch_data[path])
                               for path in paths))
    except KeyError:
      raise FileNotFoundError('One of %s is deleted in the patch.' % paths)

  def GetIdentity(self):
    return self.__class__.__name__
