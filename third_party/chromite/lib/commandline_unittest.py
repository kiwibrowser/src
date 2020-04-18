# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test the commandline module."""

from __future__ import print_function

import argparse
import cPickle
import signal
import os
import sys

from chromite.cli import command
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import gs
from chromite.lib import path_util


# pylint: disable=protected-access


class TestShutDownException(cros_test_lib.TestCase):
  """Test that ShutDownException can be pickled."""

  def testShutDownException(self):
    """Test that ShutDownException can be pickled."""
    ex = commandline._ShutDownException(signal.SIGTERM, 'Received SIGTERM')
    ex2 = cPickle.loads(cPickle.dumps(ex))
    self.assertEqual(ex.signal, ex2.signal)
    self.assertEqual(ex.message, ex2.message)


class GSPathTest(cros_test_lib.OutputTestCase):
  """Test type=gs_path normalization functionality."""

  GS_REL_PATH = 'bucket/path/to/artifacts'

  @staticmethod
  def _ParseCommandLine(argv):
    parser = commandline.ArgumentParser()
    parser.add_argument('-g', '--gs-path', type='gs_path',
                        help='GS path that contains the chrome to deploy.')
    return parser.parse_args(argv)

  def _RunGSPathTestCase(self, raw, parsed):
    options = self._ParseCommandLine(['--gs-path', raw])
    self.assertEquals(options.gs_path, parsed)

  def testNoGSPathCorrectionNeeded(self):
    """Test case where GS path correction is not needed."""
    gs_path = '%s/%s' % (gs.BASE_GS_URL, self.GS_REL_PATH)
    self._RunGSPathTestCase(gs_path, gs_path)

  def testTrailingSlashRemoval(self):
    """Test case where GS path ends with /."""
    gs_path = '%s/%s/' % (gs.BASE_GS_URL, self.GS_REL_PATH)
    self._RunGSPathTestCase(gs_path, gs_path.rstrip('/'))

  def testDuplicateSlashesRemoved(self):
    """Test case where GS path contains many / in a row."""
    self._RunGSPathTestCase(
        '%s/a/dir/with//////////slashes' % gs.BASE_GS_URL,
        '%s/a/dir/with/slashes' % gs.BASE_GS_URL)

  def testRelativePathsRemoved(self):
    """Test case where GS path contain /../ logic."""
    self._RunGSPathTestCase(
        '%s/a/dir/up/here/.././../now/down/there' % gs.BASE_GS_URL,
        '%s/a/dir/now/down/there' % gs.BASE_GS_URL)

  def testCorrectionNeeded(self):
    """Test case where GS path correction is needed."""
    self._RunGSPathTestCase(
        '%s/%s/' % (gs.PRIVATE_BASE_HTTPS_URL, self.GS_REL_PATH),
        '%s/%s' % (gs.BASE_GS_URL, self.GS_REL_PATH))

  def testInvalidPath(self):
    """Path cannot be normalized."""
    with self.OutputCapturer():
      self.assertRaises2(
          SystemExit, self._RunGSPathTestCase, 'http://badhost.com/path', '',
          check_attrs={'code': 2})


class BoolTest(cros_test_lib.TestCase):
  """Test type='bool' functionality."""

  @staticmethod
  def _ParseCommandLine(argv):
    parser = commandline.ArgumentParser()
    parser.add_argument('-e', '--enable', type='bool',
                        help='Boolean Argument.')
    return parser.parse_args(argv)

  def _RunBoolTestCase(self, enable, expected):
    options = self._ParseCommandLine(['--enable', enable])
    self.assertEquals(options.enable, expected)

  def testBoolTrue(self):
    """Test case setting the value to true."""
    self._RunBoolTestCase('True', True)
    self._RunBoolTestCase('1', True)
    self._RunBoolTestCase('true', True)
    self._RunBoolTestCase('yes', True)
    self._RunBoolTestCase('TrUe', True)

  def testBoolFalse(self):
    """Test case setting the value to false."""
    self._RunBoolTestCase('False', False)
    self._RunBoolTestCase('0', False)
    self._RunBoolTestCase('false', False)
    self._RunBoolTestCase('no', False)
    self._RunBoolTestCase('FaLse', False)


