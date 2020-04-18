#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import unittest

import PRESUBMIT

EXTENSIONS_PATH = os.path.join('chrome', 'common', 'extensions')
DOCS_PATH = os.path.join(EXTENSIONS_PATH, 'docs')
SERVER2_PATH = os.path.join(DOCS_PATH, 'server2')
PUBLIC_PATH = os.path.join(DOCS_PATH, 'templates', 'public')
PRIVATE_PATH = os.path.join(DOCS_PATH, 'templates', 'private')
INTROS_PATH = os.path.join(DOCS_PATH, 'templates', 'intros')
ARTICLES_PATH = os.path.join(DOCS_PATH, 'templates', 'articles')

class PRESUBMITTest(unittest.TestCase):
  def testCreateIntegrationTestArgs(self):
    input_files = [
      os.path.join(EXTENSIONS_PATH, 'test.cc'),
      os.path.join(EXTENSIONS_PATH, 'test2.cc'),
      os.path.join('test', 'test.py')
    ]
    expected_files = []
    self.assertEqual(expected_files,
                     PRESUBMIT._CreateIntegrationTestArgs(input_files))
    expected_files.append(os.path.join('apps', 'fileSystem.html'))
    input_files.append(os.path.join(EXTENSIONS_PATH, 'api', 'file_system.idl'))
    self.assertEqual(expected_files,
                     PRESUBMIT._CreateIntegrationTestArgs(input_files))
    expected_files.append(os.path.join('extensions', 'alarms.html'))
    expected_files.append(os.path.join('apps', 'alarms.html'))
    input_files.append(os.path.join(EXTENSIONS_PATH, 'api', 'alarms.json'))
    self.assertEqual(expected_files,
                     PRESUBMIT._CreateIntegrationTestArgs(input_files))
    expected_files.append('extensions/devtools_network.html')
    input_files.append(os.path.join(EXTENSIONS_PATH,
                                    'api',
                                    'devtools',
                                    'network.json'))
    self.assertEqual(expected_files,
                     PRESUBMIT._CreateIntegrationTestArgs(input_files))
    expected_files.append(os.path.join('extensions', 'docs.html'))
    expected_files.append(os.path.join('apps', 'docs.html'))
    input_files.append(os.path.join(PUBLIC_PATH, 'extensions', 'docs.html'))
    self.assertEqual(expected_files,
                     PRESUBMIT._CreateIntegrationTestArgs(input_files))
    expected_files.append(os.path.join('extensions', 'bookmarks.html'))
    input_files.append(os.path.join(INTROS_PATH, 'bookmarks.html'))
    self.assertEqual(expected_files,
                     PRESUBMIT._CreateIntegrationTestArgs(input_files))
    expected_files.append(os.path.join('extensions', 'i18n.html'))
    expected_files.append(os.path.join('apps', 'i18n.html'))
    input_files.append(os.path.join(INTROS_PATH, 'i18n.html'))
    self.assertEqual(expected_files,
                     PRESUBMIT._CreateIntegrationTestArgs(input_files))
    expected_files.append(os.path.join('apps', 'about_apps.html'))
    input_files.append(os.path.join(ARTICLES_PATH, 'about_apps.html'))
    self.assertEqual(expected_files,
                     PRESUBMIT._CreateIntegrationTestArgs(input_files))
    input_files.append(os.path.join(PRIVATE_PATH, 'type.html'))
    self.assertEqual([ '-a' ],
                     PRESUBMIT._CreateIntegrationTestArgs(input_files))
    input_files.pop()
    input_files.append(os.path.join(SERVER2_PATH, 'test.txt'))
    self.assertEqual(expected_files,
                     PRESUBMIT._CreateIntegrationTestArgs(input_files))
    input_files.append(os.path.join(SERVER2_PATH, 'handler.py'))
    self.assertEqual([ '-a' ],
                     PRESUBMIT._CreateIntegrationTestArgs(input_files))

if __name__ == '__main__':
  unittest.main()
