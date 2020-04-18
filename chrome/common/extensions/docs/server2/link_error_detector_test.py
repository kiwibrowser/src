#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from file_system import FileNotFoundError
from link_error_detector import LinkErrorDetector
from servlet import Response
from test_file_system import TestFileSystem

file_system = TestFileSystem({
  'docs': {
    'templates': {
      'public': {
        'apps': {
          '404.html': '404',
          'index.html': '''
            <h1 id="actual-top">Hello</h1>
            <a href="#top">world</p>
            <a href="#actual-top">!</a>
            <a href="broken.json"></a>
            <a href="crx.html"></a>
            ''',
          'crx.html': '''
            <a href="index.html#actual-top">back</a>
            <a href="broken.html"></a>
            <a href="devtools.events.html">do to underscore translation</a>
            ''',
          'devtools_events.html': '''
            <a href=" http://www.google.com/">leading space in href</a>
            <a href=" index.html">home</a>
            <a href="index.html#invalid"></a>
            <a href="fake.html#invalid"></a>
            ''',
          'unreachable.html': '''
            <p>so lonely</p>
            <a href="#aoesu"></a>
            <a href="invalid.html"></a>
            ''',
          'devtools_disconnected.html': ''
        }
      }
    },
    'static': {},
    'examples': {},
  }
})

class LinkErrorDetectorTest(unittest.TestCase):
  def _Render(self, path):
    try:
      return Response(
          content=file_system.ReadSingle('docs/templates/public/' + path).Get(),
          status=200)
    except FileNotFoundError:
      return Response(status=404)

  def testGetBrokenLinks(self):
    expected_broken_links = set([
      (404, 'apps/crx.html', 'apps/broken.html', 'target page not found'),
      (404, 'apps/index.html', 'apps/broken.json', 'target page not found'),
      (404, 'apps/unreachable.html', 'apps/invalid.html',
          'target page not found'),
      (404, 'apps/devtools_events.html', 'apps/fake.html#invalid',
          'target page not found'),
      (200, 'apps/devtools_events.html', 'apps/index.html#invalid',
          'target anchor not found'),
      (200, 'apps/unreachable.html', '#aoesu', 'target anchor not found')])

    link_error_detector = LinkErrorDetector(
        file_system, self._Render, 'templates/public/', ('apps/index.html'))
    broken_links = link_error_detector.GetBrokenLinks()

    self.assertEqual(expected_broken_links, set(broken_links))

  def testGetOrphanedPages(self):
    expected_orphaned_pages = set([
      'apps/unreachable.html',
      'apps/devtools_disconnected.html'])

    link_error_detector = LinkErrorDetector(
        file_system, self._Render, 'templates/public/', ('apps/crx.html',))
    orphaned_pages = link_error_detector.GetOrphanedPages()

    self.assertEqual(expected_orphaned_pages, set(orphaned_pages))

if __name__ == '__main__':
  unittest.main()
