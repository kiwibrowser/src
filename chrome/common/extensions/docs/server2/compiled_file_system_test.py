#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import functools
import os

from compiled_file_system import Cache, CompiledFileSystem
from copy import deepcopy
from environment import GetAppVersion
from file_system import FileNotFoundError
from mock_file_system import MockFileSystem
from object_store_creator import ObjectStoreCreator
from test_file_system import TestFileSystem
from test_object_store import TestObjectStore
import unittest

_TEST_DATA = {
  '404.html': '404.html contents',
  'apps': {
    'a11y.html': 'a11y.html contents',
    'about_apps.html': 'about_apps.html contents',
    'fakedir': {
      'file.html': 'file.html contents'
    },
    'deepdir': {
      'deepfile.html': 'deepfile.html contents',
      'deeper': {
        'deepest.html': 'deepest.html contents',
      },
    }
  },
  'extensions': {
    'activeTab.html': 'activeTab.html contents',
    'alarms.html': 'alarms.html contents'
  }
}

identity = lambda _, x: x

def _GetTestCompiledFsCreator():
  '''Returns a function which creates CompiledFileSystem views of
  TestFileSystems backed by _TEST_DATA.
  '''
  return functools.partial(
      CompiledFileSystem.Factory(
          ObjectStoreCreator(start_empty=False,
                             store_type=TestObjectStore,
                             disable_wrappers=True),
      ).Create,
      TestFileSystem(deepcopy(_TEST_DATA)))

