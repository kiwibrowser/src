#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
from copy import deepcopy
from cStringIO import StringIO
from functools import partial
from hashlib import sha1
from random import random
import unittest
from zipfile import ZipFile

from caching_file_system import CachingFileSystem
from file_system import FileNotFoundError, StatInfo
from fake_url_fetcher import FakeURLFSFetcher, MockURLFetcher
from local_file_system import LocalFileSystem
from new_github_file_system import GithubFileSystem
from object_store_creator import ObjectStoreCreator
from test_file_system import TestFileSystem


class _TestBundle(object):
  '''Bundles test file data with a GithubFileSystem and test utilites. Create
  GithubFileSystems via |CreateGfs()|, the Fetcher it uses as |fetcher|,
  randomly mutate its contents via |Mutate()|, and access the underlying zip
  data via |files|.
  '''

  def __init__(self):
    self.files = {
      'zipfile/': '',
      'zipfile/hello.txt': 'world',
      'zipfile/readme': 'test zip',
      'zipfile/dir/file1': 'contents',
      'zipfile/dir/file2': 'more contents'
    }
    self._test_files = {
      'test_owner': {
        'changing-repo': {
          'commits': {
            'HEAD': self._MakeShaJson(self._GenerateHash())
          },
          'zipball': self._ZipFromFiles(self.files)
        }
      }
    }

    self._fake_fetcher = None


  def CreateGfsAndFetcher(self):
    fetchers = []
    def create_mock_url_fetcher(base_path):
      assert not fetchers
      # Save this reference so we can replace the TestFileSystem in Mutate.
      self._fake_fetcher = FakeURLFSFetcher(
          TestFileSystem(self._test_files), base_path)
      fetchers.append(MockURLFetcher(self._fake_fetcher))
      return fetchers[-1]

    # Constructing |gfs| will create a fetcher.
    gfs = GithubFileSystem.ForTest(
        'changing-repo/', create_mock_url_fetcher, path='')
    assert len(fetchers) == 1
    return gfs, fetchers[0]

  def Mutate(self):
    fake_version = self._GenerateHash()
    fake_data = self._GenerateHash()
    self.files['zipfile/hello.txt'] = fake_data
    self.files['zipfile/new-file'] = fake_data
    self.files['zipfile/dir/file1'] = fake_data
    self._test_files['test_owner']['changing-repo']['zipball'] = (
        self._ZipFromFiles(self.files))
    self._test_files['test_owner']['changing-repo']['commits']['HEAD'] = (
        self._MakeShaJson(fake_version))

    # Update the file_system used by FakeURLFSFetcher so the above mutations
    # propagate.
    self._fake_fetcher.UpdateFS(TestFileSystem(self._test_files))

    return fake_version, fake_data

  def _GenerateHash(self):
    '''Generates an arbitrary SHA1 hash.
    '''
    return sha1(str(random())).hexdigest()

  def _MakeShaJson(self, hash_value):
    commit_json = json.loads(deepcopy(LocalFileSystem('').ReadSingle(
        'test_data/github_file_system/test_owner/repo/commits/HEAD').Get()))
    commit_json['sha'] = hash_value
    return json.dumps(commit_json)

  def _ZipFromFiles(self, file_dict):
    string = StringIO()
    zipfile = ZipFile(string, 'w')
    for filename, contents in file_dict.iteritems():
      zipfile.writestr(filename, contents)
    zipfile.close()
    return string.getvalue()