class DeviceParseTest(cros_test_lib.OutputTestCase):
  """Test device parsing functionality."""

  _ALL_SCHEMES = (commandline.DEVICE_SCHEME_FILE,
                  commandline.DEVICE_SCHEME_SSH,
                  commandline.DEVICE_SCHEME_USB)

  def _CheckDeviceParse(self, device_input, scheme, username=None,
                        hostname=None, port=None, path=None):
    """Checks that parsing a device input gives the expected result.

    Args:
      device_input: String input specifying a device.
      scheme: String expected scheme.
      username: String expected username or None.
      hostname: String expected hostname or None.
      port: Int expected port or None.
      path: String expected path or None.
    """
    parser = commandline.ArgumentParser()
    parser.add_argument('device', type=commandline.DeviceParser(scheme))
    device = parser.parse_args([device_input]).device
    self.assertEqual(device.scheme, scheme)
    self.assertEqual(device.username, username)
    self.assertEqual(device.hostname, hostname)
    self.assertEqual(device.port, port)
    self.assertEqual(device.path, path)

  def _CheckDeviceParseFails(self, device_input, schemes=_ALL_SCHEMES):
    """Checks that parsing a device input fails.

    Args:
      device_input: String input specifying a device.
      schemes: A scheme or list of allowed schemes, by default allows all.
    """
    parser = commandline.ArgumentParser()
    parser.add_argument('device', type=commandline.DeviceParser(schemes))
    with self.OutputCapturer():
      self.assertRaises2(SystemExit, parser.parse_args, [device_input])

  def testNoDevice(self):
    """Verify that an empty device specification fails."""
    self._CheckDeviceParseFails('')

  def testSshScheme(self):
    """Verify that SSH scheme-only device specification fails."""
    self._CheckDeviceParseFails('ssh://')

  def testSshHostname(self):
    """Test SSH hostname-only device specification."""
    self._CheckDeviceParse('192.168.1.200',
                           scheme=commandline.DEVICE_SCHEME_SSH,
                           hostname='192.168.1.200')

  def testSshUsernameHostname(self):
    """Test SSH username and hostname device specification."""
    self._CheckDeviceParse('me@foo_host',
                           scheme=commandline.DEVICE_SCHEME_SSH,
                           username='me',
                           hostname='foo_host')

  def testSshUsernameHostnamePort(self):
    """Test SSH username, hostname, and port device specification."""
    self._CheckDeviceParse('me@foo_host:4500',
                           scheme=commandline.DEVICE_SCHEME_SSH,
                           username='me',
                           hostname='foo_host',
                           port=4500)

  def testSshSchemeUsernameHostnamePort(self):
    """Test SSH scheme, username, hostname, and port device specification."""
    self._CheckDeviceParse('ssh://me@foo_host:4500',
                           scheme=commandline.DEVICE_SCHEME_SSH,
                           username='me',
                           hostname='foo_host',
                           port=4500)

  def testUsbScheme(self):
    """Test USB scheme-only device specification."""
    self._CheckDeviceParse('usb://', scheme=commandline.DEVICE_SCHEME_USB)

  def testUsbSchemePath(self):
    """Test USB scheme and path device specification."""
    self._CheckDeviceParse('usb://path/to/my/device',
                           scheme=commandline.DEVICE_SCHEME_USB,
                           path='path/to/my/device')

  def testFileScheme(self):
    """Verify that file scheme-only device specification fails."""
    self._CheckDeviceParseFails('file://')

  def testFileSchemePath(self):
    """Test file scheme and path device specification."""
    self._CheckDeviceParse('file://foo/bar',
                           scheme=commandline.DEVICE_SCHEME_FILE,
                           path='foo/bar')

  def testAbsolutePath(self):
    """Verify that an absolute path defaults to file scheme."""
    self._CheckDeviceParse('/path/to/my/device',
                           scheme=commandline.DEVICE_SCHEME_FILE,
                           path='/path/to/my/device')

  def testUnsupportedScheme(self):
    """Verify that an unsupported scheme fails."""
    self._CheckDeviceParseFails('ssh://192.168.1.200',
                                schemes=commandline.DEVICE_SCHEME_USB)
    self._CheckDeviceParseFails('usb://path/to/my/device',
                                schemes=[commandline.DEVICE_SCHEME_SSH,
                                         commandline.DEVICE_SCHEME_FILE])

  def testUnknownScheme(self):
    """Verify that an unknown scheme fails."""
    self._CheckDeviceParseFails('ftp://192.168.1.200')

  def testSchemeCaseInsensitive(self):
    """Verify that schemes are case-insensitive."""
    self._CheckDeviceParse('SSH://foo_host',
                           scheme=commandline.DEVICE_SCHEME_SSH,
                           hostname='foo_host')


