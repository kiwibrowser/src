#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import unittest

from compiled_file_system import CompiledFileSystem
from content_providers import ContentProviders
from extensions_paths import CHROME_EXTENSIONS
from gcs_file_system_provider import CloudStorageFileSystemProvider
from object_store_creator import ObjectStoreCreator
from test_file_system import TestFileSystem
from test_util import DisableLogging


_CONTENT_PROVIDERS = {
  'apples': {
    'chromium': {
      'dir': 'chrome/common/extensions/apples'
    },
    'serveFrom': 'apples-dir',
  },
  'bananas': {
    'serveFrom': '',
    'chromium': {
      'dir': 'chrome/common/extensions'
    },
  },
  'github-provider': {
    'serveFrom': 'gh',
    'github': {
      'dir': 'chrome/common/extensions',
      'owner': 'GoogleChrome',
      'repo': 'hello-world',
    },
  },
  'github-provider-with-dir': {
    'serveFrom': 'gh2',
    'github': {
      'dir': 'chrome/common/extensions/tomatoes/are/a',
      'owner': 'SomeOwner',
      'repo': 'some-repo',
    },
  },
  'tomatoes': {
    'serveFrom': 'tomatoes-dir/are/a',
    'chromium': {
      'dir': 'chrome/common/extensions/tomatoes/are/a'
    },
  },
}


_FILE_SYSTEM_DATA = {
  'docs': {
    'templates': {
      'json': {
        'content_providers.json': json.dumps(_CONTENT_PROVIDERS),
      },
    },
  },
  'apples': {
    'gala.txt': 'gala apples',
    'green': {
      'granny smith.txt': 'granny smith apples',
    },
  },
  'tomatoes': {
    'are': {
      'a': {
        'vegetable.txt': 'no they aren\'t',
        'fruit': {
          'cherry.txt': 'cherry tomatoes',
        },
      },
    },
  },
}


class _MockGithubFileSystemProvider(object):
  '''A GithubFileSystemProvider imitation which records every call to Create
  and returns them from GetAndReset.
  '''

  def __init__(self, file_system):
    self._file_system = file_system
    self._calls = []

  def Create(self, owner, repo):
    self._calls.append((owner, repo))
    return self._file_system

  def GetAndReset(self):
    calls = self._calls
    self._calls = []
    return calls


class ContentProvidersTest(unittest.TestCase):
  def setUp(self):
    object_store_creator = ObjectStoreCreator.ForTest()
    test_file_system = TestFileSystem(_FILE_SYSTEM_DATA,
                                      relative_to=CHROME_EXTENSIONS)
    self._github_fs_provider = _MockGithubFileSystemProvider(test_file_system)
    object_store_creator = ObjectStoreCreator.ForTest()
    # TODO(mangini): create tests for GCS
    self._gcs_fs_provider = CloudStorageFileSystemProvider(object_store_creator)
    self._content_providers = ContentProviders(
        object_store_creator,
        CompiledFileSystem.Factory(object_store_creator),
        test_file_system,
        self._gcs_fs_provider)

  def testSimpleRootPath(self):
    provider = self._content_providers.GetByName('apples')
    self.assertEqual(
        'gala apples',
        provider.GetContentAndType('gala.txt').Get().content)
    self.assertEqual(
        'granny smith apples',
        provider.GetContentAndType('green/granny smith.txt').Get().content)

  def testComplexRootPath(self):
    provider = self._content_providers.GetByName('tomatoes')
    self.assertEqual(
        'no they aren\'t',
        provider.GetContentAndType('vegetable.txt').Get().content)
    self.assertEqual(
        'cherry tomatoes',
        provider.GetContentAndType('fruit/cherry.txt').Get().content)

  def testParentRootPath(self):
    provider = self._content_providers.GetByName('bananas')
    self.assertEqual(
        'gala apples',
        provider.GetContentAndType('apples/gala.txt').Get().content)

  def testSimpleServlet(self):
    provider, serve_from, path = self._content_providers.GetByServeFrom(
        'apples-dir')
    self.assertEqual('apples', provider.name)
    self.assertEqual('apples-dir', serve_from)
    self.assertEqual('', path)
    provider, serve_from, path = self._content_providers.GetByServeFrom(
        'apples-dir/')
    self.assertEqual('apples', provider.name)
    self.assertEqual('apples-dir', serve_from)
    self.assertEqual('', path)
    provider, serve_from, path = self._content_providers.GetByServeFrom(
        'apples-dir/are/forever')
    self.assertEqual('apples', provider.name)
    self.assertEqual('apples-dir', serve_from)
    self.assertEqual('are/forever', path)

  def testComplexServlet(self):
    provider, serve_from, path = self._content_providers.GetByServeFrom(
        'tomatoes-dir/are/a')
    self.assertEqual('tomatoes', provider.name)
    self.assertEqual('tomatoes-dir/are/a', serve_from)
    self.assertEqual('', path)
    provider, serve_from, path = self._content_providers.GetByServeFrom(
        'tomatoes-dir/are/a/fruit/they/are')
    self.assertEqual('tomatoes', provider.name)
    self.assertEqual('tomatoes-dir/are/a', serve_from)
    self.assertEqual('fruit/they/are', path)

  def testEmptyStringServlet(self):
    provider, serve_from, path = self._content_providers.GetByServeFrom(
        'tomatoes-dir/are')
    self.assertEqual('bananas', provider.name)
    self.assertEqual('', serve_from)
    self.assertEqual('tomatoes-dir/are', path)
    provider, serve_from, path = self._content_providers.GetByServeFrom('')
    self.assertEqual('bananas', provider.name)
    self.assertEqual('', serve_from)
    self.assertEqual('', path)

  @DisableLogging('error')
  def testProviderNotFound(self):
    self.assertEqual(None, self._content_providers.GetByName('cabbages'))

  # TODO: Re-enable these Github tests if we ever decide to restore our support
  # for Github content providers.
  # def testGithubContentProvider(self):
  #   provider, serve_from, path = self._content_providers.GetByServeFrom(
  #       'gh/apples/green/granny smith.txt')
  #   self.assertEqual('github-provider', provider.name)
  #   self.assertEqual('gh', serve_from)
  #   self.assertEqual('apples/green/granny smith.txt', path)
  #   self.assertEqual([('GoogleChrome', 'hello-world')],
  #                    self._github_fs_provider.GetAndReset())
  #   self.assertEqual(
  #       'granny smith apples',
  #       provider.GetContentAndType(path).Get().content)

  # def testGithubContentProviderWithDir(self):
  #   provider, serve_from, path = self._content_providers.GetByServeFrom(
  #       'gh2/fruit/cherry.txt')
  #   self.assertEqual('github-provider-with-dir', provider.name)
  #   self.assertEqual('gh2', serve_from)
  #   self.assertEqual('fruit/cherry.txt', path)
  #   self.assertEqual([('SomeOwner', 'some-repo')],
  #                    self._github_fs_provider.GetAndReset())
  #   self.assertEqual(
  #       'cherry tomatoes',
  #       provider.GetContentAndType(path).Get().content)

if __name__ == '__main__':
  unittest.main()
