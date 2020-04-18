# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test the cros_build_lib module."""

from __future__ import print_function

import collections
import contextlib
import datetime
import difflib
import errno
import functools
import itertools
import mock
import os
import signal
import socket
import StringIO
import sys
import __builtin__

from chromite.lib import constants
from chromite.cbuildbot import repository
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import cros_test_lib
from chromite.lib import git
from chromite.lib import osutils
from chromite.lib import partial_mock
from chromite.lib import signals as cros_signals


# pylint: disable=W0212,R0904


class RunCommandErrorStrTest(cros_test_lib.TestCase):
  """Test that RunCommandError __str__ works as expected."""

  def testNonUTF8Characters(self):
    """Test that non-UTF8 characters do not kill __str__"""
    result = cros_build_lib.RunCommand(['ls', '/does/not/exist'],
                                       error_code_ok=True)
    rce = cros_build_lib.RunCommandError('\x81', result)
    str(rce)


class TruncateStringTest(cros_test_lib.TestCase):
  """Test the TruncateStringToLine function."""

  def testTruncate(self):
    self.assertEqual(cros_build_lib.TruncateStringToLine('1234567', 5),
                     '12...')

  def testNoTruncate(self):
    self.assertEqual(cros_build_lib.TruncateStringToLine('1234567', 7),
                     '1234567')

  def testNoTruncateMultiline(self):
    self.assertEqual(cros_build_lib.TruncateStringToLine('1234567\nasdf', 7),
                     '1234567')


class CmdToStrTest(cros_test_lib.TestCase):
  """Test the CmdToStr function."""

  def setUp(self):
    self.differ = difflib.Differ()

  def _assertEqual(self, func, test_input, test_output, result):
    """Like assertEqual but with built in diff support."""
    diff = '\n'.join(list(self.differ.compare([test_output], [result])))
    msg = ('Expected %s to translate %r to %r, but got %r\n%s' %
           (func, test_input, test_output, result, diff))
    self.assertEqual(test_output, result, msg)

  def _testData(self, functor, tests, check_type=True):
    """Process a dict of test data."""
    for test_output, test_input in tests.iteritems():
      result = functor(test_input)
      self._assertEqual(functor.__name__, test_input, test_output, result)

      if check_type:
        # Also make sure the result is a string, otherwise the %r output will
        # include a "u" prefix and that is not good for logging.
        self.assertEqual(type(test_output), str)

  def testShellQuote(self):
    """Basic ShellQuote tests."""
    # Dict of expected output strings to input lists.
    tests_quote = {
        "''": '',
        'a': unicode('a'),
        "'a b c'": unicode('a b c'),
        "'a\tb'": 'a\tb',
        "'/a$file'": '/a$file',
        "'/a#file'": '/a#file',
        """'b"c'""": 'b"c',
        "'a@()b'": 'a@()b',
        'j%k': 'j%k',
        r'''"s'a\$va\\rs"''': r"s'a$va\rs",
        r'''"\\'\\\""''': r'''\'\"''',
        r'''"'\\\$"''': r"""'\$""",
    }

    # Expected input output specific to ShellUnquote. This string cannot be
    # produced by ShellQuote but is still a valid bash escaped string.
    tests_unquote = {
        r'''\$''': r'''"\\$"''',
    }

    def aux(s):
      return cros_build_lib.ShellUnquote(cros_build_lib.ShellQuote(s))

    self._testData(cros_build_lib.ShellQuote, tests_quote)
    self._testData(cros_build_lib.ShellUnquote, tests_unquote)

    # Test that the operations are reversible.
    self._testData(aux, {k: k for k in tests_quote.values()}, False)
    self._testData(aux, {k: k for k in tests_quote.keys()}, False)

  def testCmdToStr(self):
    # Dict of expected output strings to input lists.
    tests = {
        r"a b": ['a', 'b'],
        r"'a b' c": ['a b', 'c'],
        r'''a "b'c"''': ['a', "b'c"],
        r'''a "/'\$b" 'a b c' "xy'z"''':
            [unicode('a'), "/'$b", 'a b c', "xy'z"],
        '': [],
    }
    self._testData(cros_build_lib.CmdToStr, tests)


class TestRunCommandNoMock(cros_test_lib.TestCase):
  """Class that tests RunCommand by not mocking subprocess.Popen"""

  def testErrorCodeNotRaisesError(self):
    """Don't raise exception when command returns non-zero exit code."""
    result = cros_build_lib.RunCommand(['ls', '/does/not/exist'],
                                       error_code_ok=True)
    self.assertTrue(result.returncode != 0)

  def testMissingCommandRaisesError(self):
    """Raise error when command is not found."""
    self.assertRaises(cros_build_lib.RunCommandError, cros_build_lib.RunCommand,
                      ['/does/not/exist'], error_code_ok=False)
    self.assertRaises(cros_build_lib.RunCommandError, cros_build_lib.RunCommand,
                      ['/does/not/exist'], error_code_ok=True)

  def testInputString(self):
    """Verify input argument when it is a string."""
    for data in ('', 'foo', 'bar\nhigh'):
      result = cros_build_lib.RunCommand(['cat'], input=data)
      self.assertEqual(result.output, data)

  def testInputFileObject(self):
    """Verify input argument when it is a file object."""
    result = cros_build_lib.RunCommand(['cat'], input=open('/dev/null'))
    self.assertEqual(result.output, '')

    result = cros_build_lib.RunCommand(['cat'], input=open(__file__))
    self.assertEqual(result.output, osutils.ReadFile(__file__))

  def testInputFileDescriptor(self):
    """Verify input argument when it is a file descriptor."""
    with open('/dev/null') as f:
      result = cros_build_lib.RunCommand(['cat'], input=f.fileno())
      self.assertEqual(result.output, '')

    with open(__file__) as f:
      result = cros_build_lib.RunCommand(['cat'], input=f.fileno())
      self.assertEqual(result.output, osutils.ReadFile(__file__))


def _ForceLoggingLevel(functor):
  def inner(*args, **kwargs):
    logger = logging.getLogger()
    current = logger.getEffectiveLevel()
    try:
      logger.setLevel(logging.INFO)
      return functor(*args, **kwargs)
    finally:
      logger.setLevel(current)
  return inner


