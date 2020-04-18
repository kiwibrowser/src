# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from file_system import FileSystem

class FailOnAccessFileSystem(FileSystem):
  # All this needs to do is implement GetIdentity. All other methods will
  # automatically fail with NotImplementedErrors.
  def GetIdentity(self):
    return '42'
