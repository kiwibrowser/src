# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module that handles tee-ing output to a file."""

from __future__ import print_function

import errno
import fcntl
import os
import multiprocessing
import select
import signal
import subprocess
import sys
import traceback

from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging


# Max amount of data we're hold in the buffer at a given time.
_BUFSIZE = 1024


# Custom signal handlers so we can catch the exception and handle it.
class ToldToDie(Exception):
  """Exception thrown via signal handlers."""

  def __init__(self, signum):
    Exception.__init__(self, "We received signal %i" % (signum,))


# pylint: disable=W0613
def _TeeProcessSignalHandler(signum, frame):
  """TeeProcess custom signal handler.

  This is used to decide whether or not to kill our parent.
  """
  raise ToldToDie(signum)


def _output(line, output_files, complain):
  """Print line to output_files.

  Args:
    line: Line to print.
    output_files: List of files to print to.
    complain: Print a warning if we get EAGAIN errors. Only one error
              is printed per line.
  """
  for f in output_files:
    offset = 0
    while offset < len(line):
      select.select([], [f], [])
      try:
        offset += os.write(f.fileno(), line[offset:])
      except OSError as ex:
        if ex.errno == errno.EINTR:
          continue
        elif ex.errno != errno.EAGAIN:
          raise

      if offset < len(line) and complain:
        flags = fcntl.fcntl(f.fileno(), fcntl.F_GETFL, 0)
        if flags & os.O_NONBLOCK:
          warning = '\nWarning: %s/%d is non-blocking.\n' % (f.name,
                                                             f.fileno())
          _output(warning, output_files, False)

        warning = '\nWarning: Short write for %s/%d.\n' % (f.name, f.fileno())
        _output(warning, output_files, False)


def _tee(input_fd, output_files, complain):
  """Read data from |input_fd| and write to |output_files|."""
  while True:
    # We need to use os.read() directly because it will return to us when the
    # other side has flushed its output (and is shorter than _BUFSIZE).  If we
    # use python's file object helpers (like read() and readline()), it will
    # not return until either the full buffer is filled or a newline is hit.
    data = os.read(input_fd, _BUFSIZE)
    if not data:
      return
    _output(data, output_files, complain)


class _TeeProcess(multiprocessing.Process):
  """Replicate output to multiple file handles."""

  def __init__(self, output_filenames, complain, error_fd,
               master_pid):
    """Write to stdout and supplied filenames.

    Args:
      output_filenames: List of filenames to print to.
      complain: Print a warning if we get EAGAIN errors.
      error_fd: The fd to write exceptions/errors to during
        shutdown.
      master_pid: Pid to SIGTERM if we shutdown uncleanly.
    """

    self._reader_pipe, self.writer_pipe = os.pipe()
    self._output_filenames = output_filenames
    self._complain = complain
    # Dupe the fd on the offchance it's stdout/stderr,
    # which we screw with.
    self._error_handle = os.fdopen(os.dup(error_fd), 'w', 0)
    self.master_pid = master_pid
    multiprocessing.Process.__init__(self)

  def _CloseUnnecessaryFds(self):
    preserve = set([1, 2, self._error_handle.fileno(), self._reader_pipe,
                    subprocess.MAXFD])
    preserve = iter(sorted(preserve))
    fd = 0
    while fd < subprocess.MAXFD:
      current_low = preserve.next()
      if fd != current_low:
        os.closerange(fd, current_low)
        fd = current_low
      fd += 1

  def run(self):
    """Main function for tee subprocess."""
    failed = True
    try:
      signal.signal(signal.SIGINT, _TeeProcessSignalHandler)
      signal.signal(signal.SIGTERM, _TeeProcessSignalHandler)

      # Cleanup every fd except for what we use.
      self._CloseUnnecessaryFds()

      # Read from the pipe.
      input_fd = self._reader_pipe

      # Create list of files to write to.
      output_files = [os.fdopen(sys.stdout.fileno(), 'w', 0)]
      for filename in self._output_filenames:
        output_files.append(open(filename, 'w', 0))

      # Send all data from the one input to all the outputs.
      _tee(input_fd, output_files, self._complain)
      failed = False
    except ToldToDie:
      failed = False
    except Exception as e:
      tb = traceback.format_exc()
      logging.PrintBuildbotStepFailure(self._error_handle)
      self._error_handle.write(
          'Unhandled exception occured in tee:\n%s\n' % (tb,))
      # Try to signal the parent telling them of our
      # imminent demise.

    finally:
      # Close input.
      os.close(input_fd)

      if failed:
        try:
          os.kill(self.master_pid, signal.SIGTERM)
        except Exception as e:
          self._error_handle.write("\nTee failed signaling %s\n" % e)

      # Finally, kill ourself.
      # Specifically do it in a fashion that ensures no inherited
      # cleanup code from our parent process is ran- leave that to
      # the parent.
      # pylint: disable=W0212
      os._exit(0)


class Tee(cros_build_lib.MasterPidContextManager):
  """Class that handles tee-ing output to a file."""

  def __init__(self, output_file):
    """Initializes object with path to log file."""
    cros_build_lib.MasterPidContextManager.__init__(self)
    self._file = output_file
    self._old_stdout = None
    self._old_stderr = None
    self._old_stdout_fd = None
    self._old_stderr_fd = None
    self._tee = None

  def start(self):
    """Start tee-ing all stdout and stderr output to the file."""
    # Flush and save old file descriptors.
    sys.stdout.flush()
    sys.stderr.flush()
    self._old_stdout_fd = os.dup(sys.stdout.fileno())
    self._old_stderr_fd = os.dup(sys.stderr.fileno())
    # Save file objects
    self._old_stdout = sys.stdout
    self._old_stderr = sys.stderr

    # Replace std[out|err] with unbuffered file objects
    sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', 0)
    sys.stderr = os.fdopen(sys.stderr.fileno(), 'w', 0)

    # Create a tee subprocess.
    self._tee = _TeeProcess([self._file], True, self._old_stderr_fd,
                            os.getpid())
    self._tee.start()

    # Redirect stdout and stderr to the tee subprocess.
    writer_pipe = self._tee.writer_pipe
    os.dup2(writer_pipe, sys.stdout.fileno())
    os.dup2(writer_pipe, sys.stderr.fileno())
    os.close(writer_pipe)

  def stop(self):
    """Restores old stdout and stderr handles and waits for tee proc to exit."""
    # Close unbuffered std[out|err] file objects, as well as the tee's stdin.
    sys.stdout.close()
    sys.stderr.close()

    # Restore file objects
    sys.stdout = self._old_stdout
    sys.stderr = self._old_stderr

    # Restore old file descriptors.
    os.dup2(self._old_stdout_fd, sys.stdout.fileno())
    os.dup2(self._old_stderr_fd, sys.stderr.fileno())
    os.close(self._old_stdout_fd)
    os.close(self._old_stderr_fd)
    self._tee.join()

  def _enter(self):
    self.start()

  def _exit(self, exc_type, exc, exc_traceback):
    try:
      self.stop()
    finally:
      if self._tee is not None:
        self._tee.terminate()
