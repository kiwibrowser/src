# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for the android backends modules."""

import unittest

from memory_inspector.backends import adb_client
from memory_inspector.backends import android_backend
from memory_inspector.backends import prebuilts_fetcher


_MOCK_DEVICES_OUT = """List of devices attached
0000000000000001\tdevice
0000000000000002\tdevice
"""

_MOCK_MEMDUMP_OUT = ("""[ PID=1]
8000-33000 r-xp 0 private_unevictable=8192 private=8192 shared_app=[] """
    """shared_other_unevictable=147456 shared_other=147456 "/init" [v///fv0D]
33000-35000 r--p 2a000 private_unevictable=4096 private=4096 shared_app=[] """
    """shared_other_unevictable=4096 shared_other=4096 "/init" [Aw==]
35000-37000 rw-p 2c000 private_unevictable=8192 private=8192 shared_app=[] """
    """shared_other_unevictable=0 shared_other=0 "/init" [Aw==]
37000-6d000 rw-p 0 private_unevictable=221184 private=221184 shared_app=[] """
    """shared_other_unevictable=0 shared_other=0 "[heap]" [////////Pw==]
400c5000-400c7000 rw-p 0 private_unevictable=0 private=0 shared_app=[] """
    """shared_other_unevictable=0 shared_other=0 "" [AA==]
400f4000-400f5000 r--p 0 private_unevictable=4096 private=4096 shared_app=[] """
    """shared_other_unevictable=0 shared_other=0 "" [AQ==]
40127000-40147000 rw-s 0 private_unevictable=4096 private=4096 shared_app=[] """
    """shared_other_unevictable=32768 shared_other=32768 "/X" [/////w==]
be8b7000-be8d8000 rw-p 8 private_unevictable=8192 private=8192 shared_app=[] """
    """shared_other_unevictable=4096 shared_other=12288 "[stack]" [//8=]
ffff00ffff0000-ffff00ffff1000 r-xp 0 private_unevictable=0 private=0 """
    """shared_app=[] shared_other_unevictable=0 shared_other=0 "[vectors]" []"""
)

_MOCK_HEAP_DUMP_OUT = """{
  "total_allocated": 8192,
  "num_allocs":      2,
  "num_stacks":      2,
  "allocs": {
   "a1000": {"l": 100, "f": 0, "s": "a"},
   "a2000": {"l": 100, "f": 0, "s": "a"},
   "b1000": {"l": 1000, "f": 0, "s": "b"},
   "b2000": {"l": 1000, "f": 0, "s": "b"},
   "b3000": {"l": 1000, "f": 0, "s": "b"}},
  "stacks": {
    "a": {"l": 200,  "f": [10, 20, 30, 40]},
    "b": {"l": 3000, "f": [50, 60, 70]}}
  }"""

_MOCK_PS_EXT_OUT = """
{
  "time": { "ticks": 1000100, "rate": 100},
  "mem": {
      "MemTotal:": 998092, "MemFree:": 34904,
      "Buffers:": 24620, "Cached:": 498916 },
  "cpu": [{"usr": 32205, "sys": 49826, "idle": 7096196}],
  "processes": {
    "1": {
       "name": "foo", "n_threads": 42, "start_time": 1000000, "user_time": 82,
       "sys_time": 445, "min_faults": 0, "maj_faults": 0, "vm_rss": 528},
    "2": {
       "name": "bar", "n_threads": 142, "start_time": 1, "user_time": 82,
       "sys_time": 445, "min_faults": 0, "maj_faults": 0, "vm_rss": 528}}
}
"""


class MockADBDevice(object):
  """A Mock for adb_client.py."""
  _PROPS = {'ro.product.model': 'Mock device',
            'ro.build.type': 'userdebug',
            'ro.build.id': 'ZZ007',
            'ro.product.cpu.abi': 'armeabi'}

  def __init__(self, serial):
    self.serial = serial

  def GetProp(self, name, cached=False):  # pylint: disable=W0613
    return MockADBDevice._PROPS[name]

  def Shell(self, cmd):
    if 'getprop' in cmd:
      return '\n'.join('[%s]: [%s]' % (k, v)
                       for k, v in MockADBDevice._PROPS.iteritems())
    if '/data/local/tmp/ps_ext' in cmd:
      return _MOCK_PS_EXT_OUT
    if '/data/local/tmp/memdump' in cmd:
      return _MOCK_MEMDUMP_OUT
    if '/data/local/tmp/heap_dump' in cmd:
      return _MOCK_HEAP_DUMP_OUT
    assert False, 'Not reached'

  def RestartShellAsRoot(self):
    pass

  def WaitForDevice(self):
    pass

  def Push(self, host_path, device_path):  # pylint: disable=W0613
    pass


