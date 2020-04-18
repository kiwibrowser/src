# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import os
import shutil
import tempfile
import unittest
import urlparse

import sandwich_prefetch


LOADING_DIR = os.path.dirname(__file__)
TEST_DATA_DIR = os.path.join(LOADING_DIR, 'testdata')


class SandwichPrefetchTestCase(unittest.TestCase):
  _TRACE_PATH = os.path.join(TEST_DATA_DIR, 'scanner_vs_parser.trace')

  def setUp(self):
    self._tmp_dir = tempfile.mkdtemp()

  def tearDown(self):
    shutil.rmtree(self._tmp_dir)

  def GetTmpPath(self, file_name):
    return os.path.join(self._tmp_dir, file_name)

  def GetResourceUrl(self, path):
    return urlparse.urljoin('http://l/', path)

  def testEmptyCacheWhitelisting(self):
    url_set = sandwich_prefetch._ExtractDiscoverableUrls(None,
        self._TRACE_PATH, sandwich_prefetch.Discoverer.EmptyCache)
    self.assertEquals(set(), url_set)

  def testFullCacheWhitelisting(self):
    reference_url_set = set([self.GetResourceUrl('./'),
                             self.GetResourceUrl('0.png'),
                             self.GetResourceUrl('1.png'),
                             self.GetResourceUrl('0.css'),
                             self.GetResourceUrl('favicon.ico')])
    url_set = sandwich_prefetch._ExtractDiscoverableUrls(None,
        self._TRACE_PATH, sandwich_prefetch.Discoverer.FullCache)
    self.assertEquals(reference_url_set, url_set)

  def testMainDocumentWhitelisting(self):
    reference_url_set = set([self.GetResourceUrl('./')])
    url_set = sandwich_prefetch._ExtractDiscoverableUrls(None,
        self._TRACE_PATH, sandwich_prefetch.Discoverer.MainDocument)
    self.assertEquals(reference_url_set, url_set)

  def testParserDiscoverableWhitelisting(self):
    reference_url_set = set([self.GetResourceUrl('./'),
                             self.GetResourceUrl('0.png'),
                             self.GetResourceUrl('1.png'),
                             self.GetResourceUrl('0.css')])
    url_set = sandwich_prefetch._ExtractDiscoverableUrls(None,
        self._TRACE_PATH, sandwich_prefetch.Discoverer.Parser)
    self.assertEquals(reference_url_set, url_set)

  def testHTMLPreloadScannerWhitelisting(self):
    reference_url_set = set([self.GetResourceUrl('./'),
                             self.GetResourceUrl('0.png'),
                             self.GetResourceUrl('0.css')])
    url_set = sandwich_prefetch._ExtractDiscoverableUrls(None,
        self._TRACE_PATH, sandwich_prefetch.Discoverer.HTMLPreloadScanner)
    self.assertEquals(reference_url_set, url_set)

  def testHTMLPreloadScannerStoreWhitelisting(self):
    original_headers_path = self.GetTmpPath('original_headers.json')

    def RunTest(reference_urls):
      url_set = sandwich_prefetch._ExtractDiscoverableUrls(
          original_headers_path, self._TRACE_PATH,
          sandwich_prefetch.Discoverer.HTMLPreloadScannerStore)
      self.assertEquals(set(reference_urls), url_set)

    with open(original_headers_path, 'w') as output_file:
      json.dump({
          self.GetResourceUrl('./'): {},
          self.GetResourceUrl('0.png'): {'cache-control': 'max-age=0'},
          self.GetResourceUrl('0.css'): {}
        }, output_file)
    RunTest([self.GetResourceUrl('./'),
             self.GetResourceUrl('0.png'),
             self.GetResourceUrl('0.css')])

    with open(original_headers_path, 'w') as output_file:
      json.dump({
          self.GetResourceUrl('./'): {},
          self.GetResourceUrl('0.png'): {'cache-control': 'private, no-store'},
          self.GetResourceUrl('0.css'): {}
        }, output_file)
    RunTest([self.GetResourceUrl('./'),
             self.GetResourceUrl('0.css')])

    with open(original_headers_path, 'w') as output_file:
      json.dump({
          self.GetResourceUrl('./'): {'cache-control': 'private, no-store'},
          self.GetResourceUrl('0.png'): {},
          self.GetResourceUrl('0.css'): {}
        }, output_file)
    RunTest([self.GetResourceUrl('0.png'),
             self.GetResourceUrl('0.css')])


if __name__ == '__main__':
  unittest.main()
