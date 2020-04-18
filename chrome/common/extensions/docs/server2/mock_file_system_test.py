#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from copy import deepcopy
from file_system import FileNotFoundError, StatInfo
from mock_file_system import MockFileSystem
from test_file_system import TestFileSystem
import unittest

_TEST_DATA = {
  '404.html': '404.html contents',
  'apps': {
    'a11y.html': 'a11y.html contents',
    'about_apps.html': 'about_apps.html contents',
    'fakedir': {
      'file.html': 'file.html contents'
    }
  },
  'extensions': {
    'activeTab.html': 'activeTab.html contents',
    'alarms.html': 'alarms.html contents'
  }
}

def _Get(fn):
  '''Returns a function which calls Future.Get on the result of |fn|.
  '''
  return lambda *args: fn(*args).Get()

class MockFileSystemTest(unittest.TestCase):
  def testCheckAndReset(self):
    fs = MockFileSystem(TestFileSystem(deepcopy(_TEST_DATA)))

    self.assertTrue(*fs.CheckAndReset())
    self.assertFalse(*fs.CheckAndReset(read_count=1))
    self.assertFalse(*fs.CheckAndReset(stat_count=1))

    future = fs.ReadSingle('apps/')
    self.assertTrue(*fs.CheckAndReset(read_count=1))
    future.Get()
    self.assertTrue(*fs.CheckAndReset(read_resolve_count=1))
    self.assertFalse(*fs.CheckAndReset(read_count=1))
    self.assertTrue(*fs.CheckAndReset())

    future = fs.ReadSingle('apps/')
    self.assertFalse(*fs.CheckAndReset(read_count=2))
    future.Get()
    self.assertFalse(*fs.CheckAndReset(read_resolve_count=2))

    fs.ReadSingle('extensions/').Get()
    fs.ReadSingle('extensions/').Get()
    self.assertTrue(*fs.CheckAndReset(read_count=2, read_resolve_count=2))
    self.assertFalse(*fs.CheckAndReset(read_count=2, read_resolve_count=2))
    self.assertTrue(*fs.CheckAndReset())

    fs.ReadSingle('404.html').Get()
    self.assertTrue(*fs.CheckAndReset(read_count=1, read_resolve_count=1))
    future = fs.Read(['notfound.html', 'apps/'])
    self.assertTrue(*fs.CheckAndReset(read_count=1))
    self.assertRaises(FileNotFoundError, future.Get)
    self.assertTrue(*fs.CheckAndReset(read_resolve_count=0))

    fs.Stat('404.html')
    fs.Stat('404.html')
    fs.Stat('apps/')
    self.assertFalse(*fs.CheckAndReset(stat_count=42))
    self.assertFalse(*fs.CheckAndReset(stat_count=42))
    self.assertTrue(*fs.CheckAndReset())

    fs.ReadSingle('404.html').Get()
    fs.Stat('404.html')
    fs.Stat('apps/')
    self.assertTrue(
        *fs.CheckAndReset(read_count=1, read_resolve_count=1, stat_count=2))
    self.assertTrue(*fs.CheckAndReset())

  def testUpdates(self):
    fs = MockFileSystem(TestFileSystem(deepcopy(_TEST_DATA)))

    self.assertEqual(StatInfo('0', child_versions={
      '404.html': '0',
      'apps/': '0',
      'extensions/': '0'
    }), fs.Stat(''))
    self.assertEqual(StatInfo('0'), fs.Stat('404.html'))
    self.assertEqual(StatInfo('0', child_versions={
      'a11y.html': '0',
      'about_apps.html': '0',
      'fakedir/': '0',
    }), fs.Stat('apps/'))
    self.assertEqual('404.html contents', fs.ReadSingle('404.html').Get())

    fs.Update({
      '404.html': 'New version!'
    })

    self.assertEqual(StatInfo('1', child_versions={
      '404.html': '1',
      'apps/': '0',
      'extensions/': '0'
    }), fs.Stat(''))
    self.assertEqual(StatInfo('1'), fs.Stat('404.html'))
    self.assertEqual(StatInfo('0', child_versions={
      'a11y.html': '0',
      'about_apps.html': '0',
      'fakedir/': '0',
    }), fs.Stat('apps/'))
    self.assertEqual('New version!', fs.ReadSingle('404.html').Get())

    fs.Update({
      '404.html': 'Newer version!',
      'apps': {
        'fakedir': {
          'file.html': 'yo'
        }
      }
    })

    self.assertEqual(StatInfo('2', child_versions={
      '404.html': '2',
      'apps/': '2',
      'extensions/': '0'
    }), fs.Stat(''))
    self.assertEqual(StatInfo('2'), fs.Stat('404.html'))
    self.assertEqual(StatInfo('2', child_versions={
      'a11y.html': '0',
      'about_apps.html': '0',
      'fakedir/': '2',
    }), fs.Stat('apps/'))
    self.assertEqual(StatInfo('0'), fs.Stat('apps/a11y.html'))
    self.assertEqual(StatInfo('2', child_versions={
      'file.html': '2'
    }), fs.Stat('apps/fakedir/'))
    self.assertEqual(StatInfo('2'), fs.Stat('apps/fakedir/file.html'))
    self.assertEqual(StatInfo('0', child_versions={
      'activeTab.html': '0',
      'alarms.html': '0'
    }), fs.Stat('extensions/'))
    self.assertEqual('Newer version!', fs.ReadSingle('404.html').Get())
    self.assertEqual('yo', fs.ReadSingle('apps/fakedir/file.html').Get())

if __name__ == '__main__':
  unittest.main()
