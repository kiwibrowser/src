#!/usr/bin/env python
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=protected-access

import time
import unittest

from devil import devil_env
from devil.utils import lazy
from devil.utils import timeout_retry

with devil_env.SysPath(devil_env.PYMOCK_PATH):
  import mock


class DynamicSideEffect(object):
  """A helper object for handling a sequence of single-use side effects."""

  def __init__(self, side_effects):
    self._side_effects = iter(side_effects or [])

  def __call__(self):
    val = next(self._side_effects)()
    if isinstance(val, Exception):
      raise val
    return val


class WeakConstantTest(unittest.TestCase):

  def testUninitialized(self):
    """Ensure that the first read calls the initializer."""
    initializer = mock.Mock(return_value='initializer called')
    test_constant = lazy.WeakConstant(initializer)
    self.assertEquals(
        'initializer called',
        test_constant.read())
    initializer.assert_called_once()

  def testInitialized(self):
    """Ensure that reading doesn't reinitialize the value."""
    initializer = mock.Mock(return_value='initializer called')
    test_constant = lazy.WeakConstant(initializer)
    test_constant._initialized.set()
    test_constant._val = 'initializer not called'
    self.assertEquals(
        'initializer not called',
        test_constant.read())
    initializer.assert_not_called()

  def testFirstCallHangs(self):
    """Ensure that reading works even if the first initializer call hangs."""
    dyn = DynamicSideEffect([
        lambda: time.sleep(10),
        lambda: 'second try worked!'
    ])

    initializer = mock.Mock(side_effect=dyn)
    test_constant = lazy.WeakConstant(initializer)
    self.assertEquals(
        'second try worked!',
        timeout_retry.Run(test_constant.read, 1, 1))
    initializer.assert_has_calls([mock.call(), mock.call()])


if __name__ == '__main__':
  unittest.main()
