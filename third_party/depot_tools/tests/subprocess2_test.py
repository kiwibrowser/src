#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for subprocess2.py."""

import logging
import optparse
import os
import sys
import time
import unittest

try:
  import fcntl  # pylint: disable=import-error
except ImportError:
  fcntl = None

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import subprocess
import subprocess2

from testing_support import auto_stub

# Method could be a function
# pylint: disable=no-self-use


# Create aliases for subprocess2 specific tests. They shouldn't be used for
# regression tests.
TIMED_OUT = subprocess2.TIMED_OUT
VOID = subprocess2.VOID
PIPE = subprocess2.PIPE
STDOUT = subprocess2.STDOUT


def convert_to_crlf(string):
  """Unconditionally convert LF to CRLF."""
  return string.replace('\n', '\r\n')


def convert_to_cr(string):
  """Unconditionally convert LF to CR."""
  return string.replace('\n', '\r')


def convert_win(string):
  """Converts string to CRLF on Windows only."""
  if sys.platform == 'win32':
    return string.replace('\n', '\r\n')
  return string


class DefaultsTest(auto_stub.TestCase):
  # TODO(maruel): Do a reopen() on sys.__stdout__ and sys.__stderr__ so they
  # can be trapped in the child process for better coverage.
  def _fake_communicate(self):
    """Mocks subprocess2.communicate()."""
    results = {}
    def fake_communicate(args, **kwargs):
      assert not results
      results.update(kwargs)
      results['args'] = args
      return ('stdout', 'stderr'), 0
    self.mock(subprocess2, 'communicate', fake_communicate)
    return results

  def _fake_Popen(self):
    """Mocks the whole subprocess2.Popen class."""
    results = {}
    class fake_Popen(object):
      returncode = -8
      def __init__(self, args, **kwargs):
        assert not results
        results.update(kwargs)
        results['args'] = args
      @staticmethod
      # pylint: disable=redefined-builtin
      def communicate(input=None, timeout=None, nag_max=None, nag_timer=None):
        return None, None
    self.mock(subprocess2, 'Popen', fake_Popen)
    return results

  def _fake_subprocess_Popen(self):
    """Mocks the base class subprocess.Popen only."""
    results = {}
    def __init__(self, args, **kwargs):
      assert not results
      results.update(kwargs)
      results['args'] = args
    def communicate():
      return None, None
    self.mock(subprocess.Popen, '__init__', __init__)
    self.mock(subprocess.Popen, 'communicate', communicate)
    return results

  def test_check_call_defaults(self):
    results = self._fake_communicate()
    self.assertEquals(
        ('stdout', 'stderr'), subprocess2.check_call_out(['foo'], a=True))
    expected = {
        'args': ['foo'],
        'a':True,
    }
    self.assertEquals(expected, results)

  def test_capture_defaults(self):
    results = self._fake_communicate()
    self.assertEquals(
        'stdout', subprocess2.capture(['foo'], a=True))
    expected = {
        'args': ['foo'],
        'a':True,
        'stdin': subprocess2.VOID,
        'stdout': subprocess2.PIPE,
    }
    self.assertEquals(expected, results)

  def test_communicate_defaults(self):
    results = self._fake_Popen()
    self.assertEquals(
        ((None, None), -8), subprocess2.communicate(['foo'], a=True))
    expected = {
        'args': ['foo'],
        'a': True,
    }
    self.assertEquals(expected, results)

  def test_Popen_defaults(self):
    results = self._fake_subprocess_Popen()
    proc = subprocess2.Popen(['foo'], a=True)
    # Cleanup code in subprocess.py needs this member to be set.
    # pylint: disable=attribute-defined-outside-init
    proc._child_created = None
    expected = {
        'args': ['foo'],
        'a': True,
        'shell': bool(sys.platform=='win32'),
    }
    if sys.platform != 'win32':
      env = os.environ.copy()
      is_english = lambda name: env.get(name, 'en').startswith('en')
      if not is_english('LANG'):
        env['LANG'] = 'en_US.UTF-8'
        expected['env'] = env
      if not is_english('LANGUAGE'):
        env['LANGUAGE'] = 'en_US.UTF-8'
        expected['env'] = env
    self.assertEquals(expected, results)
    self.assertTrue(time.time() >= proc.start)

  def test_check_output_defaults(self):
    results = self._fake_communicate()
    # It's discarding 'stderr' because it assumes stderr=subprocess2.STDOUT but
    # fake_communicate() doesn't 'implement' that.
    self.assertEquals('stdout', subprocess2.check_output(['foo'], a=True))
    expected = {
        'args': ['foo'],
        'a':True,
        'stdin': subprocess2.VOID,
        'stdout': subprocess2.PIPE,
    }
    self.assertEquals(expected, results)