class TestRunCommand(cros_test_lib.MockTestCase):
  """Tests of RunCommand functionality."""

  def setUp(self):
    # These ENV variables affect RunCommand behavior, hide them.
    self._old_envs = {e: os.environ.pop(e) for e in constants.ENV_PASSTHRU
                      if e in os.environ}

    # Get the original value for SIGINT so our signal() mock can return the
    # correct thing.
    self._old_sigint = signal.getsignal(signal.SIGINT)

    # Mock the return value of Popen().
    self.error = 'test error'
    self.output = 'test output'
    self.proc_mock = mock.MagicMock(
        returncode=0,
        communicate=lambda x: (self.output, self.error))
    self.popen_mock = self.PatchObject(cros_build_lib, '_Popen',
                                       return_value=self.proc_mock)

    self.signal_mock = self.PatchObject(signal, 'signal')
    self.getsignal_mock = self.PatchObject(signal, 'getsignal')
    self.PatchObject(cros_signals, 'SignalModuleUsable', return_value=True)

  def tearDown(self):
    # Restore hidden ENVs.
    os.environ.update(self._old_envs)

  @contextlib.contextmanager
  def _MockChecker(self, cmd, **kwargs):
    """Verify the mocks we set up"""
    ignore_sigint = kwargs.pop('ignore_sigint', False)

    # Make some arbitrary functors we can pretend are signal handlers.
    # Note that these are intentionally defined on the fly via lambda-
    # this is to ensure that they're unique to each run.
    sigint_suppress = lambda signum, frame: None
    sigint_suppress.__name__ = 'sig_ign_sigint'
    normal_sigint = lambda signum, frame: None
    normal_sigint.__name__ = 'sigint'
    normal_sigterm = lambda signum, frame: None
    normal_sigterm.__name__ = 'sigterm'

    # Set up complicated mock for signal.signal().
    def _SignalChecker(sig, _action):
      """Return the right signal values so we can check the calls."""
      if sig == signal.SIGINT:
        return sigint_suppress if ignore_sigint else normal_sigint
      elif sig == signal.SIGTERM:
        return normal_sigterm
      else:
        raise ValueError('unknown sig %i' % sig)
    self.signal_mock.side_effect = _SignalChecker

    # Set up complicated mock for signal.getsignal().
    def _GetsignalChecker(sig):
      """Return the right signal values so we can check the calls."""
      if sig == signal.SIGINT:
        self.assertFalse(ignore_sigint)
        return normal_sigint
      elif sig == signal.SIGTERM:
        return normal_sigterm
      else:
        raise ValueError('unknown sig %i' % sig)
    self.getsignal_mock.side_effect = _GetsignalChecker

    # Let the body of code run, then check the signal behavior afterwards.
    # We don't get visibility into signal ordering vs command execution,
    # but it's kind of hard to mess up that, so we won't bother.
    yield

    class RejectSigIgn(object):
      """Make sure the signal action is not SIG_IGN."""
      def __eq__(self, other):
        return other != signal.SIG_IGN

    # Verify the signals checked/setup are correct.
    if ignore_sigint:
      self.signal_mock.assert_has_calls([
          mock.call(signal.SIGINT, signal.SIG_IGN),
          mock.call(signal.SIGTERM, RejectSigIgn()),
          mock.call(signal.SIGINT, sigint_suppress),
          mock.call(signal.SIGTERM, normal_sigterm),
      ])
      self.assertEqual(self.getsignal_mock.call_count, 1)
    else:
      self.signal_mock.assert_has_calls([
          mock.call(signal.SIGINT, RejectSigIgn()),
          mock.call(signal.SIGTERM, RejectSigIgn()),
          mock.call(signal.SIGINT, normal_sigint),
          mock.call(signal.SIGTERM, normal_sigterm),
      ])
      self.assertEqual(self.getsignal_mock.call_count, 2)

    # Verify various args are passed down to the real command.
    pargs = self.popen_mock.call_args[0][0]
    self.assertEqual(cmd, pargs)

    # Verify various kwargs are passed down to the real command.
    pkwargs = self.popen_mock.call_args[1]
    for key in ('cwd', 'stdin', 'stdout', 'stderr'):
      kwargs.setdefault(key, None)
    kwargs.setdefault('shell', False)
    kwargs.setdefault('env', mock.ANY)
    kwargs['close_fds'] = True
    self.longMessage = True
    for key in kwargs.keys():
      self.assertEqual(kwargs[key], pkwargs[key],
                       msg='kwargs[%s] mismatch' % key)

  def _AssertCrEqual(self, expected, actual):
    """Helper method to compare two CommandResult objects.

    This is needed since assertEqual does not know how to compare two
    CommandResult objects.

    Args:
      expected: a CommandResult object, expected result.
      actual: a CommandResult object, actual result.
    """
    self.assertEqual(expected.cmd, actual.cmd)
    self.assertEqual(expected.error, actual.error)
    self.assertEqual(expected.output, actual.output)
    self.assertEqual(expected.returncode, actual.returncode)

  @_ForceLoggingLevel
  def _TestCmd(self, cmd, real_cmd, sp_kv=None, rc_kv=None, sudo=False):
    """Factor out common setup logic for testing RunCommand().

    Args:
      cmd: a string or an array of strings that will be passed to RunCommand.
      real_cmd: the real command we expect RunCommand to call (might be
          modified to have enter_chroot).
      sp_kv: key-value pairs passed to subprocess.Popen().
      rc_kv: key-value pairs passed to RunCommand().
      sudo: use SudoRunCommand() rather than RunCommand().
    """
    if sp_kv is None:
      sp_kv = {}
    if rc_kv is None:
      rc_kv = {}

    expected_result = cros_build_lib.CommandResult()
    expected_result.cmd = real_cmd
    expected_result.error = self.error
    expected_result.output = self.output
    expected_result.returncode = self.proc_mock.returncode

    arg_dict = dict()
    for attr in ('close_fds', 'cwd', 'env', 'stdin', 'stdout', 'stderr',
                 'shell'):
      if attr in sp_kv:
        arg_dict[attr] = sp_kv[attr]
      else:
        if attr == 'close_fds':
          arg_dict[attr] = True
        elif attr == 'shell':
          arg_dict[attr] = False
        else:
          arg_dict[attr] = None

    if sudo:
      runcmd = cros_build_lib.SudoRunCommand
    else:
      runcmd = cros_build_lib.RunCommand
    with self._MockChecker(real_cmd, ignore_sigint=rc_kv.get('ignore_sigint'),
                           **sp_kv):
      actual_result = runcmd(cmd, **rc_kv)

    self._AssertCrEqual(expected_result, actual_result)

  def testReturnCodeZeroWithArrayCmd(self, ignore_sigint=False):
    """--enter_chroot=False and --cmd is an array of strings.

    Parameterized so this can also be used by some other tests w/ alternate
    params to RunCommand().

    Args:
      ignore_sigint: If True, we'll tell RunCommand to ignore sigint.
    """
    self.proc_mock.returncode = 0
    cmd_list = ['foo', 'bar', 'roger']
    self._TestCmd(cmd_list, cmd_list,
                  rc_kv=dict(ignore_sigint=ignore_sigint))

  def testSignalRestoreNormalCase(self):
    """Test RunCommand() properly sets/restores sigint.  Normal case."""
    self.testReturnCodeZeroWithArrayCmd(ignore_sigint=True)

  def testReturnCodeZeroWithArrayCmdEnterChroot(self):
    """--enter_chroot=True and --cmd is an array of strings."""
    self.proc_mock.returncode = 0
    cmd_list = ['foo', 'bar', 'roger']
    real_cmd = cmd_list
    if not cros_build_lib.IsInsideChroot():
      real_cmd = ['cros_sdk', '--'] + cmd_list
    self._TestCmd(cmd_list, real_cmd, rc_kv=dict(enter_chroot=True))

  @_ForceLoggingLevel
  def testCommandFailureRaisesError(self, ignore_sigint=False):
    """Verify error raised by communicate() is caught.

    Parameterized so this can also be used by some other tests w/ alternate
    params to RunCommand().

    Args:
      ignore_sigint: If True, we'll tell RunCommand to ignore sigint.
    """
    cmd = 'test cmd'
    self.proc_mock.returncode = 1
    with self._MockChecker(['/bin/bash', '-c', cmd],
                           ignore_sigint=ignore_sigint):
      self.assertRaises(cros_build_lib.RunCommandError,
                        cros_build_lib.RunCommand, cmd, shell=True,
                        ignore_sigint=ignore_sigint, error_code_ok=False)

  @_ForceLoggingLevel
  def testSubprocessCommunicateExceptionRaisesError(self, ignore_sigint=False):
    """Verify error raised by communicate() is caught.

    Parameterized so this can also be used by some other tests w/ alternate
    params to RunCommand().

    Args:
      ignore_sigint: If True, we'll tell RunCommand to ignore sigint.
    """
    cmd = ['test', 'cmd']
    self.proc_mock.communicate = mock.MagicMock(side_effect=ValueError)
    with self._MockChecker(cmd, ignore_sigint=ignore_sigint):
      self.assertRaises(ValueError, cros_build_lib.RunCommand, cmd,
                        ignore_sigint=ignore_sigint)

  def testSignalRestoreExceptionCase(self):
    """Test RunCommand() properly sets/restores sigint.  Exception case."""
    self.testSubprocessCommunicateExceptionRaisesError(ignore_sigint=True)

  def testEnvWorks(self):
    """Test RunCommand(..., env=xyz) works."""
    # We'll put this bogus environment together, just to make sure
    # subprocess.Popen gets passed it.
    env = {'Tom': 'Jerry', 'Itchy': 'Scratchy'}

    # This is a simple case, copied from testReturnCodeZeroWithArrayCmd()
    self.proc_mock.returncode = 0
    cmd_list = ['foo', 'bar', 'roger']

    # Run.  We expect the env= to be passed through from sp (subprocess.Popen)
    # to rc (RunCommand).
    self._TestCmd(cmd_list, cmd_list,
                  sp_kv=dict(env=env),
                  rc_kv=dict(env=env))

  def testExtraEnvOnlyWorks(self):
    """Test RunCommand(..., extra_env=xyz) works."""
    # We'll put this bogus environment together, just to make sure
    # subprocess.Popen gets passed it.
    extra_env = {'Pinky' : 'Brain'}
    ## This is a little bit circular, since the same logic is used to compute
    ## the value inside, but at least it checks that this happens.
    total_env = os.environ.copy()
    total_env.update(extra_env)

    # This is a simple case, copied from testReturnCodeZeroWithArrayCmd()
    self.proc_mock.returncode = 0
    cmd_list = ['foo', 'bar', 'roger']

    # Run.  We expect the env= to be passed through from sp (subprocess.Popen)
    # to rc (RunCommand).
    self._TestCmd(cmd_list, cmd_list,
                  sp_kv=dict(env=total_env),
                  rc_kv=dict(extra_env=extra_env))

  def testExtraEnvTooWorks(self):
    """Test RunCommand(..., env=xy, extra_env=z) works."""
    # We'll put this bogus environment together, just to make sure
    # subprocess.Popen gets passed it.
    env = {'Tom': 'Jerry', 'Itchy': 'Scratchy'}
    extra_env = {'Pinky': 'Brain'}
    total_env = {'Tom': 'Jerry', 'Itchy': 'Scratchy', 'Pinky': 'Brain'}

    # This is a simple case, copied from testReturnCodeZeroWithArrayCmd()
    self.proc_mock.returncode = 0
    cmd_list = ['foo', 'bar', 'roger']

    # Run.  We expect the env= to be passed through from sp (subprocess.Popen)
    # to rc (RunCommand).
    self._TestCmd(cmd_list, cmd_list,
                  sp_kv=dict(env=total_env),
                  rc_kv=dict(env=env, extra_env=extra_env))

  @mock.patch('chromite.lib.cros_build_lib.IsInsideChroot', return_value=False)
  def testChrootExtraEnvWorks(self, _inchroot_mock):
    """Test RunCommand(..., enter_chroot=True, env=xy, extra_env=z) works."""
    # We'll put this bogus environment together, just to make sure
    # subprocess.Popen gets passed it.
    env = {'Tom': 'Jerry', 'Itchy': 'Scratchy'}
    extra_env = {'Pinky': 'Brain'}
    total_env = {'Tom': 'Jerry', 'Itchy': 'Scratchy', 'Pinky': 'Brain'}

    # This is a simple case, copied from testReturnCodeZeroWithArrayCmd()
    self.proc_mock.returncode = 0
    cmd_list = ['foo', 'bar', 'roger']

    # Run.  We expect the env= to be passed through from sp (subprocess.Popen)
    # to rc (RunCommand).
    self._TestCmd(cmd_list, ['cros_sdk', 'Pinky=Brain', '--'] + cmd_list,
                  sp_kv=dict(env=total_env),
                  rc_kv=dict(env=env, extra_env=extra_env, enter_chroot=True))

  def testExceptionEquality(self):
    """Verify equality methods for RunCommandError"""

    c1 = cros_build_lib.CommandResult(cmd=['ls', 'arg'], returncode=1)
    c2 = cros_build_lib.CommandResult(cmd=['ls', 'arg1'], returncode=1)
    c3 = cros_build_lib.CommandResult(cmd=['ls', 'arg'], returncode=2)
    e1 = cros_build_lib.RunCommandError('Message 1', c1)
    e2 = cros_build_lib.RunCommandError('Message 1', c1)
    e_diff_msg = cros_build_lib.RunCommandError('Message 2', c1)
    e_diff_cmd = cros_build_lib.RunCommandError('Message 1', c2)
    e_diff_code = cros_build_lib.RunCommandError('Message 1', c3)

    self.assertEqual(e1, e2)
    self.assertNotEqual(e1, e_diff_msg)
    self.assertNotEqual(e1, e_diff_cmd)
    self.assertNotEqual(e1, e_diff_code)

  def testSudoRunCommand(self):
    """Test SudoRunCommand(...) works."""
    cmd_list = ['foo', 'bar', 'roger']
    sudo_list = ['sudo', '--'] + cmd_list
    self.proc_mock.returncode = 0
    self._TestCmd(cmd_list, sudo_list, sudo=True)

  def testSudoRunCommandShell(self):
    """Test SudoRunCommand(..., shell=True) works."""
    cmd = 'foo bar roger'
    sudo_list = ['sudo', '--', '/bin/bash', '-c', cmd]
    self.proc_mock.returncode = 0
    self._TestCmd(cmd, sudo_list, sudo=True,
                  rc_kv=dict(shell=True))

  def testSudoRunCommandEnv(self):
    """Test SudoRunCommand(..., extra_env=z) works."""
    cmd_list = ['foo', 'bar', 'roger']
    sudo_list = ['sudo', 'shucky=ducky', '--'] + cmd_list
    extra_env = {'shucky' : 'ducky'}
    self.proc_mock.returncode = 0
    self._TestCmd(cmd_list, sudo_list, sudo=True,
                  rc_kv=dict(extra_env=extra_env))

  def testSudoRunCommandUser(self):
    """Test SudoRunCommand(..., user='...') works."""
    cmd_list = ['foo', 'bar', 'roger']
    sudo_list = ['sudo', '-u', 'MMMMMonster', '--'] + cmd_list
    self.proc_mock.returncode = 0
    self._TestCmd(cmd_list, sudo_list, sudo=True,
                  rc_kv=dict(user='MMMMMonster'))

  def testSudoRunCommandUserShell(self):
    """Test SudoRunCommand(..., user='...', shell=True) works."""
    cmd = 'foo bar roger'
    sudo_list = ['sudo', '-u', 'MMMMMonster', '--', '/bin/bash', '-c', cmd]
    self.proc_mock.returncode = 0
    self._TestCmd(cmd, sudo_list, sudo=True,
                  rc_kv=dict(user='MMMMMonster', shell=True))