class AppendOptionTest(cros_test_lib.TestCase):
  """Verify append_option/append_option_value actions."""

  def setUp(self):
    """Create a standard parser for the tests."""
    self.parser = commandline.ArgumentParser()
    self.parser.add_argument('--flag', action='append_option')
    self.parser.add_argument('--value', action='append_option_value')
    self.parser.add_argument('-x', '--shared_flag', dest='shared',
                             action='append_option')
    self.parser.add_argument('-y', '--shared_value', dest='shared',
                             action='append_option_value')

  def testNone(self):
    """Test results when no arguments are passed in."""
    result = self.parser.parse_args([])
    self.assertDictContainsSubset(
        {'flag': None, 'value': None, 'shared': None},
        vars(result),
    )

  def testSingles(self):
    """Test results when no argument is used more than once."""
    result = self.parser.parse_args(
        ['--flag', '--value', 'foo', '--shared_flag', '--shared_value', 'bar']
    )

    self.assertDictContainsSubset(
        {
            'flag': ['--flag'],
            'value': ['--value', 'foo'],
            'shared': ['--shared_flag', '--shared_value', 'bar'],
        },
        vars(result),
    )

  def testMultiples(self):
    """Test results when no arguments are used more than once."""
    result = self.parser.parse_args([
        '--flag', '--value', 'v1',
        '-x', '-y', 's1',
        '--shared_flag', '--shared_value', 's2',
        '--flag', '--value', 'v2',
    ])

    self.assertDictContainsSubset(
        {
            'flag': ['--flag', '--flag'],
            'value': ['--value', 'v1', '--value', 'v2'],
            'shared': ['-x', '-y', 's1', '--shared_flag',
                       '--shared_value', 's2'],
        },
        vars(result),
    )


class SplitExtendActionTest(cros_test_lib.TestCase):
  """Verify _SplitExtendAction/split_extend action."""

  def _CheckArgs(self, cliargs, expected):
    """Check |cliargs| produces |expected|."""
    parser = commandline.ArgumentParser()
    parser.add_argument('-x', action='split_extend', default=[])
    opts = parser.parse_args(
        cros_build_lib.iflatten_instance(['-x', x] for x in cliargs))
    self.assertEqual(opts.x, expected)

  def testDefaultNone(self):
    """Verify default=None works."""
    parser = commandline.ArgumentParser()
    parser.add_argument('-x', action='split_extend', default=None)

    opts = parser.parse_args([])
    self.assertIs(opts.x, None)

    opts = parser.parse_args(['-x', ''])
    self.assertEqual(opts.x, [])

    opts = parser.parse_args(['-x', 'f'])
    self.assertEqual(opts.x, ['f'])

  def testNoArgs(self):
    """This is more of a sanity check for resting state."""
    self._CheckArgs([], [])

  def testEmptyArg(self):
    """Make sure '' produces nothing."""
    self._CheckArgs(['', ''], [])

  def testEmptyWhitespaceArg(self):
    """Make sure whitespace produces nothing."""
    self._CheckArgs([' ', '\t', '  \t   '], [])

  def testSingleSingleArg(self):
    """Verify splitting one arg works."""
    self._CheckArgs(['a'], ['a'])

  def testMultipleSingleArg(self):
    """Verify splitting one arg works."""
    self._CheckArgs(['a b  c\td '], ['a', 'b', 'c', 'd'])

  def testMultipleMultipleArgs(self):
    """Verify splitting multiple args works."""
    self._CheckArgs(['a b  c', '', 'x', ' k '], ['a', 'b', 'c', 'x', 'k'])