class BaseTestCase(unittest.TestCase):
  def setUp(self):
    super(BaseTestCase, self).setUp()
    self.exe_path = __file__
    self.exe = [sys.executable, self.exe_path, '--child']
    self.states = {}
    if fcntl:
      for v in (sys.stdin, sys.stdout, sys.stderr):
        fileno = v.fileno()
        self.states[fileno] = fcntl.fcntl(fileno, fcntl.F_GETFL)

  def tearDown(self):
    for fileno, fl in self.states.iteritems():
      self.assertEquals(fl, fcntl.fcntl(fileno, fcntl.F_GETFL))
    super(BaseTestCase, self).tearDown()

  def _check_res(self, res, stdout, stderr, returncode):
    (out, err), code = res
    self.assertEquals(stdout, out)
    self.assertEquals(stderr, err)
    self.assertEquals(returncode, code)


class RegressionTest(BaseTestCase):
  # Regression tests to ensure that subprocess and subprocess2 have the same
  # behavior.
  def _run_test(self, function):
    """Runs tests in 12 combinations:
    - LF output with universal_newlines=False
    - CR output with universal_newlines=False
    - CRLF output with universal_newlines=False
    - LF output with universal_newlines=True
    - CR output with universal_newlines=True
    - CRLF output with universal_newlines=True

    Once with subprocess, once with subprocess2.

    First |function| argument is the conversion for the original expected LF
    string to the right EOL.
    Second |function| argument is the executable and initial flag to run, to
    control what EOL is used by the child process.
    Third |function| argument is universal_newlines value.
    """
    noop = lambda x: x
    for subp in (subprocess, subprocess2):
      function(noop, self.exe, False, subp)
      function(convert_to_cr, self.exe + ['--cr'], False, subp)
      function(convert_to_crlf, self.exe + ['--crlf'], False, subp)
      function(noop, self.exe, True, subp)
      function(noop, self.exe + ['--cr'], True, subp)
      function(noop, self.exe + ['--crlf'], True, subp)

  def _check_exception(self, subp, e, stdout, stderr, returncode):
    """On exception, look if the exception members are set correctly."""
    self.assertEquals(returncode, e.returncode)
    if subp is subprocess:
      # subprocess never save the output.
      self.assertFalse(hasattr(e, 'stdout'))
      self.assertFalse(hasattr(e, 'stderr'))
    elif subp is subprocess2:
      self.assertEquals(stdout, e.stdout)
      self.assertEquals(stderr, e.stderr)
    else:
      self.fail()

  def test_check_output_no_stdout(self):
    try:
      subprocess2.check_output(self.exe, stdout=subprocess2.PIPE)
      self.fail()
    except ValueError:
      pass

    if (sys.version_info[0] * 10 + sys.version_info[1]) >= 27:
      # python 2.7+
      try:
        # pylint: disable=no-member
        subprocess.check_output(self.exe, stdout=subprocess.PIPE)
        self.fail()
      except ValueError:
        pass

  def test_check_output_throw_stdout(self):
    def fn(c, e, un, subp):
      if not hasattr(subp, 'check_output'):
        return
      try:
        subp.check_output(
            e + ['--fail', '--stdout'], universal_newlines=un)
        self.fail()
      except subp.CalledProcessError, exception:
        self._check_exception(subp, exception, c('A\nBB\nCCC\n'), None, 64)
    self._run_test(fn)

  def test_check_output_throw_no_stderr(self):
    def fn(c, e, un, subp):
      if not hasattr(subp, 'check_output'):
        return
      try:
        subp.check_output(
            e + ['--fail', '--stderr'], universal_newlines=un)
        self.fail()
      except subp.CalledProcessError, exception:
        self._check_exception(subp, exception, c(''), None, 64)
    self._run_test(fn)

  def test_check_output_throw_stderr(self):
    def fn(c, e, un, subp):
      if not hasattr(subp, 'check_output'):
        return
      try:
        subp.check_output(
            e + ['--fail', '--stderr'],
            stderr=subp.PIPE,
            universal_newlines=un)
        self.fail()
      except subp.CalledProcessError, exception:
        self._check_exception(subp, exception, '', c('a\nbb\nccc\n'), 64)
    self._run_test(fn)

  def test_check_output_throw_stderr_stdout(self):
    def fn(c, e, un, subp):
      if not hasattr(subp, 'check_output'):
        return
      try:
        subp.check_output(
            e + ['--fail', '--stderr'],
            stderr=subp.STDOUT,
            universal_newlines=un)
        self.fail()
      except subp.CalledProcessError, exception:
        self._check_exception(subp, exception, c('a\nbb\nccc\n'), None, 64)
    self._run_test(fn)

  def test_check_call_throw(self):
    for subp in (subprocess, subprocess2):
      try:
        subp.check_call(self.exe + ['--fail', '--stderr'])
        self.fail()
      except subp.CalledProcessError, exception:
        self._check_exception(subp, exception, None, None, 64)

  def test_redirect_stderr_to_stdout_pipe(self):
    def fn(c, e, un, subp):
      # stderr output into stdout.
      proc = subp.Popen(
          e + ['--stderr'],
          stdout=subp.PIPE,
          stderr=subp.STDOUT,
          universal_newlines=un)
      res = proc.communicate(), proc.returncode
      self._check_res(res, c('a\nbb\nccc\n'), None, 0)
    self._run_test(fn)

  def test_redirect_stderr_to_stdout(self):
    def fn(c, e, un, subp):
      # stderr output into stdout but stdout is not piped.
      proc = subp.Popen(
          e + ['--stderr'], stderr=STDOUT, universal_newlines=un)
      res = proc.communicate(), proc.returncode
      self._check_res(res, None, None, 0)
    self._run_test(fn)

  def test_stderr(self):
    cmd = ['expr', '1', '/', '0']
    if sys.platform == 'win32':
      cmd = ['cmd.exe', '/c', 'exit', '1']
    p1 = subprocess.Popen(cmd, stderr=subprocess.PIPE, shell=False)
    p2 = subprocess2.Popen(cmd, stderr=subprocess.PIPE, shell=False)
    r1 = p1.communicate()
    r2 = p2.communicate(timeout=100)
    self.assertEquals(r1, r2)


