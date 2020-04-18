# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time


class Timer(object):
  '''A simple timer which starts when constructed and stops when Stop is called.
  '''

  def __init__(self):
    self._start = time.time()
    self._elapsed = None

  def Stop(self):
    '''Stops the timer. Must only be called once. Returns |self|.
    '''
    assert self._elapsed is None
    self._elapsed = time.time() - self._start
    return self

  def With(self, other):
    '''Returns a new stopped Timer with this Timer's elapsed time + |other|'s.
    Both Timers must already be stopped.
    '''
    assert self._elapsed is not None
    assert other._elapsed is not None
    self_and_other = Timer()
    self_and_other._start = min(self._start, other._start)
    self_and_other._elapsed = self._elapsed + other._elapsed
    return self_and_other

  def FormatElapsed(self):
    '''Returns the elapsed time as a string in a pretty format; as a whole
    number in either seconds or milliseconds depending on which is more
    appropriate. Must already be Stopped.
    '''
    assert self._elapsed is not None
    elapsed = self._elapsed
    if elapsed < 1:
      elapsed = int(elapsed * 1000)
      unit = 'ms'
    else:
      elapsed = int(elapsed)
      unit = 'second' if elapsed == 1 else 'seconds'
    return '%s %s' % (elapsed, unit)


def TimerClosure(closure, *args, **optargs):
  '''A shorthand for timing a single function call. Returns a tuple of
  (closure return value, timer).
  '''
  timer = Timer()
  try:
    return closure(*args, **optargs), timer
  finally:
    timer.Stop()
