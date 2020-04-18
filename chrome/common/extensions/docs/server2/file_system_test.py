#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from test_file_system import TestFileSystem

file_system = TestFileSystem({
  'file.txt': '',
  'templates': {
    'README': '',
    'public': {
      'apps': {
        '404.html': '',
        'a11y.html': ''
      },
      'extensions': {
        '404.html': '',
        'cookies.html': ''
      },
      'redirects.json': 'redirect'
    },
    'json': {
      'manifest.json': 'manifest'
    }
  }
})

class FileSystemTest(unittest.TestCase):
  def testWalk(self):
    expected_files = [
      '^/file.txt',
      'templates/README',
      'templates/public/apps/404.html',
      'templates/public/apps/a11y.html',
      'templates/public/extensions/404.html',
      'templates/public/extensions/cookies.html',
      'templates/public/redirects.json',
      'templates/json/manifest.json'
    ]

    expected_dirs = [
      '^/templates/',
      'templates/public/',
      'templates/public/apps/',
      'templates/public/extensions/',
      'templates/json/'
    ]

    all_files = []
    all_dirs = []
    for root, dirs, files in file_system.Walk(''):
      if not root: root = '^'
      all_files += [root + '/' + name for name in files]
      all_dirs += [root + '/' + name for name in dirs]

    self.assertEqual(sorted(expected_files), sorted(all_files))
    self.assertEqual(sorted(expected_dirs), sorted(all_dirs))

  def testWalkDepth(self):
    all_dirs = []
    all_files = []
    for root, dirs, files in file_system.Walk('', depth=0):
      all_dirs.extend(dirs)
      all_files.extend(files)
    self.assertEqual([], all_dirs)
    self.assertEqual([], all_files)

    for root, dirs, files in file_system.Walk('', depth=1):
      all_dirs.extend(dirs)
      all_files.extend(files)
    self.assertEqual(['templates/'], all_dirs)
    self.assertEqual(['file.txt'], all_files)

    all_dirs = []
    all_files = []
    for root, dirs, files in file_system.Walk('', depth=2):
      all_dirs.extend(dirs)
      all_files.extend(files)
    self.assertEqual(sorted(['templates/', 'public/', 'json/']),
                     sorted(all_dirs))
    self.assertEqual(sorted(['file.txt', 'README']), sorted(all_files))


  def testSubWalk(self):
    expected_files = set([
      '/redirects.json',
      'apps/404.html',
      'apps/a11y.html',
      'extensions/404.html',
      'extensions/cookies.html'
    ])

    all_files = set()
    for root, dirs, files in file_system.Walk('templates/public/'):
      all_files.update(root + '/' + name for name in files)

    self.assertEqual(expected_files, all_files)

  def testExists(self):
    def exists(path):
      return file_system.Exists(path).Get()

    # Root directory.
    self.assertTrue(exists(''))

    # Directories (are not files).
    self.assertFalse(exists('templates'))
    self.assertTrue(exists('templates/'))
    self.assertFalse(exists('templates/public'))
    self.assertTrue(exists('templates/public/'))
    self.assertFalse(exists('templates/public/apps'))
    self.assertTrue(exists('templates/public/apps/'))

    # Files (are not directories).
    self.assertTrue(exists('file.txt'))
    self.assertFalse(exists('file.txt/'))
    self.assertTrue(exists('templates/README'))
    self.assertFalse(exists('templates/README/'))
    self.assertTrue(exists('templates/public/redirects.json'))
    self.assertFalse(exists('templates/public/redirects.json/'))
    self.assertTrue(exists('templates/public/apps/a11y.html'))
    self.assertFalse(exists('templates/public/apps/a11y.html/'))


if __name__ == '__main__':
  unittest.main()