def MockListDevices():
  yield MockADBDevice('0000000000000001')
  yield MockADBDevice('0000000000000002')


class AndroidBackendTest(unittest.TestCase):

  def setUp(self):
    prebuilts_fetcher.in_test_harness = True
    adb_client.ListDevices = MockListDevices  # Sets up the ADBClient mock.

  def runTest(self):
    ab = android_backend.AndroidBackend()

    # Test settings load/store logic.
    self.assertTrue('toolchain_path' in ab.settings.expected_keys)
    ab.settings['toolchain_path'] = 'foo'
    self.assertEqual(ab.settings['toolchain_path'], 'foo')

    # Test device enumeration.
    devices = list(ab.EnumerateDevices())
    self.assertEqual(len(devices), 2)
    self.assertEqual(devices[0].name, 'Mock device ZZ007')
    self.assertEqual(devices[0].id, '0000000000000001')
    self.assertEqual(devices[1].name, 'Mock device ZZ007')
    self.assertEqual(devices[1].id, '0000000000000002')

    # Initialize device (checks that sha1 are checked in).
    device = devices[0]
    device.Initialize()

    # Test process enumeration.
    processes = list(device.ListProcesses())
    self.assertEqual(len(processes), 2)
    self.assertEqual(processes[0].pid, 1)
    self.assertEqual(processes[0].name, 'foo')
    self.assertEqual(processes[1].pid, 2)
    self.assertEqual(processes[1].name, 'bar')

    # Test process stats.
    stats = processes[0].GetStats()
    self.assertEqual(stats.threads, 42)
    self.assertEqual(stats.cpu_usage, 0)
    self.assertEqual(stats.run_time, 1)
    self.assertEqual(stats.vm_rss, 528)
    self.assertEqual(stats.page_faults, 0)

    # Test memdump parsing.
    mmaps = processes[0].DumpMemoryMaps()
    self.assertEqual(len(mmaps), 9)
    self.assertIsNone(mmaps.Lookup(0))
    self.assertIsNone(mmaps.Lookup(7999))
    self.assertIsNotNone(mmaps.Lookup(0x8000))
    self.assertIsNotNone(mmaps.Lookup(0x32FFF))
    self.assertIsNotNone(mmaps.Lookup(0x33000))
    self.assertIsNone(mmaps.Lookup(0x6d000))
    self.assertIsNotNone(mmaps.Lookup(0xffff00ffff0000L))
    self.assertIsNone(mmaps.Lookup(0xffff0000))

    entry = mmaps.Lookup(0xbe8b7000)
    self.assertIsNotNone(entry)
    self.assertEqual(entry.start, 0xbe8b7000)
    self.assertEqual(entry.end, 0xbe8d7fff)
    self.assertEqual(entry.mapped_offset, 0x8)
    self.assertEqual(entry.prot_flags, 'rw-p')
    self.assertEqual(entry.priv_dirty_bytes, 8192)
    self.assertEqual(entry.priv_clean_bytes, 0)
    self.assertEqual(entry.shared_dirty_bytes, 4096)
    self.assertEqual(entry.shared_clean_bytes, 8192)
    for i in xrange(16):
      self.assertTrue(entry.IsPageResident(i))
    for i in xrange(16, 33):
      self.assertFalse(entry.IsPageResident(i))

    # Test heap_dump parsing.
    heap = processes[0].DumpNativeHeap()
    self.assertEqual(len(heap.allocations), 5)

    for alloc in heap.allocations:
      self.assertTrue(alloc.size == 100 or alloc.size == 1000)
      if alloc.size == 100:
        self.assertEqual(alloc.size, 100)
        self.assertEqual(alloc.stack_trace.depth, 4)
        self.assertEqual([x.address for x in alloc.stack_trace.frames],
                         [10, 20, 30, 40])
      elif alloc.size == 1000:
        self.assertEqual(alloc.size, 1000)
        self.assertEqual(alloc.stack_trace.depth, 3)
        self.assertEqual([x.address for x in alloc.stack_trace.frames],
                         [50, 60, 70])