class CacheTest(cros_test_lib.MockTempDirTestCase):
  """Test cache dir default / override functionality."""

  CACHE_DIR = '/fake/cache/dir'

  def setUp(self):
    self.PatchObject(commandline.ArgumentParser, 'ConfigureCacheDir')
    dir_struct = [
        'repo/.repo/',
    ]
    cros_test_lib.CreateOnDiskHierarchy(self.tempdir, dir_struct)
    self.repo_root = os.path.join(self.tempdir, 'repo')
    self.cwd_mock = self.PatchObject(os, 'getcwd')
    self.parser = commandline.ArgumentParser(caching=True)

  def _CheckCall(self, cwd_retval, args_to_parse, expected, assert_func):
    # pylint: disable=E1101
    self.cwd_mock.return_value = cwd_retval
    self.parser.parse_args(args_to_parse)
    cache_dir_mock = self.parser.ConfigureCacheDir
    self.assertEquals(1, cache_dir_mock.call_count)
    assert_func(cache_dir_mock.call_args[0][0], expected)

  def testRepoRootNoOverride(self):
    """Test default cache location when in a repo checkout."""
    self._CheckCall(self.repo_root, [], self.repo_root, self.assertStartsWith)

  def testRepoRootWithOverride(self):
    """User provided cache location overrides repo checkout default."""
    self._CheckCall(self.repo_root, ['--cache-dir', self.CACHE_DIR],
                    self.CACHE_DIR, self.assertEquals)


