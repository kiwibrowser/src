#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from copy import deepcopy
import unittest

from file_system import FileNotFoundError, StatInfo
from patched_file_system import PatchedFileSystem
from test_file_system import TestFileSystem
from test_patcher import TestPatcher

_TEST_FS_DATA = {
  'dir1': {
    'file1.html': 'This is dir1/file1.html',
    'unmodified': {
      '1': '1',
      '2': '',
    },
  },
  'dir2': {
    'subdir1': {
      'sub1.txt': 'in subdir(1)',
      'sub2.txt': 'in subdir(2)',
      'sub3.txt': 'in subdir(3)',
    },
  },
  'dir3': {
  },
  'dir4': {
    'one.txt': '',
  },
  'dir5': {
    'subdir': {
      '1.txt': '555',
    },
  },
  'test1.txt': 'test1',
  'test2.txt': 'test2',
}

_TEST_PATCH_VERSION = '1001'
_TEST_PATCH_FILES = (
  # Added
  [
    'test3.txt',
    'dir1/file2.html',
    'dir1/newsubdir/a.js',
    'newdir/1.html',
  ],
  # Deleted
  [
    'test2.txt',
    'dir2/subdir1/sub1.txt',
    'dir4/one.txt',
    'dir5/subdir/1.txt',
  ],
  # Modified
  [
    'dir2/subdir1/sub2.txt',
  ]
)
_TEST_PATCH_DATA = {
  'test3.txt': 'test3 is added.',
  'dir1/file2.html': 'This is dir1/file2.html',
  'dir1/newsubdir/a.js': 'This is a.js',
  'newdir/1.html': 'This comes from a new dir.',
  'dir2/subdir1/sub2.txt': 'in subdir',
}

class PatchedFileSystemTest(unittest.TestCase):
  def setUp(self):
    self._patcher = TestPatcher(_TEST_PATCH_VERSION,
                                _TEST_PATCH_FILES,
                                _TEST_PATCH_DATA)
    self._host_file_system = TestFileSystem(_TEST_FS_DATA)
    self._file_system = PatchedFileSystem(self._host_file_system,
                                          self._patcher)

  def testRead(self):
    expected = deepcopy(_TEST_PATCH_DATA)
    # Files that are not modified.
    expected.update({
      'dir2/subdir1/sub3.txt': 'in subdir(3)',
      'dir1/file1.html': 'This is dir1/file1.html',
    })

    for key in expected:
      self.assertEqual(expected[key], self._file_system.ReadSingle(key).Get())

    self.assertEqual(
        expected,
        self._file_system.Read(expected.keys()).Get())

    self.assertRaises(FileNotFoundError,
                      self._file_system.ReadSingle('test2.txt').Get)
    self.assertRaises(FileNotFoundError,
                      self._file_system.ReadSingle('dir2/subdir1/sub1.txt').Get)
    self.assertRaises(FileNotFoundError,
                      self._file_system.ReadSingle('not_existing').Get)
    self.assertRaises(FileNotFoundError,
                      self._file_system.ReadSingle('dir1/not_existing').Get)
    self.assertRaises(
        FileNotFoundError,
        self._file_system.ReadSingle('dir1/newsubdir/not_existing').Get)

  def testReadDir(self):
    self.assertEqual(
        sorted(self._file_system.ReadSingle('dir1/').Get()),
        sorted(set(self._host_file_system.ReadSingle('dir1/').Get()) |
               set(('file2.html', 'newsubdir/'))))

    self.assertEqual(
        sorted(self._file_system.ReadSingle('dir1/newsubdir/').Get()),
        sorted(['a.js']))

    self.assertEqual(sorted(self._file_system.ReadSingle('dir2/').Get()),
                     sorted(self._host_file_system.ReadSingle('dir2/').Get()))

    self.assertEqual(
        sorted(self._file_system.ReadSingle('dir2/subdir1/').Get()),
        sorted(set(self._host_file_system.ReadSingle('dir2/subdir1/').Get()) -
               set(('sub1.txt',))))

    self.assertEqual(sorted(self._file_system.ReadSingle('newdir/').Get()),
                     sorted(['1.html']))

    self.assertEqual(self._file_system.ReadSingle('dir3/').Get(), [])

    self.assertEqual(self._file_system.ReadSingle('dir4/').Get(), [])

    self.assertRaises(FileNotFoundError,
                      self._file_system.ReadSingle('not_existing_dir/').Get)

  def testStat(self):
    version = 'patched_%s' % self._patcher.GetVersion()
    old_version = self._host_file_system.Stat('dir1/file1.html').version

    # Stat an unmodified file.
    self.assertEqual(self._file_system.Stat('dir1/file1.html'),
                     self._host_file_system.Stat('dir1/file1.html'))

    # Stat an unmodified directory.
    self.assertEqual(self._file_system.Stat('dir1/unmodified/'),
                     self._host_file_system.Stat('dir1/unmodified/'))

    # Stat a modified directory.
    self.assertEqual(self._file_system.Stat('dir2/'),
                     StatInfo(version, {'subdir1/': version}))
    self.assertEqual(self._file_system.Stat('dir2/subdir1/'),
                     StatInfo(version, {'sub2.txt': version,
                                        'sub3.txt': old_version}))

    # Stat a modified directory with new files.
    expected = self._host_file_system.Stat('dir1/')
    expected.version = version
    expected.child_versions.update({'file2.html': version,
                                    'newsubdir/': version})
    self.assertEqual(self._file_system.Stat('dir1/'),
                     expected)

    # Stat an added directory.
    self.assertEqual(self._file_system.Stat('dir1/newsubdir/'),
                     StatInfo(version, {'a.js': version}))
    self.assertEqual(self._file_system.Stat('dir1/newsubdir/a.js'),
                     StatInfo(version))
    self.assertEqual(self._file_system.Stat('newdir/'),
                     StatInfo(version, {'1.html': version}))
    self.assertEqual(self._file_system.Stat('newdir/1.html'),
                     StatInfo(version))

    # Stat files removed in the patch.
    self.assertRaises(FileNotFoundError, self._file_system.Stat,
        'dir2/subdir1/sub1.txt')
    self.assertRaises(FileNotFoundError, self._file_system.Stat,
        'dir4/one.txt')

    # Stat empty directories.
    self.assertEqual(self._file_system.Stat('dir3/'),
                     StatInfo(old_version, {}))
    self.assertEqual(self._file_system.Stat('dir4/'),
                     StatInfo(version, {}))
    self.assertEqual(self._file_system.Stat('dir5/subdir/'),
                     StatInfo(version, {}))

    # Stat empty (after patch) directory's parent
    self.assertEqual(self._file_system.Stat('dir5/'),
                     StatInfo(version, {'subdir/': version}))

    # Stat files that don't exist either before or after patching.
    self.assertRaises(FileNotFoundError, self._file_system.Stat,
        'not_existing/')
    self.assertRaises(FileNotFoundError, self._file_system.Stat,
        'dir1/not_existing/')
    self.assertRaises(FileNotFoundError, self._file_system.Stat,
        'dir1/not_existing')

if __name__ == '__main__':
  unittest.main()