class TestGithubFileSystem(unittest.TestCase):
  def setUp(self):
    self._gfs = GithubFileSystem.ForTest(
        'repo/', partial(FakeURLFSFetcher, LocalFileSystem('')))
    # Start and finish the repository load.
    self._cgfs = CachingFileSystem(self._gfs, ObjectStoreCreator.ForTest())

  def testReadDirectory(self):
    self._gfs.Refresh().Get()
    self.assertEqual(
        sorted(['requirements.txt', '.gitignore', 'README.md', 'src/']),
        sorted(self._gfs.ReadSingle('').Get()))
    self.assertEqual(
        sorted(['__init__.notpy', 'hello.notpy']),
        sorted(self._gfs.ReadSingle('src/').Get()))

  def testReadFile(self):
    self._gfs.Refresh().Get()
    expected = (
      '# Compiled Python files\n'
      '*.pyc\n'
    )
    self.assertEqual(expected, self._gfs.ReadSingle('.gitignore').Get())

  def testMultipleReads(self):
    self._gfs.Refresh().Get()
    self.assertEqual(
        self._gfs.ReadSingle('requirements.txt').Get(),
        self._gfs.ReadSingle('requirements.txt').Get())

  def testReads(self):
    self._gfs.Refresh().Get()
    expected = {
        'src/': sorted(['hello.notpy', '__init__.notpy']),
        '': sorted(['requirements.txt', '.gitignore', 'README.md', 'src/'])
    }

    read = self._gfs.Read(['', 'src/']).Get()
    self.assertEqual(expected['src/'], sorted(read['src/']))
    self.assertEqual(expected[''], sorted(read['']))

  def testStat(self):
    # This is the hash value from the zip on disk.
    real_hash = 'c36fc23688a9ec9e264d3182905dc0151bfff7d7'

    self._gfs.Refresh().Get()
    dir_stat = StatInfo(real_hash, {
      'hello.notpy': StatInfo(real_hash),
      '__init__.notpy': StatInfo(real_hash)
    })

    self.assertEqual(StatInfo(real_hash), self._gfs.Stat('README.md'))
    self.assertEqual(StatInfo(real_hash), self._gfs.Stat('src/hello.notpy'))
    self.assertEqual(dir_stat, self._gfs.Stat('src/'))

  def testBadReads(self):
    self._gfs.Refresh().Get()
    self.assertRaises(FileNotFoundError, self._gfs.Stat, 'DONT_README.md')
    self.assertRaises(FileNotFoundError,
                      self._gfs.ReadSingle('DONT_README.md').Get)

  def testCachingFileSystem(self):
    self._cgfs.Refresh().Get()
    initial_cgfs_read_one = self._cgfs.ReadSingle('src/hello.notpy').Get()

    self.assertEqual(initial_cgfs_read_one,
                     self._gfs.ReadSingle('src/hello.notpy').Get())
    self.assertEqual(initial_cgfs_read_one,
                     self._cgfs.ReadSingle('src/hello.notpy').Get())

    initial_cgfs_read_two = self._cgfs.Read(
        ['README.md', 'requirements.txt']).Get()

    self.assertEqual(
        initial_cgfs_read_two,
        self._gfs.Read(['README.md', 'requirements.txt']).Get())
    self.assertEqual(
        initial_cgfs_read_two,
        self._cgfs.Read(['README.md', 'requirements.txt']).Get())

  def testWithoutRefresh(self):
    # Without refreshing it will still read the content from blobstore, and it
    # does this via the magic of the FakeURLFSFetcher.
    self.assertEqual(['__init__.notpy', 'hello.notpy'],
                     sorted(self._gfs.ReadSingle('src/').Get()))

  def testRefresh(self):
    test_bundle = _TestBundle()
    gfs, fetcher = test_bundle.CreateGfsAndFetcher()

    # It shouldn't fetch until Refresh does so; then it will do 2, one for the
    # stat, and another for the read.
    self.assertTrue(*fetcher.CheckAndReset())
    gfs.Refresh().Get()
    self.assertTrue(*fetcher.CheckAndReset(fetch_count=1,
                                           fetch_async_count=1,
                                           fetch_resolve_count=1))

    # Refresh is just an alias for Read('').
    gfs.Refresh().Get()
    self.assertTrue(*fetcher.CheckAndReset())

    initial_dir_read = sorted(gfs.ReadSingle('').Get())
    initial_file_read = gfs.ReadSingle('dir/file1').Get()

    version, data = test_bundle.Mutate()

    # Check that changes have not effected the file system yet.
    self.assertEqual(initial_dir_read, sorted(gfs.ReadSingle('').Get()))
    self.assertEqual(initial_file_read, gfs.ReadSingle('dir/file1').Get())
    self.assertNotEqual(StatInfo(version), gfs.Stat(''))

    gfs, fetcher = test_bundle.CreateGfsAndFetcher()
    gfs.Refresh().Get()
    self.assertTrue(*fetcher.CheckAndReset(fetch_count=1,
                                           fetch_async_count=1,
                                           fetch_resolve_count=1))

    # Check that the changes have affected the file system.
    self.assertEqual(data, gfs.ReadSingle('new-file').Get())
    self.assertEqual(test_bundle.files['zipfile/dir/file1'],
                     gfs.ReadSingle('dir/file1').Get())
    self.assertEqual(StatInfo(version), gfs.Stat('new-file'))

    # Regression test: ensure that reading the data after it's been mutated,
    # but before Refresh() has been realised, still returns the correct data.
    gfs, fetcher = test_bundle.CreateGfsAndFetcher()
    version, data = test_bundle.Mutate()

    refresh_future = gfs.Refresh()
    self.assertTrue(*fetcher.CheckAndReset(fetch_count=1, fetch_async_count=1))

    self.assertEqual(data, gfs.ReadSingle('new-file').Get())
    self.assertEqual(test_bundle.files['zipfile/dir/file1'],
                     gfs.ReadSingle('dir/file1').Get())
    self.assertEqual(StatInfo(version), gfs.Stat('new-file'))

    refresh_future.Get()
    self.assertTrue(*fetcher.CheckAndReset(fetch_resolve_count=1))

  def testGetThenRefreshOnStartup(self):
    # Regression test: Test that calling Get() but never resolving the future,
    # then Refresh()ing the data, causes the data to be refreshed.
    test_bundle = _TestBundle()
    gfs, fetcher = test_bundle.CreateGfsAndFetcher()
    self.assertTrue(*fetcher.CheckAndReset())

    # Get a predictable version.
    version, data = test_bundle.Mutate()

    read_future = gfs.ReadSingle('hello.txt')
    # Fetch for the Stat(), async-fetch for the Read().
    self.assertTrue(*fetcher.CheckAndReset(fetch_count=1, fetch_async_count=1))

    refresh_future = gfs.Refresh()
    self.assertTrue(*fetcher.CheckAndReset())

    self.assertEqual(data, read_future.Get())
    self.assertTrue(*fetcher.CheckAndReset(fetch_resolve_count=1))
    self.assertEqual(StatInfo(version), gfs.Stat('hello.txt'))
    self.assertTrue(*fetcher.CheckAndReset())

    # The fetch will already have been resolved, so resolving the Refresh won't
    # affect anything.
    refresh_future.Get()
    self.assertTrue(*fetcher.CheckAndReset())

    # Read data should not have changed.
    self.assertEqual(data, gfs.ReadSingle('hello.txt').Get())
    self.assertEqual(StatInfo(version), gfs.Stat('hello.txt'))
    self.assertTrue(*fetcher.CheckAndReset())


if __name__ == '__main__':
  unittest.main()
