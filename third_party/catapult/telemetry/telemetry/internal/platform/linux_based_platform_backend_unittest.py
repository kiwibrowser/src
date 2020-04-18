# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import unittest

from telemetry.core import util
from telemetry.internal.platform import linux_based_platform_backend
import mock


class TestLinuxBackend(linux_based_platform_backend.LinuxBasedPlatformBackend):

  # pylint: disable=abstract-method

  def __init__(self):
    super(TestLinuxBackend, self).__init__()
    self._mock_files = {}

  def SetMockFile(self, filename, output):
    self._mock_files[filename] = output

  def GetFileContents(self, filename):
    return self._mock_files[filename]

  def GetClockTicks(self):
    return 41


class LinuxBasedPlatformBackendTest(unittest.TestCase):

  def SetMockFileInBackend(self, backend, real_file, mock_file):
    with open(os.path.join(util.GetUnittestDataDir(), real_file)) as f:
      backend.SetMockFile(mock_file, f.read())

  def testGetSystemTotalPhysicalMemory(self):
    backend = TestLinuxBackend()
    self.SetMockFileInBackend(backend, 'proc_meminfo', '/proc/meminfo')
    result = backend.GetSystemTotalPhysicalMemory()
    # 67479191552 == MemTotal * 1024
    self.assertEquals(result, 67479191552)

  def testGetCpuStatsBasic(self):
    backend = TestLinuxBackend()
    self.SetMockFileInBackend(backend, 'stat', '/proc/1/stat')
    result = backend.GetCpuStats(1)
    self.assertEquals(result, {'CpuProcessTime': 22.0})

  def testGetCpuTimestampBasic(self):
    jiffies_grep_string = """
    jiffies
jiffies  a1111
    .last_jiffies   : 4307239958
    .next_jiffies   : 4307239968
    jiffies: 10505463300
    jiffies: 10505463333
    """
    with mock.patch.object(
        linux_based_platform_backend.LinuxBasedPlatformBackend,
        'RunCommand', return_value=jiffies_grep_string) as mock_method:
      backend = linux_based_platform_backend.LinuxBasedPlatformBackend()
      result = backend.GetCpuTimestamp()
      self.assertEquals(result, {'TotalTime': 105054633.0})
    mock_method.assert_call_once_with(
        ['grep', '-m', '1', 'jiffies:', '/proc/timer_list'])
