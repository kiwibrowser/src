#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from copy import deepcopy
from file_system import FileNotFoundError, StatInfo
from test_file_system import TestFileSystem, MoveTo
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


class TestFileSystemTest(unittest.TestCase):
  def testEmptyFileSystem(self):
    self._TestMetasyntacticPaths(TestFileSystem({}))

  def testNonemptyFileNotFoundErrors(self):
    fs = TestFileSystem(deepcopy(_TEST_DATA))
    self._TestMetasyntacticPaths(fs)
    self.assertRaises(FileNotFoundError, _Get(fs.Read), ['404.html/'])
    self.assertRaises(FileNotFoundError, _Get(fs.Read), ['apps/foo/'])
    self.assertRaises(FileNotFoundError, _Get(fs.Read), ['apps/foo.html'])
    self.assertRaises(FileNotFoundError, _Get(fs.Read), ['apps/foo.html'])
    self.assertRaises(FileNotFoundError, _Get(fs.Read), ['apps/foo/',
                                                         'apps/foo.html'])
    self.assertRaises(FileNotFoundError, _Get(fs.Read), ['apps/foo/',
                                                         'apps/a11y.html'])

  def _TestMetasyntacticPaths(self, fs):
    self.assertRaises(FileNotFoundError, _Get(fs.Read), ['foo'])
    self.assertRaises(FileNotFoundError, _Get(fs.Read), ['bar/'])
    self.assertRaises(FileNotFoundError, _Get(fs.Read), ['bar/baz'])
    self.assertRaises(FileNotFoundError, _Get(fs.Read), ['foo',
                                                         'bar/',
                                                         'bar/baz'])
    self.assertRaises(FileNotFoundError, fs.Stat, 'foo')
    self.assertRaises(FileNotFoundError, fs.Stat, 'bar/')
    self.assertRaises(FileNotFoundError, fs.Stat, 'bar/baz')

  def testNonemptySuccess(self):
    fs = TestFileSystem(deepcopy(_TEST_DATA))
    self.assertEqual('404.html contents', fs.ReadSingle('404.html').Get())
    self.assertEqual('a11y.html contents',
                     fs.ReadSingle('apps/a11y.html').Get())
    self.assertEqual(['404.html', 'apps/', 'extensions/'],
                     sorted(fs.ReadSingle('').Get()))
    self.assertEqual(['a11y.html', 'about_apps.html', 'fakedir/'],
                     sorted(fs.ReadSingle('apps/').Get()))

  def testReadFiles(self):
    fs = TestFileSystem(deepcopy(_TEST_DATA))
    self.assertEqual('404.html contents',
                     fs.ReadSingle('404.html').Get())
    self.assertEqual('a11y.html contents',
                     fs.ReadSingle('apps/a11y.html').Get())
    self.assertEqual('file.html contents',
                     fs.ReadSingle('apps/fakedir/file.html').Get())

  def testReadDirs(self):
    fs = TestFileSystem(deepcopy(_TEST_DATA))
    self.assertEqual(['404.html', 'apps/', 'extensions/'],
                     sorted(fs.ReadSingle('').Get()))
    self.assertEqual(['a11y.html', 'about_apps.html', 'fakedir/'],
                     sorted(fs.ReadSingle('apps/').Get()))
    self.assertEqual(['file.html'], fs.ReadSingle('apps/fakedir/').Get())

  def testStat(self):
    fs = TestFileSystem(deepcopy(_TEST_DATA))
    self.assertRaises(FileNotFoundError, fs.Stat, 'foo')
    self.assertRaises(FileNotFoundError, fs.Stat, '404.html/')
    self.assertEquals(StatInfo('0', child_versions={
                        '404.html': '0',
                        'apps/': '0',
                        'extensions/': '0',
                      }), fs.Stat(''))
    self.assertEquals(StatInfo('0'), fs.Stat('404.html'))
    self.assertEquals(StatInfo('0', child_versions={
                        'activeTab.html': '0',
                        'alarms.html': '0',
                      }), fs.Stat('extensions/'))

    fs.IncrementStat()
    self.assertEquals(StatInfo('1', child_versions={
                        '404.html': '1',
                        'apps/': '1',
                        'extensions/': '1',
                      }), fs.Stat(''))
    self.assertEquals(StatInfo('1'), fs.Stat('404.html'))
    self.assertEquals(StatInfo('1', child_versions={
                        'activeTab.html': '1',
                        'alarms.html': '1',
                      }), fs.Stat('extensions/'))

    fs.IncrementStat(path='404.html')
    self.assertEquals(StatInfo('2', child_versions={
                        '404.html': '2',
                        'apps/': '1',
                        'extensions/': '1',
                      }), fs.Stat(''))
    self.assertEquals(StatInfo('2'), fs.Stat('404.html'))
    self.assertEquals(StatInfo('1', child_versions={
                        'activeTab.html': '1',
                        'alarms.html': '1',
                      }), fs.Stat('extensions/'))

    fs.IncrementStat()
    self.assertEquals(StatInfo('3', child_versions={
                        '404.html': '3',
                        'apps/': '2',
                        'extensions/': '2',
                      }), fs.Stat(''))
    self.assertEquals(StatInfo('3'), fs.Stat('404.html'))
    self.assertEquals(StatInfo('2', child_versions={
                        'activeTab.html': '2',
                        'alarms.html': '2',
                      }), fs.Stat('extensions/'))

    # It doesn't make sense to increment the version of directories. Directory
    # versions are derived from the version of files within them.
    self.assertRaises(ValueError, fs.IncrementStat, path='')
    self.assertRaises(ValueError, fs.IncrementStat, path='extensions/')
    self.assertEquals(StatInfo('3', child_versions={
                        '404.html': '3',
                        'apps/': '2',
                        'extensions/': '2',
                      }), fs.Stat(''))
    self.assertEquals(StatInfo('3'), fs.Stat('404.html'))
    self.assertEquals(StatInfo('2', child_versions={
                        'activeTab.html': '2',
                        'alarms.html': '2',
                      }), fs.Stat('extensions/'))

    fs.IncrementStat(path='extensions/alarms.html')
    self.assertEquals(StatInfo('3', child_versions={
                        '404.html': '3',
                        'apps/': '2',
                        'extensions/': '3',
                      }), fs.Stat(''))
    self.assertEquals(StatInfo('3'), fs.Stat('404.html'))
    self.assertEquals(StatInfo('3', child_versions={
                        'activeTab.html': '2',
                        'alarms.html': '3',
                      }), fs.Stat('extensions/'))

    fs.IncrementStat(path='extensions/activeTab.html', by=3)
    self.assertEquals(StatInfo('5', child_versions={
                        '404.html': '3',
                        'apps/': '2',
                        'extensions/': '5',
                      }), fs.Stat(''))
    self.assertEquals(StatInfo('3'), fs.Stat('404.html'))
    self.assertEquals(StatInfo('5', child_versions={
                        'activeTab.html': '5',
                        'alarms.html': '3',
                      }), fs.Stat('extensions/'))

  def testMoveTo(self):
    self.assertEqual({'foo': {'a': 'b', 'c': 'd'}},
                     MoveTo('foo/', {'a': 'b', 'c': 'd'}))
    self.assertEqual({'foo': {'bar': {'a': 'b', 'c': 'd'}}},
                     MoveTo('foo/bar/', {'a': 'b', 'c': 'd'}))
    self.assertEqual({'foo': {'bar': {'baz': {'a': 'b'}}}},
                     MoveTo('foo/bar/baz/', {'a': 'b'}))


if __name__ == '__main__':
  unittest.main()
