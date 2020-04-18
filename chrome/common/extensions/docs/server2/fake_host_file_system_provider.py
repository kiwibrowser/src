# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from mock_file_system import MockFileSystem
from test_file_system import TestFileSystem
from third_party.json_schema_compiler.memoize import memoize

class FakeHostFileSystemProvider(object):

  def __init__(self, file_system_data):
    self._file_system_data = file_system_data

  def GetMaster(self):
    return self.GetBranch('master')

  @memoize
  def GetBranch(self, branch):
    return MockFileSystem(TestFileSystem(self._file_system_data[str(branch)]))
