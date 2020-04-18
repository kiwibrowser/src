#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import unittest

from compiled_file_system import CompiledFileSystem
from object_store_creator import ObjectStoreCreator
from redirector import Redirector
from test_file_system import TestFileSystem
from third_party.json_schema_compiler.json_parse import Parse

HOST = 'localhost/'

file_system = TestFileSystem({
  'redirects.json': json.dumps({
    'foo/...': 'apps/...',
    '': '/index.html',
    'home': 'index.html',
    'index.html': 'http://something.absolute.com/'
  }),
  'apps': {
    'redirects.json': json.dumps({
      '': '../index.html',
      'index.html': 'about_apps.html',
      'foo.html': '/bar.html',
    })
  },
  'extensions': {
    'redirects.json': json.dumps({
      'manifest': 'manifest.html',
      'tabs': 'tabs.html',
      'dev/...': '...',
      'a/very/long/dir/chain/...': 'short/...',
      '_short/...': 'another/long/chain/...',
      'r1/...': 'r2/r1/...',
      'r2/r1/...': 'r3/...',
      'r3/...': 'r4/...',
      'r5/...': 'r6/...',
      'nofile1/...': 'nofile2/...',
      'noredirects1/...': 'noredirects2/...'
    }),
    'manifest': {
      'redirects.json': json.dumps({
        '': '../manifest.html',
        'more-info': 'http://lmgtfy.com'
      }),
    },
    'stable': {
        'redirects.json': json.dumps({
            'tabs': 'tabs.html'
        }),
        'manifest': {
          'redirects.json': json.dumps({
            'storage': 'storage.html'
          })
        },
    },
    'dev': {
      'redirects.json': json.dumps({
        'tabs': 'tabs.html',
        'manifest': 'manifest.html'
      }),
      'manifest': {
        'redirects.json': json.dumps({
          'storage': 'storage.html'
        })
      }
    },
    'r4': {
      'redirects.json': json.dumps({
        'manifest': 'manifest.html'
      })
    },
    'r6': {
      'redirects.json': json.dumps({
        '...': 'directory/...'
      }),
      'directory': {
        'redirects.json': json.dumps({
          'manifest': 'manifest.html'
        }),
        'manifest': 'manifest.html'
      }
    },
    'short': {
      'redirects.json': json.dumps({
        'index': 'index.html'
      })
    },
    'another': {
      'long': {
        'chain': {
          'redirects.json': json.dumps({
            'index': 'index.html'
          })
        }
      }
    },
    'nofile': {
      'redirects.json': json.dumps({
      })
    }
  },
  'priority': {
    'redirects.json': json.dumps({
      'directory/...': 'GOOD/...'
    }),
    'directory': {
      'redirects.json': json.dumps({
        '...': '../BAD/...'
      }),
    }
  },
  'relative_directory': {
    'redirects.json': json.dumps({
      '...': '../...'
    })
  },
  'infinite_redirect': {
    'redirects.json': json.dumps({
      '...': 'loop/...'
    }),
    'loop': {
      'redirects.json': json.dumps({
        '...': './...'
      })
    }
  },
  'parent_redirect': {
    'redirects.json': json.dumps({
      'a/...': 'b/...'
    })
  }
})

class RedirectorTest(unittest.TestCase):
  def setUp(self):
    self._redirector = Redirector(
        CompiledFileSystem.Factory(ObjectStoreCreator.ForTest()),
        file_system)

  def testExternalRedirection(self):
    self.assertEqual(
        'http://something.absolute.com/',
        self._redirector.Redirect(HOST, 'index.html'))
    self.assertEqual(
        'http://lmgtfy.com',
        self._redirector.Redirect(HOST, 'extensions/manifest/more-info'))

  def testAbsoluteRedirection(self):
    self.assertEqual(
        '/index.html', self._redirector.Redirect(HOST, ''))
    self.assertEqual(
        '/bar.html', self._redirector.Redirect(HOST, 'apps/foo.html'))

  def testRelativeRedirection(self):
    self.assertEqual(
        'apps/about_apps.html',
        self._redirector.Redirect(HOST, 'apps/index.html'))
    self.assertEqual(
        'extensions/manifest.html',
        self._redirector.Redirect(HOST, 'extensions/manifest/'))
    self.assertEqual(
        'extensions/manifest.html',
        self._redirector.Redirect(HOST, 'extensions/manifest'))
    self.assertEqual(
        'index.html', self._redirector.Redirect(HOST, 'apps/'))
    self.assertEqual(
        'index.html', self._redirector.Redirect(HOST, 'home'))

  def testNotFound(self):
    self.assertEqual(
        None, self._redirector.Redirect(HOST, 'not/a/real/path'))
    self.assertEqual(
        None, self._redirector.Redirect(HOST, 'public/apps/okay.html'))

  def testOldHosts(self):
    self.assertEqual(
        'https://developer.chrome.com/',
        self._redirector.Redirect('code.google.com', ''))

  def testRefresh(self):
    self._redirector.Refresh().Get()

    expected_paths = set([
      'redirects.json',
      'apps/redirects.json',
      'extensions/redirects.json',
      'extensions/manifest/redirects.json'
    ])

    for path in expected_paths:
      self.assertEqual(
          Parse(file_system.ReadSingle(path).Get()),
          # Access the cache's object store to see what files were hit during
          # the cron run. Returns strings parsed as JSON.
          # TODO(jshumway): Make a non hack version of this check.
          self._redirector._cache._file_object_store.Get(
              path).Get().cache_data)

  def testDirectoryRedirection(self):
    # Simple redirect.
    self.assertEqual(
      'extensions/manifest.html',
      self._redirector.Redirect(HOST, 'extensions/dev/manifest'))

    # Multiple hops with one file.
    self.assertEqual(
      'extensions/r4/manifest.html',
      self._redirector.Redirect(HOST, 'extensions/r1/manifest'))

    # Multiple hops w/ multiple redirection files.
    self.assertEqual(
      'extensions/r6/directory/manifest.html',
      self._redirector.Redirect(HOST, 'extensions/r5/manifest'))

    # Redirection from root directory redirector.
    self.assertEqual(
      'apps/about_apps.html',
      self._redirector.Redirect(HOST, 'foo/index.html'))

    # Short to long.
    self.assertEqual(
      'extensions/short/index.html',
      self._redirector.Redirect(HOST, 'extensions/a/very/long/dir/chain/index'))

    # Long to short.
    self.assertEqual(
      'extensions/another/long/chain/index.html',
      self._redirector.Redirect(HOST, 'extensions/_short/index'))

    # Directory redirection without a redirects.json in final directory.
    self.assertEqual(
      'extensions/noredirects2/file',
      self._redirector.Redirect(HOST, 'extensions/noredirects1/file'))

    # Directory redirection with redirects.json without rule for the filename.
    self.assertEqual(
      'extensions/nofile2/file',
      self._redirector.Redirect(HOST, 'extensions/nofile1/file'))

    # Relative directory path.
    self.assertEqual(
      'index.html',
      self._redirector.Redirect(HOST, 'relative_directory/home'))

    # Shallower directory redirects have priority.
    self.assertEqual(
      'priority/GOOD/index',
      self._redirector.Redirect(HOST, 'priority/directory/index'))

    # Don't infinitely redirect.
    self.assertEqual('infinite_redirect/loop/index',
      self._redirector.Redirect(HOST, 'infinite_redirect/index'))

    # If a parent directory is redirected, redirect children properly.
    self.assertEqual('parent_redirect/b/c/index',
      self._redirector.Redirect(HOST, 'parent_redirect/a/c/index'))


if __name__ == '__main__':
  unittest.main()
