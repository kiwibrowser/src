# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

class MemoryInspectorException(Exception):
  """Base exception class used for all memory inspector related exceptions."""
  def __init__(self, value):
    super(MemoryInspectorException, self).__init__(value)
