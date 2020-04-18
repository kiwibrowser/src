# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for updating Chrome ebuild stages."""

from __future__ import print_function

from chromite.cbuildbot import afdo
from chromite.cbuildbot.stages import afdo_stages
from chromite.cbuildbot.stages import generic_stages_unittest

from chromite.lib import gs
from chromite.lib import portage_util


class UpdateChromeEbuildTest(generic_stages_unittest.AbstractStageTestCase):
  """Test updating Chrome ebuild files"""

  def setUp(self):
    # Intercept afdo.UpdateChromeEbuildAFDOFile so that we can check how it is
    # called.
    self.patch_mock = self.PatchObject(afdo, 'UpdateChromeEbuildAFDOFile',
                                       autospec=True)
    # Don't care the return value of portage_util.BestVisible
    self.PatchObject(portage_util, 'BestVisible')
    # Don't care the return value of gs.GSContext
    self.PatchObject(gs, 'GSContext')
    # Don't call the getters; Use mock responses instead.
    self.PatchDict(afdo.PROFILE_SOURCES,
                   {'benchmark': lambda *_: 'benchmark.afdo',
                    'silvermont': lambda *_: 'silvermont.afdo'},
                   clear=True)
    self._Prepare()

  def ConstructStage(self):
    return afdo_stages.AFDOUpdateChromeEbuildStage(self._run)

  def testAFDOUpdateChromeEbuildStage(self):
    self.RunStage()

    # afdo.UpdateChromeEbuildAFDOFile should be called with the mock responses
    # from profile source specific query.
    self.patch_mock.assert_called_with(
        'amd64-generic',
        {'benchmark': 'benchmark.afdo',
         'silvermont': 'silvermont.afdo'})