class TestRunCommandOutput(cros_test_lib.TempDirTestCase,
                           cros_test_lib.OutputTestCase):
  """Tests of RunCommand output options."""

  @_ForceLoggingLevel
  def testLogStdoutToFile(self):
    log = os.path.join(self.tempdir, 'output')
    ret = cros_build_lib.RunCommand(
        ['echo', 'monkeys'], log_stdout_to_file=log)
    self.assertEqual(osutils.ReadFile(log), 'monkeys\n')
    self.assertIs(ret.output, None)
    self.assertIs(ret.error, None)

    os.unlink(log)
    ret = cros_build_lib.RunCommand(
        ['sh', '-c', 'echo monkeys3 >&2'],
        log_stdout_to_file=log, redirect_stderr=True)
    self.assertEqual(ret.error, 'monkeys3\n')
    self.assertExists(log)
    self.assertEqual(os.path.getsize(log), 0)

    os.unlink(log)
    ret = cros_build_lib.RunCommand(
        ['sh', '-c', 'echo monkeys4; echo monkeys5 >&2'],
        log_stdout_to_file=log, combine_stdout_stderr=True)
    self.assertIs(ret.output, None)
    self.assertIs(ret.error, None)
    self.assertEqual(osutils.ReadFile(log), 'monkeys4\nmonkeys5\n')


  @_ForceLoggingLevel
  def testLogStdoutToFileWithOrWithoutAppend(self):
    log = os.path.join(self.tempdir, 'output')
    ret = cros_build_lib.RunCommand(
        ['echo', 'monkeys'], log_stdout_to_file=log)
    self.assertEqual(osutils.ReadFile(log), 'monkeys\n')
    self.assertIs(ret.output, None)
    self.assertIs(ret.error, None)

    # Without append
    ret = cros_build_lib.RunCommand(
        ['echo', 'monkeys2'], log_stdout_to_file=log)
    self.assertEqual(osutils.ReadFile(log), 'monkeys2\n')
    self.assertIs(ret.output, None)
    self.assertIs(ret.error, None)

    # With append
    ret = cros_build_lib.RunCommand(
        ['echo', 'monkeys3'], append_to_file=True, log_stdout_to_file=log)
    self.assertEqual(osutils.ReadFile(log), 'monkeys2\nmonkeys3\n')
    self.assertIs(ret.output, None)
    self.assertIs(ret.error, None)


  def _CaptureRunCommand(self, command, mute_output):
    """Capture a RunCommand() output with the specified |mute_output|.

    Args:
      command: command to send to RunCommand().
      mute_output: RunCommand() |mute_output| parameter.

    Returns:
      A (stdout, stderr) pair of captured output.
    """
    with self.OutputCapturer() as output:
      cros_build_lib.RunCommand(command,
                                debug_level=logging.DEBUG,
                                mute_output=mute_output)
    return (output.GetStdout(), output.GetStderr())

  @_ForceLoggingLevel
  def testSubprocessMuteOutput(self):
    """Test RunCommand |mute_output| parameter."""
    command = ['sh', '-c', 'echo foo; echo bar >&2']
    # Always mute: we shouldn't get any output.
    self.assertEqual(self._CaptureRunCommand(command, mute_output=True),
                     ('', ''))
    # Mute based on |debug_level|: we should't get any output.
    self.assertEqual(self._CaptureRunCommand(command, mute_output=None),
                     ('', ''))
    # Never mute: we should get 'foo\n' and 'bar\n'.
    self.assertEqual(self._CaptureRunCommand(command, mute_output=False),
                     ('foo\n', 'bar\n'))

  def testRunCommandAtNoticeLevel(self):
    """Ensure that RunCommand prints output when mute_output is False."""
    # Needed by cros_sdk and brillo/cros chroot.
    with self.OutputCapturer():
      cros_build_lib.RunCommand(['echo', 'foo'], mute_output=False,
                                error_code_ok=True, print_cmd=False,
                                debug_level=logging.NOTICE)
    self.AssertOutputContainsLine('foo')

  def testRunCommandRedirectStdoutStderrOnCommandError(self):
    """Tests that stderr is captured when RunCommand raises."""
    with self.assertRaises(cros_build_lib.RunCommandError) as cm:
      cros_build_lib.RunCommand(['cat', '/'], redirect_stderr=True)
    self.assertIsNotNone(cm.exception.result.error)
    self.assertNotEqual('', cm.exception.result.error)

  def _CaptureLogOutput(self, cmd, **kwargs):
    """Capture logging output of RunCommand."""
    log = os.path.join(self.tempdir, 'output')
    fh = logging.FileHandler(log)
    fh.setLevel(logging.DEBUG)
    logging.getLogger().addHandler(fh)
    cros_build_lib.RunCommand(cmd, **kwargs)
    logging.getLogger().removeHandler(fh)
    return osutils.ReadFile(log)

  @_ForceLoggingLevel
  def testLogOutput(self):
    """Normal log_output, stdout followed by stderr."""
    cmd = 'echo Greece; echo Italy >&2; echo Spain'
    log_output = ("RunCommand: /bin/bash -c "
                  "'echo Greece; echo Italy >&2; echo Spain'\n"
                  "(stdout):\nGreece\nSpain\n\n(stderr):\nItaly\n\n")
    self.assertEquals(self._CaptureLogOutput(cmd, shell=True, log_output=True),
                      log_output)

  @_ForceLoggingLevel
  def testStreamLog(self):
    """Streaming log_output, stdout and stderr interwoven in order."""
    cmd = 'echo Greece; echo Italy >&2; echo Spain'
    log_output = ("RunCommand: /bin/bash -c "
                  "'echo Greece; echo Italy >&2; echo Spain'\n"
                  "(stdout/stderr):\n\nGreece\nItaly\nSpain\n")
    self.assertEquals(self._CaptureLogOutput(cmd, shell=True, stream_log=True),
                      log_output)