class S2Test(BaseTestCase):
  # Tests that can only run in subprocess2, e.g. new functionalities.
  # In particular, subprocess2.communicate() doesn't exist in subprocess.
  def _run_test(self, function):
    """Runs tests in 6 combinations:
    - LF output with universal_newlines=False
    - CR output with universal_newlines=False
    - CRLF output with universal_newlines=False
    - LF output with universal_newlines=True
    - CR output with universal_newlines=True
    - CRLF output with universal_newlines=True

    First |function| argument is the conversion for the origianl expected LF
    string to the right EOL.
    Second |function| argument is the executable and initial flag to run, to
    control what EOL is used by the child process.
    Third |function| argument is universal_newlines value.
    """
    noop = lambda x: x
    function(noop, self.exe, False)
    function(convert_to_cr, self.exe + ['--cr'], False)
    function(convert_to_crlf, self.exe + ['--crlf'], False)
    function(noop, self.exe, True)
    function(noop, self.exe + ['--cr'], True)
    function(noop, self.exe + ['--crlf'], True)

  def _check_exception(self, e, stdout, stderr, returncode):
    """On exception, look if the exception members are set correctly."""
    self.assertEquals(returncode, e.returncode)
    self.assertEquals(stdout, e.stdout)
    self.assertEquals(stderr, e.stderr)

  def test_timeout(self):
    # timeout doesn't exist in subprocess.
    def fn(c, e, un):
      res = subprocess2.communicate(
          self.exe + ['--sleep_first', '--stdout'],
          timeout=0.01,
          stdout=PIPE,
          shell=False)
      self._check_res(res, '', None, TIMED_OUT)
    self._run_test(fn)

  def test_timeout_shell_throws(self):
    def fn(c, e, un):
      try:
        # With shell=True, it needs a string.
        subprocess2.communicate(' '.join(self.exe), timeout=0.01, shell=True)
        self.fail()
      except TypeError:
        pass
    self._run_test(fn)

  def test_stdin(self):
    def fn(c, e, un):
      stdin = '0123456789'
      res = subprocess2.communicate(
          e + ['--read'],
          stdin=stdin,
          universal_newlines=un)
      self._check_res(res, None, None, 10)
    self._run_test(fn)

  def test_stdin_unicode(self):
    def fn(c, e, un):
      stdin = u'0123456789'
      res = subprocess2.communicate(
          e + ['--read'],
          stdin=stdin,
          universal_newlines=un)
      self._check_res(res, None, None, 10)
    self._run_test(fn)

  def test_stdin_empty(self):
    def fn(c, e, un):
      stdin = ''
      res = subprocess2.communicate(
          e + ['--read'],
          stdin=stdin,
          universal_newlines=un)
      self._check_res(res, None, None, 0)
    self._run_test(fn)

  def test_stdin_void(self):
    res = subprocess2.communicate(self.exe + ['--read'], stdin=VOID)
    self._check_res(res, None, None, 0)

  def test_stdin_void_stdout_timeout(self):
    # Make sure a mix of VOID, PIPE and timeout works.
    def fn(c, e, un):
      res = subprocess2.communicate(
          e + ['--stdout', '--read'],
          stdin=VOID,
          stdout=PIPE,
          timeout=10,
          universal_newlines=un,
          shell=False)
      self._check_res(res, c('A\nBB\nCCC\n'), None, 0)
    self._run_test(fn)

  def test_stdout_void(self):
    def fn(c, e, un):
      res = subprocess2.communicate(
          e + ['--stdout', '--stderr'],
          stdout=VOID,
          stderr=PIPE,
          universal_newlines=un)
      self._check_res(res, None, c('a\nbb\nccc\n'), 0)
    self._run_test(fn)

  def test_stderr_void(self):
    def fn(c, e, un):
      res = subprocess2.communicate(
          e + ['--stdout', '--stderr'],
          stdout=PIPE,
          stderr=VOID,
          universal_newlines=un)
      self._check_res(res, c('A\nBB\nCCC\n'), None, 0)
    self._run_test(fn)

  def test_stdout_void_stderr_redirect(self):
    def fn(c, e, un):
      res = subprocess2.communicate(
          e + ['--stdout', '--stderr'],
          stdout=VOID,
          stderr=STDOUT,
          universal_newlines=un)
      self._check_res(res, None, None, 0)
    self._run_test(fn)

  def test_tee_stderr(self):
    def fn(c, e, un):
      stderr = []
      res = subprocess2.communicate(
          e + ['--stderr'], stderr=stderr.append, universal_newlines=un)
      self.assertEquals(c('a\nbb\nccc\n'), ''.join(stderr))
      self._check_res(res, None, None, 0)
    self._run_test(fn)

  def test_tee_stdout_stderr(self):
    def fn(c, e, un):
      stdout = []
      stderr = []
      res = subprocess2.communicate(
          e + ['--stdout', '--stderr'],
          stdout=stdout.append,
          stderr=stderr.append,
          universal_newlines=un)
      self.assertEquals(c('A\nBB\nCCC\n'), ''.join(stdout))
      self.assertEquals(c('a\nbb\nccc\n'), ''.join(stderr))
      self._check_res(res, None, None, 0)
    self._run_test(fn)

  def test_tee_stdin(self):
    def fn(c, e, un):
      # Mix of stdin input and stdout callback.
      stdout = []
      stdin = '0123456789'
      res = subprocess2.communicate(
          e + ['--stdout', '--read'],
          stdin=stdin,
          stdout=stdout.append,
          universal_newlines=un)
      self.assertEquals(c('A\nBB\nCCC\n'), ''.join(stdout))
      self._check_res(res, None, None, 10)
    self._run_test(fn)

  def test_tee_throw(self):
    def fn(c, e, un):
      # Make sure failure still returns stderr completely.
      stderr = []
      try:
        subprocess2.check_output(
            e + ['--stderr', '--fail'],
            stderr=stderr.append,
            universal_newlines=un)
        self.fail()
      except subprocess2.CalledProcessError, exception:
        self._check_exception(exception, '', None, 64)
        self.assertEquals(c('a\nbb\nccc\n'), ''.join(stderr))
    self._run_test(fn)

  def test_tee_timeout_stdout_void(self):
    def fn(c, e, un):
      stderr = []
      res = subprocess2.communicate(
          e + ['--stdout', '--stderr', '--fail'],
          stdout=VOID,
          stderr=stderr.append,
          shell=False,
          timeout=10,
          universal_newlines=un)
      self._check_res(res, None, None, 64)
      self.assertEquals(c('a\nbb\nccc\n'), ''.join(stderr))
    self._run_test(fn)

  def test_tee_timeout_stderr_void(self):
    def fn(c, e, un):
      stdout = []
      res = subprocess2.communicate(
          e + ['--stdout', '--stderr', '--fail'],
          stdout=stdout.append,
          stderr=VOID,
          shell=False,
          timeout=10,
          universal_newlines=un)
      self._check_res(res, None, None, 64)
      self.assertEquals(c('A\nBB\nCCC\n'), ''.join(stdout))
    self._run_test(fn)

  def test_tee_timeout_stderr_stdout(self):
    def fn(c, e, un):
      stdout = []
      res = subprocess2.communicate(
          e + ['--stdout', '--stderr', '--fail'],
          stdout=stdout.append,
          stderr=STDOUT,
          shell=False,
          timeout=10,
          universal_newlines=un)
      self._check_res(res, None, None, 64)
      # Ordering is random due to buffering.
      self.assertEquals(
          set(c('a\nbb\nccc\nA\nBB\nCCC\n').splitlines(True)),
          set(''.join(stdout).splitlines(True)))
    self._run_test(fn)

  def test_tee_large(self):
    stdout = []
    # Read 128kb. On my workstation it takes >2s. Welcome to 2011.
    res = subprocess2.communicate(self.exe + ['--large'], stdout=stdout.append)
    self.assertEquals(128*1024, len(''.join(stdout)))
    self._check_res(res, None, None, 0)

  def test_tee_large_stdin(self):
    stdout = []
    # Write 128kb.
    stdin = '0123456789abcdef' * (8*1024)
    res = subprocess2.communicate(
        self.exe + ['--large', '--read'], stdin=stdin, stdout=stdout.append)
    self.assertEquals(128*1024, len(''.join(stdout)))
    # Windows return code is > 8 bits.
    returncode = len(stdin) if sys.platform == 'win32' else 0
    self._check_res(res, None, None, returncode)

  def test_tee_cb_throw(self):
    # Having a callback throwing up should not cause side-effects. It's a bit
    # hard to measure.
    class Blow(Exception):
      pass
    def blow(_):
      raise Blow()
    proc = subprocess2.Popen(self.exe + ['--stdout'], stdout=blow)
    try:
      proc.communicate()
      self.fail()
    except Blow:
      self.assertNotEquals(0, proc.returncode)

  def test_nag_timer(self):
    w = []
    l = logging.getLogger()
    class _Filter(logging.Filter):
      def filter(self, record):
        if record.levelno == logging.WARNING:
          w.append(record.getMessage().lstrip())
        return 0
    f = _Filter()
    l.addFilter(f)
    proc = subprocess2.Popen(
        self.exe + ['--stdout', '--sleep_first'], stdout=PIPE)
    res = proc.communicate(nag_timer=3), proc.returncode
    l.removeFilter(f)
    self._check_res(res, 'A\nBB\nCCC\n', None, 0)
    expected = ['No output for 3 seconds from command:', proc.cmd_str,
                'No output for 6 seconds from command:', proc.cmd_str,
                'No output for 9 seconds from command:', proc.cmd_str]
    self.assertEquals(w, expected)