class CompiledFileSystemTest(unittest.TestCase):
  def testPopulateNamespace(self):
    def CheckNamespace(expected_file, expected_list, fs):
      self.assertEqual(expected_file, fs._file_object_store.namespace)
      self.assertEqual(expected_list, fs._list_object_store.namespace)
    compiled_fs_creator = _GetTestCompiledFsCreator()
    f = lambda x: x
    CheckNamespace(
        'class=CompiledFileSystem&'
            'category=CompiledFileSystemTest/TestFileSystem/file&'
            'app_version=%s' % GetAppVersion(),
        'class=CompiledFileSystem&'
            'category=CompiledFileSystemTest/TestFileSystem/list&'
            'app_version=%s' % GetAppVersion(),
        compiled_fs_creator(f, CompiledFileSystemTest))
    CheckNamespace(
        'class=CompiledFileSystem&'
            'category=CompiledFileSystemTest/TestFileSystem/foo/file&'
            'app_version=%s' % GetAppVersion(),
        'class=CompiledFileSystem&'
            'category=CompiledFileSystemTest/TestFileSystem/foo/list&'
            'app_version=%s' % GetAppVersion(),
        compiled_fs_creator(f, CompiledFileSystemTest, category='foo'))

  def testPopulateFromFile(self):
    def Sleepy(key, val):
      return '%s%s' % ('Z' * len(key), 'z' * len(val))
    compiled_fs = _GetTestCompiledFsCreator()(Sleepy, CompiledFileSystemTest)
    self.assertEqual('ZZZZZZZZzzzzzzzzzzzzzzzzz',
                     compiled_fs.GetFromFile('404.html').Get())
    self.assertEqual('ZZZZZZZZZZZZZZzzzzzzzzzzzzzzzzzz',
                     compiled_fs.GetFromFile('apps/a11y.html').Get())
    self.assertEqual('ZZZZZZZZZZZZZZZZZZZZZZzzzzzzzzzzzzzzzzzz',
                     compiled_fs.GetFromFile('apps/fakedir/file.html').Get())

  def testPopulateFromFileListing(self):
    def strip_ext(_, files):
      return [os.path.splitext(f)[0] for f in files]
    compiled_fs = _GetTestCompiledFsCreator()(strip_ext, CompiledFileSystemTest)
    expected_top_listing = [
      '404',
      'apps/a11y',
      'apps/about_apps',
      'apps/deepdir/deeper/deepest',
      'apps/deepdir/deepfile',
      'apps/fakedir/file',
      'extensions/activeTab',
      'extensions/alarms'
    ]
    self.assertEqual(expected_top_listing,
                     sorted(compiled_fs.GetFromFileListing('').Get()))
    expected_apps_listing = [
      'a11y',
      'about_apps',
      'deepdir/deeper/deepest',
      'deepdir/deepfile',
      'fakedir/file',
    ]
    self.assertEqual(expected_apps_listing,
                     sorted(compiled_fs.GetFromFileListing('apps/').Get()))
    self.assertEqual(['file',],
                     compiled_fs.GetFromFileListing('apps/fakedir/').Get())
    self.assertEqual(['deeper/deepest', 'deepfile'],
                     sorted(compiled_fs.GetFromFileListing(
                         'apps/deepdir/').Get()))
    self.assertEqual(['deepest'],
                     compiled_fs.GetFromFileListing(
                         'apps/deepdir/deeper/').Get())

  def testCaching(self):
    compiled_fs = _GetTestCompiledFsCreator()(Cache(identity),
                                              CompiledFileSystemTest)
    self.assertEqual('404.html contents',
                     compiled_fs.GetFromFile('404.html').Get())
    self.assertEqual(set(('file.html',)),
                     set(compiled_fs.GetFromFileListing('apps/fakedir/').Get()))

    compiled_fs._file_system._path_values['404.html'] = 'boom'
    compiled_fs._file_system._path_values['apps/fakedir/'] = [
        'file.html', 'boom.html']
    self.assertEqual('404.html contents',
                     compiled_fs.GetFromFile('404.html').Get())
    self.assertEqual(set(('file.html',)),
                     set(compiled_fs.GetFromFileListing('apps/fakedir/').Get()))

    compiled_fs._file_system.IncrementStat()
    self.assertEqual('boom', compiled_fs.GetFromFile('404.html').Get())
    self.assertEqual(set(('file.html', 'boom.html')),
                     set(compiled_fs.GetFromFileListing('apps/fakedir/').Get()))

  def testFailures(self):
    compiled_fs = _GetTestCompiledFsCreator()(identity, CompiledFileSystemTest)
    self.assertRaises(FileNotFoundError,
                      compiled_fs.GetFromFile('405.html').Get)
    # TODO(kalman): would be nice to test this fails since apps/ is a dir.
    compiled_fs.GetFromFile('apps')
    #self.assertRaises(SomeError, compiled_fs.GetFromFile, 'apps/')
    self.assertRaises(FileNotFoundError,
                      compiled_fs.GetFromFileListing('nodir/').Get)
    # TODO(kalman): likewise, not a FileNotFoundError.
    self.assertRaises(FileNotFoundError,
                      compiled_fs.GetFromFileListing('404.html/').Get)

  def testCorrectFutureBehaviour(self):
    # Tests that the underlying FileSystem's Read Future has had Get() called
    # on it before the Future is resolved, but the underlying Future isn't
    # resolved until Get is.
    mock_fs = MockFileSystem(TestFileSystem(_TEST_DATA))
    compiled_fs = CompiledFileSystem.Factory(
        ObjectStoreCreator.ForTest()).Create(
            mock_fs, lambda path, contents: contents, type(self))

    self.assertTrue(*mock_fs.CheckAndReset())
    future = compiled_fs.GetFromFile('404.html')
    self.assertTrue(*mock_fs.CheckAndReset(stat_count=1, read_count=1))
    future.Get()
    self.assertTrue(*mock_fs.CheckAndReset(read_resolve_count=1))

    future = compiled_fs.GetFromFileListing('apps/')
    # Current behaviour is to have read=2 and read_resolve=1 because the first
    # level is read eagerly, then all of the second is read (in parallel). If
    # it weren't eager (and it may be worth experimenting with that) then it'd
    # be read=1 and read_resolve=0.
    self.assertTrue(*mock_fs.CheckAndReset(stat_count=1,
                                           read_count=2,
                                           read_resolve_count=1))
    future.Get()
    # It's doing 1 more level 'deeper' (already read 'fakedir' and 'deepdir'
    # though not resolved), so that's 1 more read/resolve + the resolve from
    # the first read.
    self.assertTrue(*mock_fs.CheckAndReset(read_count=1, read_resolve_count=2))

    # Even though the directory is 1 layer deep the caller has no way of
    # determining that ahead of time (though perhaps the API could give some
    # kind of clue, if we really cared).
    future = compiled_fs.GetFromFileListing('extensions/')
    self.assertTrue(*mock_fs.CheckAndReset(stat_count=1,
                                           read_count=1,
                                           read_resolve_count=1))
    future.Get()
    self.assertTrue(*mock_fs.CheckAndReset())

    # Similar configuration to the 'apps/' case but deeper.
    future = compiled_fs.GetFromFileListing('')
    self.assertTrue(*mock_fs.CheckAndReset(stat_count=1,
                                           read_count=2,
                                           read_resolve_count=1))
    future.Get()
    self.assertTrue(*mock_fs.CheckAndReset(read_count=2, read_resolve_count=3))

  def testSkipNotFound(self):
    mock_fs = MockFileSystem(TestFileSystem(_TEST_DATA))
    compiled_fs = CompiledFileSystem.Factory(
        ObjectStoreCreator.ForTest()).Create(
            mock_fs, Cache(lambda path, contents: contents), type(self))

    future = compiled_fs.GetFromFile('no_file', skip_not_found=True)
    # If the file doesn't exist, then the file system is not read.
    self.assertTrue(*mock_fs.CheckAndReset(read_count=1, stat_count=1))
    self.assertEqual(None, future.Get())
    self.assertTrue(*mock_fs.CheckAndReset(read_resolve_count=1))
    future = compiled_fs.GetFromFile('no_file', skip_not_found=True)
    self.assertTrue(*mock_fs.CheckAndReset(stat_count=1))
    self.assertEqual(None, future.Get())
    # The result for a non-existent file should still be cached.
    self.assertTrue(*mock_fs.CheckAndReset())
    future = compiled_fs.GetFromFile('no_file')
    self.assertRaises(FileNotFoundError, future.Get)


if __name__ == '__main__':
  unittest.main()