class ParseArgsTest(cros_test_lib.TestCase):
  """Test parse_args behavior of our custom argument parsing classes."""

  def _CreateOptionParser(self, cls):
    """Create a class of optparse.OptionParser with prepared config.

    Args:
      cls: Some subclass of optparse.OptionParser.

    Returns:
      The created OptionParser object.
    """
    usage = 'usage: some usage'
    parser = cls(usage=usage)

    # Add some options.
    parser.add_option('-x', '--xxx', action='store_true', default=False,
                      help='Gimme an X')
    parser.add_option('-y', '--yyy', action='store_true', default=False,
                      help='Gimme a Y')
    parser.add_option('-a', '--aaa', type='string', default='Allan',
                      help='Gimme an A')
    parser.add_option('-b', '--bbb', type='string', default='Barry',
                      help='Gimme a B')
    parser.add_option('-c', '--ccc', type='string', default='Connor',
                      help='Gimme a C')

    return parser

  def _CreateArgumentParser(self, cls):
    """Create a class of argparse.ArgumentParser with prepared config.

    Args:
      cls: Some subclass of argparse.ArgumentParser.

    Returns:
      The created ArgumentParser object.
    """
    usage = 'usage: some usage'
    parser = cls(usage=usage)

    # Add some options.
    parser.add_argument('-x', '--xxx', action='store_true', default=False,
                        help='Gimme an X')
    parser.add_argument('-y', '--yyy', action='store_true', default=False,
                        help='Gimme a Y')
    parser.add_argument('-a', '--aaa', type=str, default='Allan',
                        help='Gimme an A')
    parser.add_argument('-b', '--bbb', type=str, default='Barry',
                        help='Gimme a B')
    parser.add_argument('-c', '--ccc', type=str, default='Connor',
                        help='Gimme a C')
    parser.add_argument('args', type=str, nargs='*', help='args')

    return parser

  def _TestParser(self, parser):
    """Test the given parser with a prepared argv."""
    argv = ['-x', '--bbb', 'Bobby', '-c', 'Connor', 'foobar']

    parsed = parser.parse_args(argv)

    if isinstance(parser, commandline.FilteringParser):
      # optparse returns options and args separately.
      options, args = parsed
      self.assertEquals(['foobar'], args)
    else:
      # argparse returns just options.  Options configured above to have the
      # args stored at option "args".
      options = parsed
      self.assertEquals(['foobar'], parsed.args)

    self.assertTrue(options.xxx)
    self.assertFalse(options.yyy)

    self.assertEquals('Allan', options.aaa)
    self.assertEquals('Bobby', options.bbb)
    self.assertEquals('Connor', options.ccc)

    self.assertRaises(AttributeError, getattr, options, 'xyz')

    # Now try altering option values.
    options.aaa = 'Arick'
    self.assertEquals('Arick', options.aaa)

    # Now freeze the options and try altering again.
    options.Freeze()
    self.assertRaises(commandline.cros_build_lib.AttributeFrozenError,
                      setattr, options, 'aaa', 'Arnold')
    self.assertEquals('Arick', options.aaa)

  def testFilterParser(self):
    self._TestParser(self._CreateOptionParser(commandline.FilteringParser))

  def testArgumentParser(self):
    self._TestParser(self._CreateArgumentParser(commandline.ArgumentParser))

  def testDisableCommonLogging(self):
    """Verify we can elide common logging options."""
    parser = commandline.ArgumentParser(logging=False)

    # Sanity check it first.
    opts = parser.parse_args([])
    self.assertFalse(hasattr(opts, 'log_level'))

    # Now add our own logging options.  If the options were added,
    # argparse would throw duplicate flag errors for us.
    parser.add_argument('--log-level')
    parser.add_argument('--nocolor')

  def testCommonBaseDefaults(self):
    """Make sure common options work with just a base parser."""
    parser = commandline.ArgumentParser(logging=True, default_log_level='info')

    # Make sure the default works.
    opts = parser.parse_args([])
    self.assertEqual(opts.log_level, 'info')
    self.assertEqual(opts.color, None)

    # Then we can set up our own values.
    opts = parser.parse_args(['--nocolor', '--log-level=notice'])
    self.assertEqual(opts.log_level, 'notice')
    self.assertEqual(opts.color, False)

  def testCommonBaseAndSubDefaults(self):
    """Make sure common options work between base & sub parsers."""
    parser = commandline.ArgumentParser(logging=True, default_log_level='info')

    sub_parsers = parser.add_subparsers(title='Subs')
    sub_parsers.add_parser('cmd1')
    sub_parsers.add_parser('cmd2')

    # Make sure the default works.
    opts = parser.parse_args(['cmd1'])
    self.assertEqual(opts.log_level, 'info')
    self.assertEqual(opts.color, None)

    # Make sure options passed to base parser work.
    opts = parser.parse_args(['--nocolor', '--log-level=notice', 'cmd2'])
    self.assertEqual(opts.log_level, 'notice')
    self.assertEqual(opts.color, False)

    # Make sure options passed to sub parser work.
    opts = parser.parse_args(['cmd2', '--nocolor', '--log-level=notice'])
    self.assertEqual(opts.log_level, 'notice')
    self.assertEqual(opts.color, False)


