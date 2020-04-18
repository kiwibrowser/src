# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Process related utilities."""

from __future__ import print_function

import errno
import os
import signal
import sys
import time


def GetExitStatus(status):
  """Get the exit status of a child from an os.waitpid call.

  Args:
    status: The return value of os.waitpid(pid, 0)[1]

  Returns:
    The exit status of the process. If the process exited with a signal,
    the return value will be 128 plus the signal number.
  """
  if os.WIFSIGNALED(status):
    return 128 + os.WTERMSIG(status)
  else:
    assert os.WIFEXITED(status), 'Unexpected exit status %r' % status
    return os.WEXITSTATUS(status)


def ExitAsStatus(status):
  """Exit the same way as |status|.

  If the status field says it was killed by a signal, then we'll do that to
  ourselves.  Otherwise we'll exit with the exit code.

  See http://www.cons.org/cracauer/sigint.html for more details.

  Args:
    status: A status as returned by os.wait type funcs.
  """
  exit_status = os.WEXITSTATUS(status)

  if os.WIFSIGNALED(status):
    # Kill ourselves with the same signal.
    sig_status = os.WTERMSIG(status)
    pid = os.getpid()
    os.kill(pid, sig_status)
    time.sleep(0.1)

    # Still here?  Maybe the signal was masked.
    try:
      signal.signal(sig_status, signal.SIG_DFL)
    except RuntimeError as e:
      if e.args[0] != errno.EINVAL:
        raise
    os.kill(pid, sig_status)
    time.sleep(0.1)

    # Still here?  Just exit.
    exit_status = 127

  # Exit with the code we want.
  sys.exit(exit_status)
