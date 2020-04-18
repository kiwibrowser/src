#!/usr/bin/python
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Testing Library For Nacl.

"""

import atexit
import difflib
import os
import re
import shutil
import signal
import subprocess
import sys
import tempfile
import threading

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import pynacl.platform


# Windows does not fully implement os.times functionality.  If
# _GetTimesPosix were used, the fields for CPU time used in user and
# in kernel mode by the process will both be zero.  Instead, we must
# use the ctypes module and access windll's kernel32 interface to
# extract the CPU usage information.

if sys.platform[:3] == 'win':
  import ctypes


class SubprocessCpuTimer:
  """Timer used to measure user and kernel CPU time expended by a subprocess.

  A new object of this class should be instantiated just before the
  subprocess is created, and after the subprocess is finished and
  waited for (via the wait method of the Popen object), the elapsed
  time can be obtained by invoking the ElapsedCpuTime of the
  SubprocessCpuTimer instance.

  """

  WIN32_PROCESS_TIMES_TICKS_PER_SECOND = 1.0e7

  # use a class variable to avoid slicing at run-time
  _use_proc_handle = sys.platform[:3] == 'win'


  @staticmethod
  def _GetTimePosix():
    try:
      t = os.times()
    except OSError:
      # This works around a bug in the calling conventions for the
      # times() system call on Linux.  This syscall returns a number
      # of clock ticks since an arbitrary time in the past, but if
      # this happens to be between -4095 and -1, it is interpreted as
      # an errno value, and we get an exception here.
      # Returning 0 as a dummy value may result in ElapsedCpuTime()
      # below returning a negative value.  This is OK for our use
      # because a test that takes too long is likely to be caught
      # elsewhere.
      if sys.platform == "linux2":
        return 0
      raise
    else:
      return t[2] + t[3]


  @staticmethod
  def _GetTimeWindows(proc_handle):
    if proc_handle is None:
      return 0
    creation_time = ctypes.c_ulonglong()
    exit_time = ctypes.c_ulonglong()
    kernel_time = ctypes.c_ulonglong()
    user_time = ctypes.c_ulonglong()
    rc = ctypes.windll.kernel32.GetProcessTimes(
        int(proc_handle._handle),
        ctypes.byref(creation_time),
        ctypes.byref(exit_time),
        ctypes.byref(kernel_time),
        ctypes.byref(user_time))
    if not rc:
      print >>sys.stderr, 'Could not obtain process time'
      return 0
    return ((kernel_time.value + user_time.value)
            / SubprocessCpuTimer.WIN32_PROCESS_TIMES_TICKS_PER_SECOND)


  @staticmethod
  def _GetTime(proc_handle):
    if SubprocessCpuTimer._use_proc_handle:
      return SubprocessCpuTimer._GetTimeWindows(proc_handle)
    return SubprocessCpuTimer._GetTimePosix()


  def __init__(self):
    self._start_time = self._GetTime(None)


  def ElapsedCpuTime(self, proc_handle):
    return self._GetTime(proc_handle) - self._start_time

def PopenBufSize():
  return 1000 * 1000


def CommunicateWithTimeout(proc, input_data=None, timeout=None):
  if timeout == 0:
    timeout = None

  result = []
  def Target():
    result.append(list(proc.communicate(input_data)))

  thread = threading.Thread(target=Target)
  thread.start()
  thread.join(timeout)
  if thread.is_alive():
    sys.stderr.write('\nAttempting to kill test due to timeout!\n')
    # This will kill the process which should force communicate to return with
    # any partial output.
    pynacl.platform.KillSubprocessAndChildren(proc)
    # Thus result should ALWAYS contain something after this join.
    thread.join()
    sys.stderr.write('\n\nKilled test due to timeout!\n')
    # Also append to stderr.
    result[0][1] += '\n\nKilled test due to timeout!\n'
    returncode = -9
  else:
    returncode = proc.returncode
  assert len(result) == 1
  return tuple(result[0]) + (returncode,)


def RunTestWithInput(cmd, input_data, timeout=None):
  """Run a test where we only care about the return code."""
  assert type(cmd) == list
  failed = 0
  timer = SubprocessCpuTimer()
  p = None
  try:
    sys.stdout.flush() # Make sure stdout stays in sync on the bots.
    if type(input_data) == str:
      p = subprocess.Popen(cmd,
                           bufsize=PopenBufSize(),
                           stdin=subprocess.PIPE)
      _, _, retcode = CommunicateWithTimeout(p, input_data, timeout=timeout)
    else:
      p = subprocess.Popen(cmd,
                           bufsize=PopenBufSize(),
                           stdin=input_data)
      _, _, retcode = CommunicateWithTimeout(p, timeout=timeout)
  except OSError:
    print 'exception: ' + str(sys.exc_info()[1])
    retcode = 0
    failed = 1

  if p is None:
    return (0, 0, 1)
  return (timer.ElapsedCpuTime(p), retcode, failed)


def RunTestWithInputOutput(cmd, input_data, capture_stderr=True, timeout=None):
  """Run a test where we also care about stdin/stdout/stderr.

  NOTE: this function may have problems with arbitrarily
        large input or output, especially on windows
  NOTE: input_data can be either a string or or a file like object,
        file like objects may be better for large input/output
  """
  assert type(cmd) == list
  stdout = ''
  stderr = ''
  failed = 0

  p = None
  timer = SubprocessCpuTimer()
  try:
    # Python on windows does not include any notion of SIGPIPE.  On
    # Linux and OSX, Python installs a signal handler for SIGPIPE that
    # sets the handler to SIG_IGN so that syscalls return -1 with
    # errno equal to EPIPE, and translates those to exceptions;
    # unfortunately, the subprocess module fails to reset the handler
    # for SIGPIPE to SIG_DFL, and the SIG_IGN behavior is inherited.
    # subprocess.Popen's preexec_fn is apparently okay to use on
    # Windows, as long as its value is None.

    if hasattr(signal, 'SIGPIPE'):
      no_pipe = lambda : signal.signal(signal.SIGPIPE, signal.SIG_DFL)
    else:
      no_pipe = None

    # Only capture stderr if capture_stderr is true
    p_stderr = subprocess.PIPE if capture_stderr else None

    if type(input_data) == str:
      p = subprocess.Popen(cmd,
                           bufsize=PopenBufSize(),
                           stdin=subprocess.PIPE,
                           stderr=p_stderr,
                           stdout=subprocess.PIPE,
                           preexec_fn = no_pipe)
      stdout, stderr, retcode = CommunicateWithTimeout(
          p, input_data, timeout=timeout)
    else:
      # input_data is a file like object
      p = subprocess.Popen(cmd,
                           bufsize=PopenBufSize(),
                           stdin=input_data,
                           stderr=p_stderr,
                           stdout=subprocess.PIPE,
                           preexec_fn = no_pipe)
      stdout, stderr, retcode = CommunicateWithTimeout(p, timeout=timeout)
  except OSError, x:
    if x.errno == 10:
      print '@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@'
      print 'ignoring exception', str(sys.exc_info()[1])
      print 'return code NOT checked'
      print 'this seems to be a windows issue'
      print '@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@'
      failed = 0
      retcode = 0
    else:
      print 'exception: ' + str(sys.exc_info()[1])
      retcode = 0
      failed = 1
  if p is None:
    cpu_time_consumed = 0
  else:
    cpu_time_consumed = timer.ElapsedCpuTime(p)
  return (cpu_time_consumed, retcode, failed, stdout, stderr)


def DiffStringsIgnoringWhiteSpace(a, b):
  a = a.splitlines()
  b = b.splitlines()
  # NOTE: the whitespace stuff seems to be broken in python
  cruncher = difflib.SequenceMatcher(lambda x: x in ' \t\r', a, b)

  for group in cruncher.get_grouped_opcodes():
    eq = True
    for tag, i1, i2, j1, j2 in group:
      if tag != 'equal':
        eq = False
        break
    if eq: continue
    i1, i2, j1, j2 = group[0][1], group[-1][2], group[0][3], group[-1][4]
    yield '@@ -%d,%d +%d,%d @@\n' % (i1+1, i2-i1, j1+1, j2-j1)

    for tag, i1, i2, j1, j2 in group:
      if tag == 'equal':
        for line in a[i1:i2]:
          yield ' [' + line + ']'
        continue
      if tag == 'replace' or tag == 'delete':
        for line in a[i1:i2]:
          yield '-[' + repr(line) + ']'
      if tag == 'replace' or tag == 'insert':
        for line in b[j1:j2]:
          yield '+[' + repr(line) + ']'


def RegexpFilterLines(regexp, inverse, group_only, lines):
  """Apply regexp to filter lines of text, keeping only those lines
  that match.

  Any carriage return / newline sequence is turned into a newline.

  Args:
    regexp: A regular expression, only lines that match are kept
    inverse: Only keep lines that do not match
    group_only: replace matching lines with the regexp groups,
                text outside the groups are omitted, useful for
                eliminating file names that might change, etc).

    lines: A string containing newline-separated lines of text

  Return:
    Filtered lines of text, newline separated.
  """

  result = []
  nfa = re.compile(regexp)
  for line in lines.split('\n'):
    if line.endswith('\r'):
      line = line[:-1]
    mobj = nfa.search(line)
    if mobj and inverse:
      continue
    if not mobj and not inverse:
      continue

    if group_only:
      matched_strings = []
      for s in mobj.groups():
        if s is not None:
          matched_strings.append(s)
      result.append(''.join(matched_strings))
    else:
      result.append(line)

  return '\n'.join(result)


def MakeTempDir(env, **kwargs):
  """Create a temporary directory and arrange to clean it up on exit.

  Passes arguments through to tempfile.mkdtemp
  """
  temporary_dir = tempfile.mkdtemp(**kwargs)
  def Cleanup():
    try:
      # Try to remove the dir but only if it exists. Some tests may clean up
      # after themselves.
      if os.path.exists(temporary_dir):
        shutil.rmtree(temporary_dir)
    except BaseException as e:
      sys.stderr.write('Unable to delete dir %s on exit: %s\n' % (
        temporary_dir, e))
  atexit.register(Cleanup)
  return temporary_dir

def MakeTempFile(env, **kwargs):
  """Create a temporary file and arrange to clean it up on exit.

  Passes arguments through to tempfile.mkstemp
  """
  handle, path = tempfile.mkstemp(**kwargs)
  def Cleanup():
    try:
      # Try to remove the file but only if it exists. Some tests may clean up
      # after themselves.
      if os.path.exists(path):
        os.unlink(path)
    except BaseException as e:
      sys.stderr.write('Unable to delete file %s on exit: %s\n' % (
        path, e))
  atexit.register(Cleanup)
  return handle, path
