# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module that contains unittests for cl_messages module."""

from __future__ import print_function

from chromite.lib import cl_messages
from chromite.lib import constants
from chromite.lib import cros_test_lib
from chromite.lib import triage_lib
from chromite.lib import patch_unittest

class TestCreateValidationFailureMessage(cros_test_lib.MockTestCase):
  """Tests CreateValidationFailureMessage."""

  def setUp(self):
    self._patch_factory = patch_unittest.MockPatchFactory()
    self.GetPatches = self._patch_factory.GetPatches

  def _AssertMessage(self, change, suspects, messages, sanity=True,
                     infra_fail=False, lab_fail=False, no_stat=None,
                     xretry=False, pre_cq_trybot=False, cl_status_url=None):
    """Call the CreateValidationFailureMessage method.

    Args:
      change: The change we are commenting on.
      suspects: List of suspected changes.
      messages: List of messages should appear in the failure message.
      sanity: Bool indicating sanity of build, default: True.
      infra_fail: True if build failed due to infrastructure issues.
      lab_fail: True if build failed due to lab infrastructure issues.
      no_stat: List of builders that did not start.
      xretry: Whether we expect the change to be retried.
      pre_cq_trybot: Whether the builder is a Pre-CQ trybot.
      cl_status_url: URL of the CL status viewer for the change.
    """
    suspects = triage_lib.SuspectChanges({
        x: constants.SUSPECT_REASON_UNKNOWN for x in suspects})
    msg = cl_messages.CreateValidationFailureMessage(
        pre_cq_trybot, change, suspects, [], sanity=sanity,
        infra_fail=infra_fail, lab_fail=lab_fail, no_stat=no_stat,
        retry=xretry, cl_status_url=cl_status_url)
    for x in messages:
      self.assertTrue(x in msg)
    self.assertEqual(xretry, 'retry your change automatically' in msg)
    return msg

  def testSuspectChange(self):
    """Test case where 1 is the only change and is suspect."""
    patch = self.GetPatches(1)
    self._AssertMessage(patch, [patch], ['probably caused by your change'])

  def testInnocentChange(self):
    """Test case where 1 is innocent."""
    patch1, patch2 = self.GetPatches(2)
    self._AssertMessage(patch1, [patch2],
                        ['This failure was probably caused by',
                         'retry your change automatically'],
                        xretry=True)

  def testSuspectChanges(self):
    """Test case where 1 is suspected, but so is 2."""
    patches = self.GetPatches(2)
    self._AssertMessage(patches[0], patches,
                        ['may have caused this failure'])

  def testInnocentChangeWithMultipleSuspects(self):
    """Test case where 2 and 3 are suspected."""
    patches = self.GetPatches(3)
    self._AssertMessage(patches[0], patches[1:],
                        ['One of the following changes is probably'],
                        xretry=True)

  def testNoMessages(self):
    """Test case where there are no messages."""
    patch1 = self.GetPatches(1)
    self._AssertMessage(patch1, [patch1], [])

  def testInsaneBuild(self):
    """Test case where the build was not sane."""
    patches = self.GetPatches(3)
    self._AssertMessage(
        patches[0], patches, ['The build was consider not sane',
                              'retry your change automatically'],
        sanity=False, xretry=True)

  def testLabFailMessage(self):
    """Test case where the build failed due to lab failures."""
    patches = self.GetPatches(3)
    self._AssertMessage(
        patches[0], patches, ['Lab infrastructure',
                              'retry your change automatically'],
        lab_fail=True, xretry=True)

  def testInfraFailMessage(self):
    """Test case where the build failed due to infrastructure failures."""
    patches = self.GetPatches(2)
    self._AssertMessage(
        patches[0], [patches[0]],
        ['may have been caused by infrastructure',
         'This failure was probably caused by your change'],
        infra_fail=True)
    self._AssertMessage(
        patches[1], [patches[0]], ['may have been caused by infrastructure',
                                   'retry your change automatically'],
        infra_fail=True, xretry=True)

  def testPreCQFailMessage(self):
    """Test case where the build failed in pre-CQ."""
    patches = self.GetPatches(3)
    self._AssertMessage(
        patches[0], patches,
        ['We notify the first failure only',
         'Please find the full status at http://example.com/.'],
        pre_cq_trybot=True,
        cl_status_url='http://example.com/')