class ScriptWrapperMainTest(cros_test_lib.MockTestCase):
  """Test the behavior of the ScriptWrapperMain function."""

  def setUp(self):
    self.PatchObject(sys, 'exit')
    self.lastTargetFound = None

  SYS_ARGV = ['/cmd', '/cmd', 'arg1', 'arg2']
  CMD_ARGS = ['/cmd', 'arg1', 'arg2']
  CHROOT_ARGS = ['--workspace', '/work']

  def testRestartInChrootPreserveArgs(self):
    """Verify args to ScriptWrapperMain are passed through to chroot.."""
    # Setup Mocks/Fakes
    rc = self.StartPatcher(cros_test_lib.RunCommandMock())
    rc.SetDefaultCmdResult()

    def findTarget(target):
      """ScriptWrapperMain needs a function to find a function to run."""
      def raiseChrootRequiredError(args):
        raise commandline.ChrootRequiredError(args)

      self.lastTargetFound = target
      return raiseChrootRequiredError

    # Run Test
    commandline.ScriptWrapperMain(findTarget, self.SYS_ARGV)

    # Verify Results
    rc.assertCommandContains(enter_chroot=True)
    rc.assertCommandContains(self.CMD_ARGS)
    self.assertEqual('/cmd', self.lastTargetFound)

  def testRestartInChrootWithChrootArgs(self):
    """Verify args and chroot args from exception are used."""
    # Setup Mocks/Fakes
    rc = self.StartPatcher(cros_test_lib.RunCommandMock())
    rc.SetDefaultCmdResult()

    def findTarget(_):
      """ScriptWrapperMain needs a function to find a function to run."""
      def raiseChrootRequiredError(_args):
        raise commandline.ChrootRequiredError(self.CMD_ARGS, self.CHROOT_ARGS)

      return raiseChrootRequiredError

    # Run Test
    commandline.ScriptWrapperMain(findTarget, ['unrelated'])

    # Verify Results
    rc.assertCommandContains(enter_chroot=True)
    rc.assertCommandContains(self.CMD_ARGS)
    rc.assertCommandContains(chroot_args=self.CHROOT_ARGS)


class TestRunInsideChroot(cros_test_lib.MockTestCase):
  """Test commandline.RunInsideChroot()."""

  def setUp(self):
    self.orig_argv = sys.argv
    sys.argv = ['/cmd', 'arg1', 'arg2']

    self.mockFromHostToChrootPath = self.PatchObject(
        path_util, 'ToChrootPath', return_value='/inside/cmd')

    # Return values for these two should be set by each test.
    self.mock_inside_chroot = self.PatchObject(cros_build_lib, 'IsInsideChroot')

    # Mocked CliCommand object to pass to RunInsideChroot.
    self.cmd = command.CliCommand(argparse.Namespace())
    self.cmd.options.log_level = 'info'

  def teardown(self):
    sys.argv = self.orig_argv

  def _VerifyRunInsideChroot(self, expected_cmd, expected_chroot_args=None,
                             log_level_args=None, **kwargs):
    """Run RunInsideChroot, and verify it raises with expected values.

    Args:
      expected_cmd: Command that should be executed inside the chroot.
      expected_chroot_args: Args that should be passed as chroot args.
      log_level_args: Args that set the log level of cros_sdk.
      kwargs: Additional args to pass to RunInsideChroot().
    """
    with self.assertRaises(commandline.ChrootRequiredError) as cm:
      commandline.RunInsideChroot(self.cmd, **kwargs)

    if log_level_args is None:
      log_level_args = ['--log-level', self.cmd.options.log_level]

    if expected_chroot_args is not None:
      log_level_args.extend(expected_chroot_args)
      expected_chroot_args = log_level_args
    else:
      expected_chroot_args = log_level_args

    self.assertEqual(expected_cmd, cm.exception.cmd)
    self.assertEqual(expected_chroot_args, cm.exception.chroot_args)

  def testRunInsideChroot(self):
    """Test we can restart inside the chroot."""
    self.mock_inside_chroot.return_value = False
    self._VerifyRunInsideChroot(['/inside/cmd', 'arg1', 'arg2'])

  def testRunInsideChrootLogLevel(self):
    """Test chroot restart with properly inherited log-level."""
    self.cmd.options.log_level = 'notice'
    self.mock_inside_chroot.return_value = False
    self._VerifyRunInsideChroot(['/inside/cmd', 'arg1', 'arg2'],
                                log_level_args=['--log-level', 'notice'])

  def testRunInsideChrootAlreadyInside(self):
    """Test we don't restart inside the chroot if we are already there."""
    self.mock_inside_chroot.return_value = True

    # Since we are in the chroot, it should return, doing nothing.
    commandline.RunInsideChroot(self.cmd)
