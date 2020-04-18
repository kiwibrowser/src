# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

class MockFunction(object):
  '''Decorates a function to record the number of times it's called, and
  use that to make test assertions.

  Use like:

  @MockFunction
  def my_function(): pass
  my_function()
  my_function()
  self.assertTrue(*my_function.CheckAndReset(2))

  or

  my_constructor = MockFunction(HTMLParser)
  my_constructor()
  self.assertTrue(*my_constructor.CheckAndReset(1))

  and so on.
  '''

  def __init__(self, fn):
    self._fn = fn
    self._call_count = 0

  def __call__(self, *args, **optargs):
    self._call_count += 1
    return self._fn(*args, **optargs)

  def CheckAndReset(self, expected_call_count):
    actual_call_count = self._call_count
    self._call_count = 0
    if expected_call_count == actual_call_count:
      return True, ''
    return (False, '%s: expected %s call(s), got %s' %
                   (self._fn.__name__, expected_call_count, actual_call_count))
