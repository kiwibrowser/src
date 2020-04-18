# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for the namespaces.py module."""

from __future__ import print_function

import errno
import os
import unittest

from chromite.lib import cros_test_lib
from chromite.lib import namespaces


class SetNSTests(cros_test_lib.TestCase):
  """Tests for SetNS()"""

  def testBasic(self):
    """Simple functionality test."""
    NS_PATH = '/proc/self/ns/mnt'
    if not os.path.exists(NS_PATH):
      raise unittest.SkipTest('kernel too old (missing %s)' % NS_PATH)

    with open(NS_PATH) as f:
      try:
        namespaces.SetNS(f.fileno(), 0)
      except OSError as e:
        if e.errno != errno.EPERM:
          # Running as non-root will fail, so ignore it.  We ran most
          # of the code in the process which is all we really wanted.
          raise


class UnshareTests(cros_test_lib.TestCase):
  """Tests for Unshare()"""

  def testBasic(self):
    """Simple functionality test."""
    try:
      namespaces.Unshare(namespaces.CLONE_NEWNS)
    except OSError as e:
      if e.errno != errno.EPERM:
        # Running as non-root will fail, so ignore it.  We ran most
        # of the code in the process which is all we really wanted.
        raise