class TestTimedSection(cros_test_lib.TestCase):
  """Tests for TimedSection context manager."""

  def testTimerValues(self):
    """Make sure simple stuff works."""
    with cros_build_lib.TimedSection() as timer:
      # While running, we have access to the start time.
      self.assertIsInstance(timer.start, datetime.datetime)
      self.assertIsNone(timer.finish)
      self.assertIsNone(timer.delta)

    # After finishing, all values should be set.
    self.assertIsInstance(timer.start, datetime.datetime)
    self.assertIsInstance(timer.finish, datetime.datetime)
    self.assertIsInstance(timer.delta, datetime.timedelta)


class TestListFiles(cros_test_lib.TempDirTestCase):
  """Tests of ListFiles funciton."""

  def _CreateNestedDir(self, dir_structure):
    for entry in dir_structure:
      full_path = os.path.join(os.path.join(self.tempdir, entry))
      # ensure dirs are created
      try:
        os.makedirs(os.path.dirname(full_path))
        if full_path.endswith('/'):
          # we only want to create directories
          return
      except OSError as err:
        if err.errno == errno.EEXIST:
          # we don't care if the dir already exists
          pass
        else:
          raise
      # create dummy files
      tmp = open(full_path, 'w')
      tmp.close()

  def testTraverse(self):
    """Test that we are traversing the directory properly."""
    dir_structure = ['one/two/test.txt', 'one/blah.py',
                     'three/extra.conf']
    self._CreateNestedDir(dir_structure)

    files = cros_build_lib.ListFiles(self.tempdir)
    for f in files:
      f = f.replace(self.tempdir, '').lstrip('/')
      if f not in dir_structure:
        self.fail('%s was not found in %s' % (f, dir_structure))

  def testEmptyFilePath(self):
    """Test that we return nothing when directories are empty."""
    dir_structure = ['one/', 'two/', 'one/a/']
    self._CreateNestedDir(dir_structure)
    files = cros_build_lib.ListFiles(self.tempdir)
    self.assertEqual(files, [])

  def testNoSuchDir(self):
    try:
      cros_build_lib.ListFiles(os.path.join(self.tempdir, 'missing'))
    except OSError as err:
      self.assertEqual(err.errno, errno.ENOENT)


class HelperMethodSimpleTests(cros_test_lib.OutputTestCase):
  """Tests for various helper methods without using mocks."""

  def _TestChromeosVersion(self, test_str, expected=None):
    actual = cros_build_lib.GetChromeosVersion(test_str)
    self.assertEqual(expected, actual)

  def testGetChromeosVersionWithValidVersionReturnsValue(self):
    expected = '0.8.71.2010_09_10_1530'
    test_str = ' CHROMEOS_VERSION_STRING=0.8.71.2010_09_10_1530 '
    self._TestChromeosVersion(test_str, expected)

  def testGetChromeosVersionWithMultipleVersionReturnsFirstMatch(self):
    expected = '0.8.71.2010_09_10_1530'
    test_str = (' CHROMEOS_VERSION_STRING=0.8.71.2010_09_10_1530 '
                ' CHROMEOS_VERSION_STRING=10_1530 ')
    self._TestChromeosVersion(test_str, expected)

  def testGetChromeosVersionWithInvalidVersionReturnsDefault(self):
    test_str = ' CHROMEOS_VERSION_STRING=invalid_version_string '
    self._TestChromeosVersion(test_str)

  def testGetChromeosVersionWithEmptyInputReturnsDefault(self):
    self._TestChromeosVersion('')

  def testGetChromeosVersionWithNoneInputReturnsDefault(self):
    self._TestChromeosVersion(None)

  def testUserDateTime(self):
    """Test with a raw time value."""
    expected = 'Mon, 16 Jun 1980 05:03:20 -0700 (PDT)'
    with cros_test_lib.SetTimeZone('US/Pacific'):
      timeval = 330005000
      self.assertEqual(cros_build_lib.UserDateTimeFormat(timeval=timeval),
                       expected)

  def testUserDateTimeDateTime(self):
    """Test with a datetime object."""
    expected = 'Mon, 16 Jun 1980 00:00:00 -0700 (PDT)'
    with cros_test_lib.SetTimeZone('US/Pacific'):
      timeval = datetime.datetime(1980, 6, 16)
      self.assertEqual(cros_build_lib.UserDateTimeFormat(timeval=timeval),
                       expected)

  def testUserDateTimeDateTimeInWinter(self):
    """Test that we correctly switch from PDT to PST."""
    expected = 'Wed, 16 Jan 1980 00:00:00 -0800 (PST)'
    with cros_test_lib.SetTimeZone('US/Pacific'):
      timeval = datetime.datetime(1980, 1, 16)
      self.assertEqual(cros_build_lib.UserDateTimeFormat(timeval=timeval),
                       expected)

  def testUserDateTimeDateTimeInEST(self):
    """Test that we correctly switch from PDT to EST."""
    expected = 'Wed, 16 Jan 1980 00:00:00 -0500 (EST)'
    with cros_test_lib.SetTimeZone('US/Eastern'):
      timeval = datetime.datetime(1980, 1, 16)
      self.assertEqual(cros_build_lib.UserDateTimeFormat(timeval=timeval),
                       expected)

  def testUserDateTimeCurrentTime(self):
    """Test that we can get the current time."""
    cros_build_lib.UserDateTimeFormat()

  def testParseUserDateTimeFormat(self):
    stringtime = cros_build_lib.UserDateTimeFormat(100000.0)
    self.assertEqual(cros_build_lib.ParseUserDateTimeFormat(stringtime),
                     100000.0)

  def testParseDurationToSeconds(self):
    self.assertEqual(cros_build_lib.ParseDurationToSeconds('1:01:01'),
                     3600 + 60 + 1)

  def testMachineDetails(self):
    """Verify we don't crash."""
    contents = cros_build_lib.MachineDetails()
    self.assertNotEqual(contents, '')
    self.assertEqual(contents[-1], '\n')

  def testGetCommonPathPrefix(self):
    """Test helper function correctness."""
    self.assertEqual('/a', cros_build_lib.GetCommonPathPrefix(['/a/b']))
    self.assertEqual('/a', cros_build_lib.GetCommonPathPrefix(['/a/']))
    self.assertEqual('/', cros_build_lib.GetCommonPathPrefix(['/a']))
    self.assertEqual(
        '/a', cros_build_lib.GetCommonPathPrefix(['/a/b', '/a/c']))
    self.assertEqual(
        '/a/b', cros_build_lib.GetCommonPathPrefix(['/a/b/c', '/a/b/d']))
    self.assertEqual('/', cros_build_lib.GetCommonPathPrefix(['/a/b', '/c/d']))
    self.assertEqual(
        '/', cros_build_lib.GetCommonPathPrefix(['/a/b', '/aa/b']))

  def testFormatDetailedTraceback(self):
    """Verify various aspects of the traceback"""
    # When there is no active exception, should output nothing.
    data = cros_build_lib.FormatDetailedTraceback()
    self.assertEqual(data, '')

    # Generate a local exception and test it.
    try:
      varint = 12345
      varstr = 'vaaars'
      raise Exception('fooood')
    except Exception:
      lines = cros_build_lib.FormatDetailedTraceback().splitlines()
      # Check basic start/finish lines.
      self.assertIn('Traceback ', lines[0])
      self.assertIn('Exception: fooood', lines[-1])

      # Verify some local vars get correctly decoded.
      for line in lines:
        if 'varint' in line:
          self.assertIn('int', line)
          self.assertIn(str(varint), line)
          break
      else:
        raise AssertionError('could not find local "varint" in output:\n\n%s' %
                             ''.join(lines))

      for line in lines:
        if 'varstr' in line:
          self.assertIn('str', line)
          self.assertIn(varstr, line)
          break
      else:
        raise AssertionError('could not find local "varstr" in output:\n\n%s' %
                             ''.join(lines))

  def _testPrintDetailedTraceback(self, check_stdout):
    """Helper method for testing PrintDetailedTraceback."""
    try:
      varint = 12345
      varstr = 'vaaars'
      raise Exception('fooood')
    except Exception:
      with self.OutputCapturer() as output:
        if check_stdout is None:
          stream = None
        elif check_stdout:
          stream = sys.stdout
        else:
          stream = sys.stderr
        cros_build_lib.PrintDetailedTraceback(file=stream)

        # The non-selected stream shouldn't have anything.
        data = output.GetStderr() if check_stdout else output.GetStdout()
        self.assertEqual(data, '')

        kwargs = {
            'check_stdout': check_stdout,
            'check_stderr': not check_stdout,
        }
        self.AssertOutputContainsLine(r'Traceback ', **kwargs)
        self.AssertOutputContainsLine(r'Exception: fooood', **kwargs)
        self.AssertOutputContainsLine(r'varint.*int.*%s' % varint, **kwargs)
        self.AssertOutputContainsLine(r'varstr.*str.*%s' % varstr, **kwargs)

  def testPrintDetailedTracebackStderrDefault(self):
    """Verify default (stderr) handling"""
    self._testPrintDetailedTraceback(None)

  def testPrintDetailedTracebackStderr(self):
    """Verify stderr handling"""
    self._testPrintDetailedTraceback(False)

  def testPrintDetailedTracebackStdout(self):
    """Verify stdout handling"""
    self._testPrintDetailedTraceback(True)


