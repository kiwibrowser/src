# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Fake out system python modules that are not available on AE.

Chromite imports some standard python modules that are not available on the
restricted sandbox environment on appengine. Fake out these modules such that
imports don't break, but any attempt to use the modules blows up obviously.
"""

from __future__ import print_function

import os
import sys

# Some chromite modules that we import have transitive dependencies on various
# other modules that cannot be imported on appengine. The following hacks are to
# ensure that those import statements silently do nothing and succeed.

class Fake(object):
  """A fake class that does nothing."""

  def __call__(self, *args, **kwargs):
    """Allow arbitrary calls, doing nothing."""

  def __iter__(self):
    return self

  def next(self):
    """All iterations are 0 lengths."""
    raise StopIteration()

  def __getattr__(self, key):
    return Fake()

  def __hasattr__(self, key):
    return True


_FORBIDDEN_MODULES = (
    'ctypes',
    'ctypes.util',
    'google.protobuf',
    'multiprocessing',
    'multiprocessing.managers',
    'pwd',
    'ssl',
    'subprocess',
)

for m in _FORBIDDEN_MODULES:
  sys.modules[m] = Fake()

import google
google.protobuf = Fake()

sys.modules['subprocess'].Popen = Fake
sys.modules['multiprocessing'].Process = Fake
sys.modules['multiprocessing.managers'].SyncManager = Fake


# We can't avoid importing os.path, but we still need to make sure that calls to
# os.path.expanduser don't blow up.

FAKE_HOMEDIR = '/tmp/an_obviously_non_existent_home_dir'
def _expanduser(_):
  """A fake implementation of os.path.expanduser.

  os.path.expanduser needs to import 'pwd' that is not available on appengine.
  In fact, the concept of HOMEDIR doesn't make sense at all. So, patch it to
  return a safe fake path. If we try to use it anywhere, it will fail obviously.
  """
  return FAKE_HOMEDIR

# Importing this module has the side effect of patching all of the following
# library modules / classes / functions.
os.path.expanduser = _expanduser

fake_signal = Fake()
# pylint: disable=attribute-defined-outside-init
fake_signal.signal = Fake()
fake_signal.SIGUSR1 = 30
fake_signal.SIGHUP = 1
sys.modules['signal'] = fake_signal

fake_pkg_resources = Fake()
fake_pkg_resources.declare_namespace = Fake()
fake_pkg_resources.fixup_namespace_packages = Fake()
sys.modules['pkg_resources'] = fake_pkg_resources
