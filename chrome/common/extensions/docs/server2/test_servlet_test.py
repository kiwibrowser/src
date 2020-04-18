#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from empty_dir_file_system import EmptyDirFileSystem
from host_file_system_provider import HostFileSystemProvider
from servlet import Request
from test_branch_utility import TestBranchUtility
from fail_on_access_file_system import FailOnAccessFileSystem
from test_servlet import TestServlet

class _TestDelegate(object):
  def CreateBranchUtility(self, object_store_creator):
    return TestBranchUtility.CreateWithCannedData()

  def CreateAppSamplesFileSystem(self, object_store_creator):
    return EmptyDirFileSystem()

  def CreateHostFileSystemProvider(self, object_store_creator):
    return HostFileSystemProvider.ForTest(
        FailOnAccessFileSystem(), object_store_creator)

# This test can't really be useful. The set of valid tests is changing and
# there is no reason to test the tests themselves, they are already tested in
# their respective modules. The only testable behavior TestServlet adds is
# returning a 404 if a test does not exist.
class TestServletTest(unittest.TestCase):
  def testTestServlet(self):
    request = Request('not_a_real_test_url', 'localhost', {})
    test_servlet = TestServlet(request, _TestDelegate())
    response = test_servlet.Get()

    self.assertEqual(404, response.status)

if __name__ == '__main__':
  unittest.main()
