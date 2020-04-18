# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for the binpkg.py module."""

from __future__ import print_function

import os

from chromite.lib import binpkg
from chromite.lib import cros_test_lib
from chromite.lib import gs_unittest
from chromite.lib import osutils


PACKAGES_CONTENT = """USE: test

CPV: chromeos-base/shill-0.0.1-r1

CPV: chromeos-base/test-0.0.1-r1
DEBUG_SYMBOLS: yes
"""

class FetchTarballsTest(cros_test_lib.MockTempDirTestCase):
  """Tests for GSContext that go over the network."""

  def testFetchFakePackages(self):
    """Pretend to fetch binary packages."""
    gs_mock = self.StartPatcher(gs_unittest.GSContextMock())
    gs_mock.SetDefaultCmdResult()
    uri = 'gs://foo/bar'
    packages_uri = '{}/Packages'.format(uri)
    packages_file = '''URI: gs://foo

CPV: boo/baz
PATH boo/baz.tbz2
'''
    gs_mock.AddCmdResult(['cat', packages_uri], output=packages_file)

    binpkg.FetchTarballs([uri], self.tempdir)

  @cros_test_lib.NetworkTest()
  def testFetchRealPackages(self):
    """Actually fetch a real binhost from the network."""
    uri = 'gs://chromeos-prebuilt/board/lumpy/paladin-R37-5905.0.0-rc2/packages'
    binpkg.FetchTarballs([uri], self.tempdir)


class DebugSymbolsTest(cros_test_lib.TempDirTestCase):
  """Tests for the debug symbols handling in binpkg."""

  def testDebugSymbolsDetected(self):
    """When generating the Packages file, DEBUG_SYMBOLS is updated."""
    osutils.WriteFile(os.path.join(self.tempdir,
                                   'chromeos-base/shill-0.0.1-r1.debug.tbz2'),
                      'hello', makedirs=True)
    osutils.WriteFile(os.path.join(self.tempdir, 'Packages'),
                      PACKAGES_CONTENT)

    index = binpkg.GrabLocalPackageIndex(self.tempdir)
    self.assertEquals(index.packages[0]['CPV'], 'chromeos-base/shill-0.0.1-r1')
    self.assertEquals(index.packages[0].get('DEBUG_SYMBOLS'), 'yes')
    self.assertFalse('DEBUG_SYMBOLS' in index.packages[1])
