# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for dry_run library."""

from __future__ import print_function

from chromite.lib import cros_test_lib
from chromite.lib.paygen import dryrun_lib


# pylint: disable=W0212


class FuncClass(object):
  """Helper class with a Func to call."""
  @staticmethod
  def Func(func, *args, **kwargs):
    """Dummy function."""


class TestDryRunMgr(cros_test_lib.MoxTestCase):
  """Test cases for dryrun_lib."""

  def testNonzero(self):
    self.assertTrue(dryrun_lib.DryRunMgr(True))
    self.assertFalse(dryrun_lib.DryRunMgr(False))

  def testCall(self):
    self.mox.StubOutWithMock(FuncClass, 'Func')
    drm = dryrun_lib.DryRunMgr(False)

    # Set up the test replay script.
    FuncClass.Func('arg1', 'arg2', keya='arga')
    self.mox.ReplayAll()

    # Run the test verification.
    drm._Call(FuncClass.Func, 'arg1', 'arg2', keya='arga')
    self.mox.VerifyAll()

  def testSkip(self):
    self.mox.StubOutWithMock(FuncClass, 'Func')
    drm = dryrun_lib.DryRunMgr(True)

    # Set up the test replay script.
    self.mox.ReplayAll()

    # Run the test verification.
    drm._Skip(FuncClass.Func, 'arg1', 'arg2', keya='arga')
    self.mox.VerifyAll()

  def testRunCall(self):
    mocked_drm = self.mox.CreateMock(dryrun_lib.DryRunMgr)
    mocked_drm.dry_run = False
    mocked_drm.quiet = False

    args = ['arg1', 'arg2']
    kwargs = {'keya': 'arga', 'keyb': 'argb'}

    # Set up the test replay script.
    mocked_drm._Call(FuncClass.Func, *args, **kwargs)
    self.mox.ReplayAll()

    # Run the test verification.
    dryrun_lib.DryRunMgr.Run(mocked_drm, FuncClass.Func,
                             *args, **kwargs)
    self.mox.VerifyAll()

  def testRunSkip(self):
    mocked_drm = self.mox.CreateMock(dryrun_lib.DryRunMgr)
    mocked_drm.dry_run = True
    mocked_drm.quiet = False

    args = ['arg1', 'arg2']
    kwargs = {'keya': 'arga', 'keyb': 'argb'}

    # Set up the test replay script.
    func_path = '%s.%s' % (FuncClass.Func.__module__, FuncClass.Func.__name__)
    mocked_drm._Skip(func_path, *args, **kwargs)
    self.mox.ReplayAll()

    # Run the test verification.
    dryrun_lib.DryRunMgr.Run(mocked_drm, FuncClass.Func,
                             *args, **kwargs)
    self.mox.VerifyAll()
