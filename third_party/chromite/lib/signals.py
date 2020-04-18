# -*- coding: utf-8 -*-
# Copyright (c) 2011-2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Signal related functionality."""

from __future__ import print_function

import signal
import contextlib


def RelaySignal(handler, signum, frame):
  """Notify a listener returned from getsignal of receipt of a signal.

  Returns:
    True if it was relayed to the target, False otherwise.
    False in particular occurs if the target isn't relayable.
  """
  if handler in (None, signal.SIG_IGN):
    return True
  elif handler == signal.SIG_DFL:
    # This scenario is a fairly painful to handle fully, thus we just
    # state we couldn't handle it and leave it to client code.
    return False
  handler(signum, frame)
  return True


def SignalModuleUsable(_signal=signal.signal, _SIGUSR1=signal.SIGUSR1):
  """Verify that the signal module is usable and won't segfault on us.

  See http://bugs.python.org/issue14173.  This function detects if the
  signals module is no longer safe to use (which only occurs during
  final stages of the interpreter shutdown) and heads off a segfault
  if signal.* was accessed.

  This shouldn't be used by anything other than functionality that is
  known and unavoidably invoked by finalizer code during python shutdown.

  Finally, the default args here are intentionally binding what we need
  from the signal module to do the necessary test; invoking code shouldn't
  pass any options, nor should any developer ever remove those default
  options.

  Note that this functionality is intended to be removed just as soon
  as all consuming code installs their own SIGTERM handlers.
  """
  # Track any signals we receive while doing the check.
  received, actual = [], None
  def handler(signum, frame):
    received.append([signum, frame])
  try:
    # Play with sigusr1, since it's not particularly used.
    actual = _signal(_SIGUSR1, handler)
    _signal(_SIGUSR1, actual)
    return True
  except (TypeError, AttributeError, SystemError, ValueError):
    # The first three exceptions can be thrown depending on the state of the
    # signal module internal Handlers array; we catch all, and interpret it
    # as if we were invoked during sys.exit cleanup.
    # The last exception can be thrown if we're trying to be used in a thread
    # which is not the main one.  This can come up with standard python modules
    # such as BaseHTTPServer.HTTPServer.
    return False
  finally:
    # And now relay those signals to the original handler.  Not all may
    # be delivered- the first may throw an exception for example.  Not our
    # problem however.
    for signum, frame in received:
      actual(signum, frame)


@contextlib.contextmanager
def DeferSignals(*args):
  """Context Manger to defer signals during a critical block.

  If a signal comes in for the masked signals, the original handler
  is ran after the  critical block has exited.

  Args:
    args: Which signals to ignore.  If none are given, defaults to
      SIGINT and SIGTERM.
  """
  signals = args
  if not signals:
    signals = [signal.SIGINT, signal.SIGTERM, signal.SIGALRM]

  # Rather than directly setting the handler, we first pull the handlers, then
  # set the new handler.  The ordering has to be done this way to ensure that
  # if someone passes in a bad signum (or a signal lands prior to starting the
  # critical block), we can restore things to pristine state.
  handlers = dict((signum, signal.getsignal(signum)) for signum in signals)

  received = []
  def handler(signum, frame):
    received.append((signum, frame))

  try:
    for signum in signals:
      signal.signal(signum, handler)

    yield

  finally:
    for signum, original in handlers.iteritems():
      signal.signal(signum, original)

    for signum, frame in received:
      RelaySignal(handlers[signum], signum, frame)


def StrSignal(sig_num):
  """Convert a signal number to the symbolic name

  Note: Some signal number have multiple names, so you might get
  back a confusing result like "SIGIOT|SIGABRT".  Since they have
  the same signal number, it's impossible to say which one is right.

  Args:
    sig_num: The numeric signal you wish to convert

  Returns:
    A string of the signal name(s)
  """
  # Handle realtime signals first since they are unnamed.
  if sig_num >= signal.SIGRTMIN and sig_num < signal.SIGRTMAX:
    return 'SIGRT_%i' % sig_num

  # Probe the module looking for matching signal constant.
  sig_names = []
  for name, num in signal.__dict__.iteritems():
    if name.startswith('SIG') and num == sig_num:
      sig_names.append(name)
  if sig_names:
    return '|'.join(sig_names)
  else:
    return 'SIG_%i' % sig_num