class TestInput(cros_test_lib.MockOutputTestCase):
  """Tests of input gathering functionality."""

  def testGetInput(self):
    """Verify GetInput() basic behavior."""
    response = 'Some response'
    self.PatchObject(__builtin__, 'raw_input', return_value=response)
    self.assertEquals(response, cros_build_lib.GetInput('prompt'))

  def testBooleanPrompt(self):
    """Verify BooleanPrompt() full behavior."""
    m = self.PatchObject(cros_build_lib, 'GetInput')

    m.return_value = ''
    self.assertTrue(cros_build_lib.BooleanPrompt())
    self.assertFalse(cros_build_lib.BooleanPrompt(default=False))

    m.return_value = 'yes'
    self.assertTrue(cros_build_lib.BooleanPrompt())
    m.return_value = 'ye'
    self.assertTrue(cros_build_lib.BooleanPrompt())
    m.return_value = 'y'
    self.assertTrue(cros_build_lib.BooleanPrompt())

    m.return_value = 'no'
    self.assertFalse(cros_build_lib.BooleanPrompt())
    m.return_value = 'n'
    self.assertFalse(cros_build_lib.BooleanPrompt())

  def testBooleanShellValue(self):
    """Verify BooleanShellValue() inputs work as expected"""
    for v in (None,):
      self.assertTrue(cros_build_lib.BooleanShellValue(v, True))
      self.assertFalse(cros_build_lib.BooleanShellValue(v, False))

    for v in (1234, '', 'akldjsf', '"'):
      self.assertRaises(ValueError, cros_build_lib.BooleanShellValue, v, True)
      self.assertTrue(cros_build_lib.BooleanShellValue(v, True, msg=''))
      self.assertFalse(cros_build_lib.BooleanShellValue(v, False, msg=''))

    for v in ('yes', 'YES', 'YeS', 'y', 'Y', '1', 'true', 'True', 'TRUE',):
      self.assertTrue(cros_build_lib.BooleanShellValue(v, True))
      self.assertTrue(cros_build_lib.BooleanShellValue(v, False))

    for v in ('no', 'NO', 'nO', 'n', 'N', '0', 'false', 'False', 'FALSE',):
      self.assertFalse(cros_build_lib.BooleanShellValue(v, True))
      self.assertFalse(cros_build_lib.BooleanShellValue(v, False))

  def testGetChoiceLists(self):
    """Verify GetChoice behavior w/lists."""
    m = self.PatchObject(cros_build_lib, 'GetInput')

    m.return_value = '1'
    ret = cros_build_lib.GetChoice('title', ['a', 'b', 'c'])
    self.assertEqual(ret, 1)

  def testGetChoiceGenerator(self):
    """Verify GetChoice behavior w/generators."""
    m = self.PatchObject(cros_build_lib, 'GetInput')

    m.return_value = '2'
    ret = cros_build_lib.GetChoice('title', xrange(3))
    self.assertEqual(ret, 2)

  def testGetChoiceWindow(self):
    """Verify GetChoice behavior w/group_size set."""
    m = self.PatchObject(cros_build_lib, 'GetInput')

    cnt = [0]
    def _Gen():
      while True:
        cnt[0] += 1
        yield 'a'

    m.side_effect = ['\n', '2']
    ret = cros_build_lib.GetChoice('title', _Gen(), group_size=2)
    self.assertEqual(ret, 2)

    # Verify we showed the correct number of times.
    self.assertEqual(cnt[0], 5)


class TestContextManagerStack(cros_test_lib.TestCase):
  """Test the ContextManagerStack class."""

  def test(self):
    invoked = []
    counter = iter(itertools.count()).next
    def _mk_kls(has_exception=None, exception_kls=None, suppress=False):
      class foon(object):
        """Simple context manager which runs checks on __exit__."""
        marker = counter()
        def __enter__(self):
          return self

        # pylint: disable=no-self-argument,bad-context-manager
        def __exit__(obj_self, exc_type, exc, traceback):
          invoked.append(obj_self.marker)
          if has_exception is not None:
            self.assertTrue(all(x is not None
                                for x in (exc_type, exc, traceback)))
            self.assertTrue(exc_type == has_exception)
          if exception_kls:
            raise exception_kls()
          if suppress:
            return True
      return foon

    with cros_build_lib.ContextManagerStack() as stack:
      # Note... these tests are in reverse, since the exception
      # winds its way up the stack.
      stack.Add(_mk_kls())
      stack.Add(_mk_kls(ValueError, suppress=True))
      stack.Add(_mk_kls(IndexError, exception_kls=ValueError))
      stack.Add(_mk_kls(IndexError))
      stack.Add(_mk_kls(exception_kls=IndexError))
      stack.Add(_mk_kls())
    self.assertEqual(invoked, list(reversed(range(6))))


class TestManifestCheckout(cros_test_lib.TempDirTestCase):
  """Tests for ManifestCheckout functionality."""

  def setUp(self):
    self.manifest_dir = os.path.join(self.tempdir, '.repo', 'manifests')

    # Initialize a repo instance here.
    local_repo = os.path.join(constants.SOURCE_ROOT, '.repo/repo/.git')

    # Create a copy of our existing manifests.git, but rewrite it so it
    # looks like a remote manifests.git.  This is to avoid hitting the
    # network, and speeds things up in general.
    local_manifests = 'file://%s/.repo/manifests.git' % constants.SOURCE_ROOT
    temp_manifests = os.path.join(self.tempdir, 'manifests.git')
    git.RunGit(self.tempdir, ['clone', '-n', '--bare', local_manifests])
    git.RunGit(temp_manifests,
               ['fetch', '-f', '-u', local_manifests,
                'refs/remotes/origin/*:refs/heads/*'])
    git.RunGit(temp_manifests, ['branch', '-D', 'default'])
    repo = repository.RepoRepository(
        temp_manifests, self.tempdir,
        repo_url='file://%s' % local_repo, repo_branch='default')
    repo.Initialize()

    self.active_manifest = os.path.realpath(
        os.path.join(self.tempdir, '.repo', 'manifest.xml'))

  def testManifestInheritance(self):
    osutils.WriteFile(self.active_manifest, """
        <manifest>
          <include name="include-target.xml" />
          <include name="empty.xml" />
          <project name="monkeys" path="baz" remote="foon" revision="master" />
        </manifest>""")
    # First, verify it properly explodes if the include can't be found.
    self.assertRaises(EnvironmentError,
                      git.ManifestCheckout, self.tempdir)

    # Next, verify it can read an empty manifest; this is to ensure
    # that we can point Manifest at the empty manifest without exploding,
    # same for ManifestCheckout; this sort of thing is primarily useful
    # to ensure no step of an include assumes everything is yet assembled.
    empty_path = os.path.join(self.manifest_dir, 'empty.xml')
    osutils.WriteFile(empty_path, '<manifest/>')
    git.Manifest(empty_path)
    git.ManifestCheckout(self.tempdir, manifest_path=empty_path)

    # Next, verify include works.
    osutils.WriteFile(
        os.path.join(self.manifest_dir, 'include-target.xml'),
        """
        <manifest>
          <remote name="foon" fetch="http://localhost" />
        </manifest>""")
    manifest = git.ManifestCheckout(self.tempdir)
    self.assertEqual(list(manifest.checkouts_by_name), ['monkeys'])
    self.assertEqual(list(manifest.remotes), ['foon'])

  # pylint: disable=E1101
  def testGetManifestsBranch(self):
    func = git.ManifestCheckout._GetManifestsBranch
    manifest = self.manifest_dir
    repo_root = self.tempdir

    # pylint: disable=W0613
    def reconfig(merge='master', origin='origin'):
      if merge is not None:
        merge = 'refs/heads/%s' % merge
      for key in ('merge', 'origin'):
        val = locals()[key]
        key = 'branch.default.%s' % key
        if val is None:
          git.RunGit(manifest, ['config', '--unset', key], error_code_ok=True)
        else:
          git.RunGit(manifest, ['config', key, val])

    # First, verify our assumptions about a fresh repo init are correct.
    self.assertEqual('default', git.GetCurrentBranch(manifest))
    self.assertEqual('master', func(repo_root))

    # Ensure we can handle a missing origin; this can occur jumping between
    # branches, and can be worked around.
    reconfig(origin=None)
    self.assertEqual('default', git.GetCurrentBranch(manifest))
    self.assertEqual('master', func(repo_root))

    def assertExcept(message, **kwargs):
      reconfig(**kwargs)
      self.assertRaises2(OSError, func, repo_root, ex_msg=message,
                         check_attrs={'errno': errno.ENOENT})

    # No merge target means the configuration isn't usable, period.
    assertExcept("git tracking configuration for that branch is broken",
                 merge=None)

    # Ensure we detect if we're on the wrong branch, even if it has
    # tracking setup.
    git.RunGit(manifest, ['checkout', '-t', 'origin/master', '-b', 'test'])
    assertExcept("It should be checked out to 'default'")

    # Ensure we handle detached HEAD w/ an appropriate exception.
    git.RunGit(manifest, ['checkout', '--detach', 'test'])
    assertExcept("It should be checked out to 'default'")

    # Finally, ensure that if the default branch is non-existant, we still throw
    # a usable exception.
    git.RunGit(manifest, ['branch', '-d', 'default'])
    assertExcept("It should be checked out to 'default'")

  def testGitMatchBranchName(self):
    git_repo = os.path.join(self.tempdir, '.repo', 'manifests')

    branches = git.MatchBranchName(git_repo, 'default', namespace='')
    self.assertEqual(branches, ['refs/heads/default'])

    branches = git.MatchBranchName(git_repo, 'default', namespace='refs/heads/')
    self.assertEqual(branches, ['default'])

    branches = git.MatchBranchName(git_repo, 'origin/f.*link',
                                   namespace='refs/remotes/')
    self.assertTrue('firmware-link-' in branches[0])

    branches = git.MatchBranchName(git_repo, 'r23')
    self.assertEqual(branches, ['refs/remotes/origin/release-R23-2913.B'])


