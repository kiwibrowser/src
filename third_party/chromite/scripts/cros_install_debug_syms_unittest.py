# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for cros_install_debug_syms.py"""

from __future__ import print_function

from collections import namedtuple
import os

from chromite.lib import cros_test_lib
from chromite.scripts import cros_install_debug_syms


SimpleIndex = namedtuple('SimpleIndex', 'header packages')


class InstallDebugSymsTest(cros_test_lib.MockTestCase):
  """Test the parsing of package index"""

  def setUp(self):
    self.local_binhosts = ['/build/something/packages/',
                           'file:///build/somethingelse/packages',
                           'file://localhost/build/another/packages']

    self.remote_binhosts = ['http://domain.com/binhost',
                            'gs://chromeos-stuff/binhost']

  def testGetLocalPackageIndex(self):
    """Check that local binhosts are fetched correctly."""
    self.PatchObject(cros_install_debug_syms.binpkg, "GrabLocalPackageIndex",
                     return_value=SimpleIndex({}, {}))
    self.PatchObject(cros_install_debug_syms.os.path, 'isdir',
                     return_value=True)
    for binhost in self.local_binhosts:
      cros_install_debug_syms.GetPackageIndex(binhost)

  def testGetRemotePackageIndex(self):
    """Check that remote binhosts are fetched correctly."""
    self.PatchObject(cros_install_debug_syms.binpkg, "GrabRemotePackageIndex",
                     return_value=SimpleIndex({}, {}))
    for binhost in self.remote_binhosts:
      cros_install_debug_syms.GetPackageIndex(binhost)

  def testListRemoteBinhost(self):
    """Check that urls are generated correctly for remote binhosts."""
    chaps_cpv = 'chromeos-base/chaps-0-r2'
    metrics_cpv = 'chromeos-base/metrics-0-r4'

    index = SimpleIndex({}, [{'CPV': 'chromeos-base/shill-0-r1'},
                             {'CPV': chaps_cpv,
                              'DEBUG_SYMBOLS': 'yes'},
                             {'CPV': metrics_cpv,
                              'DEBUG_SYMBOLS': 'yes',
                              'PATH': 'path/to/binpkg.tbz2'}])
    self.PatchObject(cros_install_debug_syms, 'GetPackageIndex',
                     return_value=index)

    for binhost in self.remote_binhosts:
      expected = {chaps_cpv: os.path.join(binhost, chaps_cpv + '.debug.tbz2'),
                  metrics_cpv: os.path.join(binhost,
                                            'path/to/binpkg.debug.tbz2')}
      self.assertEquals(cros_install_debug_syms.ListBinhost(binhost), expected)

  def testListRemoteBinhostWithURI(self):
    """Check that urls are generated correctly when URI is defined."""
    index = SimpleIndex({'URI': 'gs://chromeos-prebuilts'},
                        [{'CPV': 'chromeos-base/shill-0-r1',
                          'DEBUG_SYMBOLS': 'yes',
                          'PATH': 'amd64-generic/paladin1234/shill-0-r1.tbz2'}])
    self.PatchObject(cros_install_debug_syms, 'GetPackageIndex',
                     return_value=index)

    binhost = 'gs://chromeos-prebuilts/gizmo-paladin/'
    debug_symbols_url = ('gs://chromeos-prebuilts/amd64-generic'
                         '/paladin1234/shill-0-r1.debug.tbz2')
    self.assertEquals(cros_install_debug_syms.ListBinhost(binhost),
                      {'chromeos-base/shill-0-r1': debug_symbols_url})
