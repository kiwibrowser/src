# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import sys
import traceback

_no_value = object()


def _DefaultErrorHandler(error):
  raise error


def All(futures, except_pass=None, except_pass_log=False):
  '''Creates a Future which returns a list of results from each Future in
  |futures|.

  If any Future raises an error other than those in |except_pass| the returned
  Future will raise as well.

  If any Future raises an error in |except_pass| then None will be inserted as
  its result. If |except_pass_log| is True then the exception will be logged.
  '''
  def resolve():
    resolved = []
    for f in futures:
      try:
        resolved.append(f.Get())
      # "except None" will simply not catch any errors.
      except except_pass:
        if except_pass_log:
          logging.error(traceback.format_exc())
        resolved.append(None)
        pass
    return resolved
  return Future(callback=resolve)


def Race(futures, except_pass=None, default=_no_value):
  '''Returns a Future which resolves to the first Future in |futures| that
  either succeeds or throws an error apart from those in |except_pass|.

  If all Futures throw errors in |except_pass| then |default| is returned,
  if specified. If |default| is not specified then one of the passed errors
  will be re-thrown, for a nice stack trace.
  '''
  def resolve():
    first_future = None
    for future in futures:
      if first_future is None:
        first_future = future
      try:
        return future.Get()
      # "except None" will simply not catch any errors.
      except except_pass:
        pass
    if default is not _no_value:
      return default
    # Everything failed and there is no default value, propagate the first
    # error even though it was caught by |except_pass|.
    return first_future.Get()
  return Future(callback=resolve)


class Future(object):
  '''Stores a value, error, or callback to be used later.
  '''
  def __init__(self, value=_no_value, callback=None, exc_info=None):
    self._value = value
    self._callback = callback
    self._exc_info = exc_info
    if (self._value is _no_value and
        self._callback is None and
        self._exc_info is None):
      raise ValueError('Must have either a value, error, or callback.')

  def Then(self, callback, error_handler=_DefaultErrorHandler):
    '''Creates and returns a future that runs |callback| on the value of this
    future, or runs optional |error_handler| if resolving this future results in
    an exception.

    If |callback| returns a non-Future value then the returned Future will
    resolve to that value.

    If |callback| returns a Future then it gets chained to the current Future.
    This means that the returned Future will resolve to *that* Future's value.
    This behaviour is transitive.

    For example,

      def fortytwo():
        return Future(value=42)

      def inc(x):
        return x + 1

      def inc_future(x):
        return Future(value=x + 1)

    fortytwo().Then(inc).Get()                         ==> 43
    fortytwo().Then(inc_future).Get()                  ==> 43
    fortytwo().Then(inc_future).Then(inc_future).Get() ==> 44
    '''
    def then():
      val = None
      try:
        val = self.Get()
      except Exception as e:
        val = error_handler(e)
      else:
        val = callback(val)
      return val.Get() if isinstance(val, Future) else val
    return Future(callback=then)

  def Get(self):
    '''Gets the stored value, error, or callback contents.
    '''
    if self._value is not _no_value:
      return self._value
    if self._exc_info is not None:
      self._Raise()
    try:
      self._value = self._callback()
      return self._value
    except:
      self._exc_info = sys.exc_info()
      self._Raise()

  def _Raise(self):
    exc_info = self._exc_info
    raise exc_info[0], exc_info[1], exc_info[2]