def child_main(args):
  if sys.platform == 'win32':
    # Annoying, make sure the output is not translated on Windows.
    # pylint: disable=no-member,import-error
    import msvcrt
    msvcrt.setmode(sys.stdout.fileno(), os.O_BINARY)
    msvcrt.setmode(sys.stderr.fileno(), os.O_BINARY)

  parser = optparse.OptionParser()
  parser.add_option(
      '--fail',
      dest='return_value',
      action='store_const',
      default=0,
      const=64)
  parser.add_option(
      '--crlf', action='store_const', const='\r\n', dest='eol', default='\n')
  parser.add_option(
      '--cr', action='store_const', const='\r', dest='eol')
  parser.add_option('--stdout', action='store_true')
  parser.add_option('--stderr', action='store_true')
  parser.add_option('--sleep_first', action='store_true')
  parser.add_option('--sleep_last', action='store_true')
  parser.add_option('--large', action='store_true')
  parser.add_option('--read', action='store_true')
  options, args = parser.parse_args(args)
  if args:
    parser.error('Internal error')
  if options.sleep_first:
    time.sleep(10)

  def do(string):
    if options.stdout:
      sys.stdout.write(string.upper())
      sys.stdout.write(options.eol)
    if options.stderr:
      sys.stderr.write(string.lower())
      sys.stderr.write(options.eol)

  do('A')
  do('BB')
  do('CCC')
  if options.large:
    # Print 128kb.
    string = '0123456789abcdef' * (8*1024)
    sys.stdout.write(string)
  if options.read:
    assert options.return_value is 0
    try:
      while sys.stdin.read(1):
        options.return_value += 1
    except OSError:
      pass
  if options.sleep_last:
    time.sleep(10)
  return options.return_value


if __name__ == '__main__':
  logging.basicConfig(level=
      [logging.WARNING, logging.INFO, logging.DEBUG][
        min(2, sys.argv.count('-v'))])
  if len(sys.argv) > 1 and sys.argv[1] == '--child':
    sys.exit(child_main(sys.argv[2:]))
  unittest.main()
