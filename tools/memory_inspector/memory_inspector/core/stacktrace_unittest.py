# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from memory_inspector.core import stacktrace


class StacktraceTest(unittest.TestCase):
  def runTest(self):
    st = stacktrace.Stacktrace()
    frame_1 = stacktrace.Frame(20)
    frame_2 = stacktrace.Frame(24)
    frame_2.SetExecFileInfo('/foo/bar.so', 0)
    self.assertEqual(frame_2.exec_file_name, 'bar.so')
    st.Add(frame_1)
    st.Add(frame_1)
    st.Add(frame_2)
    st.Add(frame_1)
    self.assertEqual(st.depth, 4)