class TestGroupByKey(cros_test_lib.TestCase):
  """Test SplitByKey."""

  def testEmpty(self):
    self.assertEqual({}, cros_build_lib.GroupByKey([], ''))

  def testGroupByKey(self):
    input_iter = [{'a': None, 'b': 0},
                  {'a': 1, 'b': 1},
                  {'a': 2, 'b': 2},
                  {'a': 1, 'b': 3}]
    expected_result = {
        None: [{'a': None, 'b': 0}],
        1:    [{'a': 1, 'b': 1},
               {'a': 1, 'b': 3}],
        2:    [{'a': 2, 'b': 2}]}
    self.assertEqual(cros_build_lib.GroupByKey(input_iter, 'a'),
                     expected_result)


class GroupNamedtuplesByKeyTests(cros_test_lib.TestCase):
  """Tests for GroupNamedtuplesByKey"""

  def testGroupNamedtuplesByKeyWithEmptyInputIter(self):
    """Test GroupNamedtuplesByKey with empty input_iter."""
    self.assertEqual({}, cros_build_lib.GroupByKey([], ''))

  def testGroupNamedtuplesByKey(self):
    """Test GroupNamedtuplesByKey."""
    TestTuple = collections.namedtuple('TestTuple', ('key1', 'key2'))
    r1 = TestTuple('t1', 'val1')
    r2 = TestTuple('t2', 'val2')
    r3 = TestTuple('t2', 'val2')
    r4 = TestTuple('t3', 'val3')
    r5 = TestTuple('t3', 'val3')
    r6 = TestTuple('t3', 'val3')
    input_iter = [r1, r2, r3, r4, r5, r6]

    expected_result = {
        't1': [r1],
        't2': [r2, r3],
        't3': [r4, r5, r6]}
    self.assertDictEqual(
        cros_build_lib.GroupNamedtuplesByKey(input_iter, 'key1'),
        expected_result)

    expected_result = {
        'val1': [r1],
        'val2': [r2, r3],
        'val3': [r4, r5, r6]}
    self.assertDictEqual(
        cros_build_lib.GroupNamedtuplesByKey(input_iter, 'key2'),
        expected_result)

    expected_result = {
        None: [r1, r2, r3, r4, r5, r6]}
    self.assertDictEqual(
        cros_build_lib.GroupNamedtuplesByKey(input_iter, 'test'),
        expected_result)


class InvertDictionayTests(cros_test_lib.TestCase):
  """Tests for InvertDictionary."""

  def testInvertDictionary(self):
    """Test InvertDictionary."""
    changes = ['change_1', 'change_2', 'change_3', 'change_4']
    slaves = ['slave_1', 'slave_2', 'slave_3', 'slave_4']
    slave_changes_dict = {
        slaves[0]: set(changes[0:1]),
        slaves[1]: set(changes[0:2]),
        slaves[2]: set(changes[2:4]),
        slaves[3]: set()
    }
    change_slaves_dict = cros_build_lib.InvertDictionary(
        slave_changes_dict)

    expected_dict = {
        changes[0]: set(slaves[0:2]),
        changes[1]: set([slaves[1]]),
        changes[2]: set([slaves[2]]),
        changes[3]: set([slaves[2]])
    }
    self.assertDictEqual(change_slaves_dict, expected_dict)


class Test_iflatten_instance(cros_test_lib.TestCase):
  """Test iflatten_instance function."""

  def test_it(self):
    f = lambda *a: list(cros_build_lib.iflatten_instance(*a))
    self.assertEqual([1, 2], f([1, 2]))
    self.assertEqual([1, '2a'], f([1, '2a']))
    self.assertEqual([1, 2, 'b'], f([1, [2, 'b']]))
    self.assertEqual([1, 2, 'f', 'd', 'a', 's'], f([1, 2, ('fdas',)], int))
    self.assertEqual([''], f(''))


class TestKeyValueFiles(cros_test_lib.TempDirTestCase):
  """Tests handling of key/value files."""

  def setUp(self):
    self.contents = """# A comment !@
A = 1
AA= 2
AAA =3
AAAA\t=\t4
AAAAA\t   \t=\t   5
AAAAAA = 6     \t\t# Another comment
\t
\t# Aerith lives!
C = 'D'
CC= 'D'
CCC ='D'
\x20
 \t# monsters go boom #
E \t= "Fxxxxx" # Blargl
EE= "Faaa\taaaa"\x20
EEE ="Fk  \t  kkkk"\t
Q = "'q"
\tQQ ="q'"\x20
 QQQ='"q"'\t
R = "r
"
RR = "rr
rrr"
RRR = 'rrr
 RRRR
 rrr
'
SSS=" ss
'ssss'
ss"
T="
ttt"
"""
    self.expected = {
        'A': '1',
        'AA': '2',
        'AAA': '3',
        'AAAA': '4',
        'AAAAA': '5',
        'AAAAAA': '6',
        'C': 'D',
        'CC': 'D',
        'CCC': 'D',
        'E': 'Fxxxxx',
        'EE': 'Faaa\taaaa',
        'EEE': 'Fk  \t  kkkk',
        'Q': "'q",
        'QQ': "q'",
        'QQQ': '"q"',
        'R': 'r\n',
        'RR': 'rr\nrrr',
        'RRR': 'rrr\n RRRR\n rrr\n',
        'SSS': ' ss\n\'ssss\'\nss',
        'T': '\nttt'
    }

    self.conf_file = os.path.join(self.tempdir, 'file.conf')
    osutils.WriteFile(self.conf_file, self.contents)

  def _RunAndCompare(self, test_input, multiline):
    result = cros_build_lib.LoadKeyValueFile(test_input, multiline=multiline)
    self.assertEqual(self.expected, result)

  def testLoadFilePath(self):
    """Verify reading a simple file works"""
    self._RunAndCompare(self.conf_file, True)

  def testLoadStringIO(self):
    """Verify passing in StringIO object works."""
    self._RunAndCompare(StringIO.StringIO(self.contents), True)

  def testLoadFileObject(self):
    """Verify passing in open file object works."""
    with open(self.conf_file) as f:
      self._RunAndCompare(f, True)

  def testNoMultlineValues(self):
    """Verify exception is thrown when multiline is disabled."""
    self.assertRaises(ValueError, self._RunAndCompare, self.conf_file, False)


class SafeRunTest(cros_test_lib.TestCase):
  """Tests SafeRunTest functionality."""

  def _raise_exception(self, e):
    raise e

  def testRunsSafely(self):
    """Verify that we are robust to exceptions."""
    def append_val(value):
      call_list.append(value)

    call_list = []
    f_list = [functools.partial(append_val, 1),
              functools.partial(self._raise_exception,
                                Exception('testRunsSafely exception.')),
              functools.partial(append_val, 2)]
    self.assertRaises(Exception, cros_build_lib.SafeRun, f_list)
    self.assertEquals(call_list, [1, 2])

  def testRaisesFirstException(self):
    """Verify we raise the first exception when multiple are encountered."""
    class E1(Exception):
      """Simple exception class."""
      pass

    class E2(Exception):
      """Simple exception class."""
      pass

    f_list = [functools.partial(self._raise_exception, e) for e in [E1, E2]]
    self.assertRaises(E1, cros_build_lib.SafeRun, f_list)

  def testCombinedRaise(self):
    """Raises a RuntimeError with exceptions combined."""
    f_list = [functools.partial(self._raise_exception, Exception())] * 3
    self.assertRaises(RuntimeError, cros_build_lib.SafeRun, f_list,
                      combine_exceptions=True)


