#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for git_cache.py"""

import os
import shutil
import sys
import tempfile
import unittest

DEPOT_TOOLS_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, DEPOT_TOOLS_ROOT)

from testing_support import coverage_utils
import git_cache

class GitCacheTest(unittest.TestCase):
  @classmethod
  def setUpClass(cls):
    cls.cache_dir = tempfile.mkdtemp(prefix='git_cache_test_')
    git_cache.Mirror.SetCachePath(cls.cache_dir)

  @classmethod
  def tearDownClass(cls):
    shutil.rmtree(cls.cache_dir, ignore_errors=True)

  def testParseFetchSpec(self):
    testData = [
        ([], []),
        (['master'], [('+refs/heads/master:refs/heads/master',
                       r'\+refs/heads/master:.*')]),
        (['master/'], [('+refs/heads/master:refs/heads/master',
                       r'\+refs/heads/master:.*')]),
        (['+master'], [('+refs/heads/master:refs/heads/master',
                       r'\+refs/heads/master:.*')]),
        (['refs/heads/*'], [('+refs/heads/*:refs/heads/*',
                            r'\+refs/heads/\*:.*')]),
        (['foo/bar/*', 'baz'], [('+refs/heads/foo/bar/*:refs/heads/foo/bar/*',
                                r'\+refs/heads/foo/bar/\*:.*'),
                               ('+refs/heads/baz:refs/heads/baz',
                                r'\+refs/heads/baz:.*')]),
        (['refs/foo/*:refs/bar/*'], [('+refs/foo/*:refs/bar/*',
                                      r'\+refs/foo/\*:.*')])
        ]

    mirror = git_cache.Mirror('test://phony.example.biz')
    for fetch_specs, expected in testData:
      mirror = git_cache.Mirror('test://phony.example.biz', refs=fetch_specs)
      self.assertItemsEqual(mirror.fetch_specs, expected)

if __name__ == '__main__':
  sys.exit(coverage_utils.covered_main((
    os.path.join(DEPOT_TOOLS_ROOT, 'git_cache.py')
  ), required_percentage=0))