class FrozenAttributesTest(cros_test_lib.TestCase):
  """Tests FrozenAttributesMixin functionality."""

  class DummyClass(object):
    """Any class that does not override __setattr__."""

  class SetattrClass(object):
    """Class that does override __setattr__."""
    SETATTR_OFFSET = 10
    def __setattr__(self, attr, value):
      """Adjust value here to later confirm that this code ran."""
      object.__setattr__(self, attr, self.SETATTR_OFFSET + value)

  def _TestBasics(self, cls):
    # pylint: disable=W0201
    def _Expected(val):
      return getattr(cls, 'SETATTR_OFFSET', 0) + val

    obj = cls()
    obj.a = 1
    obj.b = 2
    self.assertEquals(_Expected(1), obj.a)
    self.assertEquals(_Expected(2), obj.b)

    obj.Freeze()
    self.assertRaises(cros_build_lib.AttributeFrozenError, setattr, obj, 'a', 3)
    self.assertEquals(_Expected(1), obj.a)

    self.assertRaises(cros_build_lib.AttributeFrozenError, setattr, obj, 'c', 3)
    self.assertFalse(hasattr(obj, 'c'))

  def testFrozenByMetaclass(self):
    """Test attribute freezing with FrozenAttributesClass."""
    class DummyByMeta(self.DummyClass):
      """Class that freezes DummyClass using metaclass construct."""
      __metaclass__ = cros_build_lib.FrozenAttributesClass

    self._TestBasics(DummyByMeta)

    class SetattrByMeta(self.SetattrClass):
      """Class that freezes SetattrClass using metaclass construct."""
      __metaclass__ = cros_build_lib.FrozenAttributesClass

    self._TestBasics(SetattrByMeta)

  def testFrozenByMixinFirst(self):
    """Test attribute freezing with FrozenAttributesMixin first in hierarchy."""
    class Dummy(cros_build_lib.FrozenAttributesMixin, self.DummyClass):
      """Class that freezes DummyClass using mixin construct."""

    self._TestBasics(Dummy)

    class Setattr(cros_build_lib.FrozenAttributesMixin, self.SetattrClass):
      """Class that freezes SetattrClass using mixin construct."""

    self._TestBasics(Setattr)

  def testFrozenByMixinLast(self):
    """Test attribute freezing with FrozenAttributesMixin last in hierarchy."""
    class Dummy(self.DummyClass, cros_build_lib.FrozenAttributesMixin):
      """Class that freezes DummyClass using mixin construct."""

    self._TestBasics(Dummy)

    class Setattr(self.SetattrClass, cros_build_lib.FrozenAttributesMixin):
      """Class that freezes SetattrClass using mixin construct."""

    self._TestBasics(Setattr)


class TestGetIPv4Address(cros_test_lib.RunCommandTestCase):
  """Tests the GetIPv4Address function."""

  IP_GLOBAL_OUTPUT = """
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 16436 qdisc noqueue state UNKNOWN
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
2: eth0: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc pfifo_fast state \
DOWN qlen 1000
    link/ether cc:cc:cc:cc:cc:cc brd ff:ff:ff:ff:ff:ff
3: eth1: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast state UP \
qlen 1000
    link/ether dd:dd:dd:dd:dd:dd brd ff:ff:ff:ff:ff:ff
    inet 111.11.11.111/22 brd 111.11.11.255 scope global eth1
    inet6 cdef:0:cdef:cdef:cdef:cdef:cdef:cdef/64 scope global dynamic
       valid_lft 2592000sec preferred_lft 604800sec
"""

  def testGetIPv4AddressParseResult(self):
    """Verifies we can parse the output and get correct IP address."""
    self.rc.AddCmdResult(partial_mock.In('ip'), output=self.IP_GLOBAL_OUTPUT)
    self.assertEqual(cros_build_lib.GetIPv4Address(), '111.11.11.111')

  def testGetIPv4Address(self):
    """Tests that correct shell commmand is called."""
    cros_build_lib.GetIPv4Address(global_ip=False, dev='eth0')
    self.rc.assertCommandContains(
        ['ip', 'addr', 'show', 'scope', 'host', 'dev', 'eth0'])

    cros_build_lib.GetIPv4Address(global_ip=True)
    self.rc.assertCommandContains(['ip', 'addr', 'show', 'scope', 'global'])


class TestGetHostname(cros_test_lib.MockTestCase):
  """Tests GetHostName & GetHostDomain functionality."""

  def setUp(self):
    self.gethostname_mock = self.PatchObject(
        socket, 'gethostname', return_value='m!!n')
    self.gethostbyaddr_mock = self.PatchObject(
        socket, 'gethostbyaddr', return_value=(
            'm!!n.google.com', ('cow', 'bar',), ('127.0.0.1.a',)))

  def testGetHostNameNonQualified(self):
    """Verify non-qualified behavior"""
    self.assertEqual(cros_build_lib.GetHostName(), 'm!!n')

  def testGetHostNameFullyQualified(self):
    """Verify fully qualified behavior"""
    self.assertEqual(cros_build_lib.GetHostName(fully_qualified=True),
                     'm!!n.google.com')

  def testGetHostNameBadDns(self):
    """Do not fail when the user's dns is bad"""
    self.gethostbyaddr_mock.side_effect = socket.gaierror('should be caught')
    self.assertEqual(cros_build_lib.GetHostName(), 'm!!n')

  def testGetHostDomain(self):
    """Verify basic behavior"""
    self.assertEqual(cros_build_lib.GetHostDomain(), 'google.com')

  def testHostIsCIBuilder(self):
    """Test HostIsCIBuilder."""
    fq_hostname_golo = 'test.golo.chromium.org'
    fq_hostname_gce_1 = 'test.chromeos-bot.internal'
    fq_hostname_gce_2 = 'test.chrome.corp.google.com'
    fq_hostname_invalid = 'test'
    self.assertTrue(cros_build_lib.HostIsCIBuilder(fq_hostname_golo))
    self.assertTrue(cros_build_lib.HostIsCIBuilder(fq_hostname_gce_1))
    self.assertTrue(cros_build_lib.HostIsCIBuilder(fq_hostname_gce_2))
    self.assertFalse(cros_build_lib.HostIsCIBuilder(fq_hostname_invalid))
    self.assertFalse(cros_build_lib.HostIsCIBuilder(
        fq_hostname=fq_hostname_golo, gce_only=True))
    self.assertFalse(cros_build_lib.HostIsCIBuilder(
        fq_hostname=fq_hostname_gce_1, golo_only=True))


class CollectionTest(cros_test_lib.TestCase):
  """Tests for Collection helper."""

  def testDefaults(self):
    """Verify default values kick in."""
    O = cros_build_lib.Collection('O', a=0, b='string', c={})
    o = O()
    self.assertEqual(o.a, 0)
    self.assertEqual(o.b, 'string')
    self.assertEqual(o.c, {})

  def testOverrideDefaults(self):
    """Verify we can set custom values at instantiation time."""
    O = cros_build_lib.Collection('O', a=0, b='string', c={})
    o = O(a=1000)
    self.assertEqual(o.a, 1000)
    self.assertEqual(o.b, 'string')
    self.assertEqual(o.c, {})

  def testSetNoNewMembers(self):
    """Verify we cannot add new members after the fact."""
    O = cros_build_lib.Collection('O', a=0, b='string', c={})
    o = O()

    # Need the func since self.assertRaises evaluates the args in this scope.
    def _setit(collection):
      collection.does_not_exit = 10
    self.assertRaises(AttributeError, _setit, o)
    self.assertRaises(AttributeError, setattr, o, 'new_guy', 10)

  def testGetNoNewMembers(self):
    """Verify we cannot get new members after the fact."""
    O = cros_build_lib.Collection('O', a=0, b='string', c={})
    o = O()

    # Need the func since self.assertRaises evaluates the args in this scope.
    def _getit(collection):
      return collection.does_not_exit
    self.assertRaises(AttributeError, _getit, o)
    self.assertRaises(AttributeError, getattr, o, 'foooo')

  def testNewValue(self):
    """Verify we change members correctly."""
    O = cros_build_lib.Collection('O', a=0, b='string', c={})
    o = O()
    o.a = 'a string'
    o.c = 123
    self.assertEqual(o.a, 'a string')
    self.assertEqual(o.b, 'string')
    self.assertEqual(o.c, 123)

  def testString(self):
    """Make sure the string representation is readable by da hue mans."""
    O = cros_build_lib.Collection('O', a=0, b='string', c={})
    o = O()
    self.assertEqual("Collection_O(a=0, b='string', c={})", str(o))


class GetImageDiskPartitionInfoTests(cros_test_lib.RunCommandTestCase):
  """Tests the GetImageDiskPartitionInfo function."""

  SAMPLE_PARTED = """/foo/chromiumos_qemu_image.bin:3360MB:file:512:512:gpt:;
11:0.03MB:8.42MB:8.39MB::RWFW:;
6:8.42MB:8.42MB:0.00MB::KERN-C:;
7:8.42MB:8.42MB:0.00MB::ROOT-C:;
9:8.42MB:8.42MB:0.00MB::reserved:;
10:8.42MB:8.42MB:0.00MB::reserved:;
2:10.5MB:27.3MB:16.8MB::KERN-A:;
4:27.3MB:44.0MB:16.8MB::KERN-B:;
8:44.0MB:60.8MB:16.8MB:ext4:OEM:;
12:128MB:145MB:16.8MB:fat16:EFI-SYSTEM:boot;
5:145MB:2292MB:2147MB::ROOT-B:;
3:2292MB:4440MB:2147MB:ext2:ROOT-A:;
1:4440MB:7661MB:3221MB:ext4:STATE:;
"""

  SAMPLE_CGPT = """
       start        size    part  contents
           0           1          PMBR (Boot GUID: 88FB7EB8-2B3F-B943-B933-\
EEC571FFB6E1)
           1           1          Pri GPT header
           2          32          Pri GPT table
     1921024     2097152       1  Label: "STATE"
                                  Type: Linux data
                                  UUID: EEBD83BE-397E-BD44-878B-0DDDD5A5C510
       20480       32768       2  Label: "KERN-A"
                                  Type: ChromeOS kernel
                                  UUID: 7007C2F3-08E5-AB40-A4BC-FF5B01F5460D
                                  Attr: priority=15 tries=15 successful=1
     1101824      819200       3  Label: "ROOT-A"
                                  Type: ChromeOS rootfs
                                  UUID: F4C5C3AD-027F-894B-80CD-3DEC57932948
       53248       32768       4  Label: "KERN-B"
                                  Type: ChromeOS kernel
                                  UUID: C85FB478-404C-8741-ADB8-11312A35880D
                                  Attr: priority=0 tries=0 successful=0
      282624      819200       5  Label: "ROOT-B"
                                  Type: ChromeOS rootfs
                                  UUID: A99F4231-1EC3-C542-AC0C-DF3729F5DB07
       16448           1       6  Label: "KERN-C"
                                  Type: ChromeOS kernel
                                  UUID: 81F0E336-FAC9-174D-A08C-864FE627B637
                                  Attr: priority=0 tries=0 successful=0
       16449           1       7  Label: "ROOT-C"
                                  Type: ChromeOS rootfs
                                  UUID: 9E127FCA-30C1-044E-A5F2-DF74E6932692
       86016       32768       8  Label: "OEM"
                                  Type: Linux data
                                  UUID: 72986347-A37C-684F-9A19-4DBAF41C55A9
       16450           1       9  Label: "reserved"
                                  Type: ChromeOS reserved
                                  UUID: BA85A0A7-1850-964D-8EF8-6707AC106C3A
       16451           1      10  Label: "reserved"
                                  Type: ChromeOS reserved
                                  UUID: 16C9EC9B-50FA-DD46-98DC-F781360817B4
          64       16384      11  Label: "RWFW"
                                  Type: ChromeOS firmware
                                  UUID: BE8AECB9-4F78-7C44-8F23-5A9273B7EC8F
      249856       32768      12  Label: "EFI-SYSTEM"
                                  Type: EFI System Partition
                                  UUID: 88FB7EB8-2B3F-B943-B933-EEC571FFB6E1
     4050847          32          Sec GPT table
     4050879           1          Sec GPT header
"""

  def testCgpt(self):
    """Tests that we can list all partitions with `cgpt` correctly."""
    self.PatchObject(cros_build_lib, 'IsInsideChroot', return_value=True)
    self.rc.AddCmdResult(partial_mock.Ignore(), output=self.SAMPLE_CGPT)
    partitions = cros_build_lib.GetImageDiskPartitionInfo('...', unit='B')
    self.assertEqual(partitions['STATE'].start, 983564288)
    self.assertEqual(partitions['STATE'].size, 1073741824)
    self.assertEqual(partitions['STATE'].number, 1)
    self.assertEqual(partitions['STATE'].name, 'STATE')
    self.assertEqual(partitions['EFI-SYSTEM'].start, 249856 * 512)
    self.assertEqual(partitions['EFI-SYSTEM'].size, 32768 * 512)
    self.assertEqual(partitions['EFI-SYSTEM'].number, 12)
    self.assertEqual(partitions['EFI-SYSTEM'].name, 'EFI-SYSTEM')
    # Because "reserved" is duplicated, we only have 11 key-value pairs.
    self.assertEqual(11, len(partitions))

  def testNormalPath(self):
    self.PatchObject(cros_build_lib, 'IsInsideChroot', return_value=False)
    self.rc.AddCmdResult(partial_mock.Ignore(), output=self.SAMPLE_PARTED)
    partitions = cros_build_lib.GetImageDiskPartitionInfo('_ignored')
    # Because "reserved" is duplicated, we only have 11 key-value pairs.
    self.assertEqual(11, len(partitions))
    self.assertEqual(1, partitions['STATE'].number)
    self.assertEqual(2147, partitions['ROOT-A'].size)

  def testKeyedByNumber(self):
    self.PatchObject(cros_build_lib, 'IsInsideChroot', return_value=False)
    self.rc.AddCmdResult(partial_mock.Ignore(), output=self.SAMPLE_PARTED)
    partitions = cros_build_lib.GetImageDiskPartitionInfo(
        '_ignored', key_selector='number'
    )
    self.assertEqual(12, len(partitions))
    self.assertEqual('STATE', partitions[1].name)
    self.assertEqual(2147, partitions[3].size)
    self.assertEqual('reserved', partitions[9].name)
    self.assertEqual('reserved', partitions[10].name)

  def testChangeUnitOutsideChroot(self):

    def changeUnit(unit):
      cros_build_lib.GetImageDiskPartitionInfo('_ignored', unit)
      self.assertCommandContains(
          ['-m', '_ignored', 'unit', unit, 'print'],
      )

    self.PatchObject(cros_build_lib, 'IsInsideChroot', return_value=False)
    self.rc.AddCmdResult(partial_mock.Ignore(), output=self.SAMPLE_PARTED)
    # We must use 2-char units here because the mocked output is in 'MB'.
    changeUnit('MB')
    changeUnit('KB')

  def testChangeUnitInsideChroot(self):
    self.PatchObject(cros_build_lib, 'IsInsideChroot', return_value=True)
    self.rc.AddCmdResult(partial_mock.Ignore(), output=self.SAMPLE_CGPT)
    partitions = cros_build_lib.GetImageDiskPartitionInfo('_ignored', 'B')
    self.assertEqual(partitions['STATE'].start, 983564288)
    self.assertEqual(partitions['STATE'].size, 1073741824)
    partitions = cros_build_lib.GetImageDiskPartitionInfo('_ignored', 'KB')
    self.assertEqual(partitions['STATE'].start, 983564288 / 1000.0)
    self.assertEqual(partitions['STATE'].size, 1073741824 / 1000.0)
    partitions = cros_build_lib.GetImageDiskPartitionInfo('_ignored', 'MB')
    self.assertEqual(partitions['STATE'].start, 983564288 / 10.0**6)
    self.assertEqual(partitions['STATE'].size, 1073741824 / 10.0**6)

    self.assertRaises(KeyError, cros_build_lib.GetImageDiskPartitionInfo,
                      '_ignored', 'PB')


class CreateTarballTests(cros_test_lib.TempDirTestCase):
  """Test the CreateTarball function."""

  def setUp(self):
    """Create files/dirs needed for tar test."""
    self.target = os.path.join(self.tempdir, 'test.tar.xz')
    self.inputDir = os.path.join(self.tempdir, 'inputs')
    self.inputs = [
        'inputA',
        'inputB',
        'sub/subfile',
        'sub2/subfile',
    ]

    self.inputsWithDirs = [
        'inputA',
        'inputB',
        'sub',
        'sub2',
    ]


    # Create the input files.
    for i in self.inputs:
      osutils.WriteFile(os.path.join(self.inputDir, i), i, makedirs=True)

  def testSuccess(self):
    """Create a tarfile."""
    cros_build_lib.CreateTarball(self.target, self.inputDir,
                                 inputs=self.inputs)

  def testSuccessWithDirs(self):
    """Create a tarfile."""
    cros_build_lib.CreateTarball(self.target, self.inputDir,
                                 inputs=self.inputsWithDirs)

# Tests for tar failure retry logic.

class FailedCreateTarballTests(cros_test_lib.MockTestCase):
  """Tests special case error handling for CreateTarBall."""

  def setUp(self):
    """Mock RunCommand mock."""
    # Each test can change this value as needed.  Each element is the return
    # code in the CommandResult for subsequent calls to RunCommand().
    self.tarResults = []

    def Result(*_args, **_kwargs):
      """Creates CommandResult objects for each tarResults value in turn."""
      return cros_build_lib.CommandResult(returncode=self.tarResults.pop(0))

    self.mockRun = self.PatchObject(cros_build_lib, 'RunCommand',
                                    autospec=True,
                                    side_effect=Result)

  def testSuccess(self):
    """CreateTarball works the first time."""
    self.tarResults = [0]
    cros_build_lib.CreateTarball('foo', 'bar', inputs=['a', 'b'])

    self.assertEqual(self.mockRun.call_count, 1)

  def testFailedOnceSoft(self):
    """Force a single retry for CreateTarball."""
    self.tarResults = [1, 0]
    cros_build_lib.CreateTarball('foo', 'bar', inputs=['a', 'b'])

    self.assertEqual(self.mockRun.call_count, 2)

  def testFailedOnceHard(self):
    """Test unrecoverable error."""
    self.tarResults = [2]
    with self.assertRaises(cros_build_lib.RunCommandError) as cm:
      cros_build_lib.CreateTarball('foo', 'bar', inputs=['a', 'b'])

    self.assertEqual(self.mockRun.call_count, 1)
    self.assertEqual(cm.exception.args[1].returncode, 2)

  def testFailedTwiceSoft(self):
    """Exhaust retries for recoverable errors."""
    self.tarResults = [1, 1]
    with self.assertRaises(cros_build_lib.RunCommandError) as cm:
      cros_build_lib.CreateTarball('foo', 'bar', inputs=['a', 'b'])

    self.assertEqual(self.mockRun.call_count, 2)
    self.assertEqual(cm.exception.args[1].returncode, 1)
