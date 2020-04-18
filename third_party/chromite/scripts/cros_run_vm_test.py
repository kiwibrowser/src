# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script for running tests in a VM."""

from __future__ import print_function

import argparse
import datetime
import os
import re

from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import osutils
from chromite.scripts import cros_vm


class VMTest(object):
  """Class for running VM tests."""

  def __init__(self, opts):
    """Initialize VMTest.

    Args:
      opts: command line options.
    """
    self.start_time = datetime.datetime.utcnow()

    self.start_vm = opts.start_vm
    self.board = opts.board
    self.cache_dir = opts.cache_dir

    self.build = opts.build
    self.deploy = opts.deploy
    self.build_dir = opts.build_dir

    self.catapult_tests = opts.catapult_tests
    self.guest = opts.guest

    self.autotest = opts.autotest
    self.results_dir = opts.results_dir

    self.remote_cmd = opts.remote_cmd
    self.host_cmd = opts.host_cmd
    self.cwd = opts.cwd
    self.files = opts.files
    self.files_from = opts.files_from
    self.args = opts.args[1:] if opts.args else None

    self.output = opts.output
    self.results_src = opts.results_src
    self.results_dest_dir = opts.results_dest_dir

    self._vm = cros_vm.VM(opts)

  def __del__(self):
    self._StopVM()

    logging.info('Time elapsed %s.',
                 datetime.datetime.utcnow() - self.start_time)

  def Run(self):
    """Start a VM, build/deploy, run tests, and stop the VM."""
    self._StartVM()

    self._Build()
    self._Deploy()

    returncode = self._RunTests()

    self._StopVM()
    return returncode

  def _StartVM(self):
    """Start a VM if necessary.

    If --start-vm is specified, we launch a new VM, otherwise we use an
    existing VM.
    """
    if not self._vm.IsRunning():
      self.start_vm = True

    if self.start_vm:
      self._vm.Start()
    self._vm.WaitForBoot()

  def _StopVM(self):
    """Stop the VM if necessary.

    If --start-vm was specified, we launched this VM, so we now stop it.
    """
    if self._vm and self.start_vm:
      self._vm.Stop()

  def _Build(self):
    """Build chrome."""
    if not self.build:
      return
    cros_build_lib.RunCommand(['autoninja', '-C', self.build_dir, 'chrome',
                               'chrome_sandbox', 'nacl_helper'],
                              log_output=True)

  def _Deploy(self):
    """Deploy chrome."""
    if not self.build and not self.deploy:
      return

    deploy_cmd = [
        'deploy_chrome', '--force',
        '--build-dir', self.build_dir,
        '--process-timeout', '180',
        '--to', 'localhost',
        '--port', str(self._vm.ssh_port),
    ]
    if self.board:
      deploy_cmd += ['--board', self.board]
    if self.cache_dir:
      deploy_cmd += ['--cache-dir', self.cache_dir]
    cros_build_lib.RunCommand(deploy_cmd, log_output=True)
    self._vm.WaitForBoot()

  def _RunCatapultTests(self):
    """Run catapult tests matching a pattern using run_tests.

    Returns:
      cros_build_lib.CommandResult object.
    """

    browser = 'system-guest' if self.guest else 'system'
    return self._vm.RemoteCommand([
        'python',
        '/usr/local/telemetry/src/third_party/catapult/telemetry/bin/run_tests',
        '--browser=%s' % browser,
    ] + self.catapult_tests)

  def _RunAutotest(self):
    """Run an autotest using test_that.

    Returns:
      cros_build_lib.CommandResult object.
    """
    cmd = ['test_that']
    if self.board:
      cmd += ['--board', self.board]
    if self.results_dir:
      cmd += ['--results_dir', self.results_dir]
    cmd += [
        '--no-quickmerge',
        '--ssh_options', '-F /dev/null -i /dev/null',
        'localhost:%d' % self._vm.ssh_port,
    ] + self.autotest
    return cros_build_lib.RunCommand(cmd, log_output=True)

  def _RunTests(self):
    """Run tests.

    Run user-specified tests, catapult tests, autotest, or the default,
    vm_sanity.

    Returns:
      Command execution return code.
    """
    if self.remote_cmd:
      result = self._RunVMCmd()
    elif self.host_cmd:
      result = cros_build_lib.RunCommand(self.args, log_output=True)
    elif self.catapult_tests:
      result = self._RunCatapultTests()
    elif self.autotest:
      result = self._RunAutotest()
    else:
      result = self._vm.RemoteCommand(['/usr/local/autotest/bin/vm_sanity.py'])

    self._OutputResults(result)
    self._FetchResults()
    return result.returncode

  def _OutputResults(self, result):
    """Log the output from RunTests.

    Args:
      result: cros_build_lib.CommandResult object.
    """
    result_str = 'succeeded' if result.returncode == 0 else 'failed'
    logging.info('Tests %s.', result_str)
    if not self.output:
      return

    # Skip SSH warning.
    suppress_list = [
        r'Warning: Permanently added .* to the list of known hosts']
    with open(self.output, 'w') as f:
      lines = result.output.splitlines(True)
      for line in lines:
        for suppress in suppress_list:
          if not re.search(suppress, line):
            f.write(line)

  def _FetchResults(self):
    """Fetch results files/directories."""
    if not self.results_src:
      return
    osutils.SafeMakedirs(self.results_dest_dir)
    for src in self.results_src:
      logging.info('Fetching %s to %s', src, self.results_dest_dir)
      self._vm.remote.CopyFromDevice(src=src, dest=self.results_dest_dir,
                                     mode='scp', error_code_ok=True,
                                     debug_level=logging.INFO)

  def _RunVMCmd(self):
    """Run a command in the VM.

    Copy src files to /usr/local/vm_test/, change working directory to self.cwd,
    run the command in self.args, and cleanup.

    Returns:
      cros_build_lib.CommandResult object.
    """
    DEST_BASE = '/usr/local/vm_test'
    files = FileList(self.files, self.files_from)
    # Copy files, preserving the directory structure.
    for f in files:
      # Trailing / messes up dirname.
      f = f.rstrip('/')
      dirname = os.path.join(DEST_BASE, os.path.dirname(f))
      self._vm.RemoteCommand(['mkdir', '-p', dirname])
      self._vm.remote.CopyToDevice(src=f, dest=dirname, mode='scp',
                                   debug_level=logging.INFO)

    # Make cwd an absolute path (if it isn't one) rooted in DEST_BASE.
    cwd = self.cwd
    if files and not (cwd and os.path.isabs(cwd)):
      cwd = os.path.join(DEST_BASE, cwd) if cwd else DEST_BASE
      self._vm.RemoteCommand(['mkdir', '-p', cwd])
    if cwd:
      # Run the remote command with cwd.
      cmd = '"cd %s && %s"' % (cwd, ' '.join(self.args))
      result = self._vm.RemoteCommand(cmd, stream_log=True, shell=True)
    else:
      result = self._vm.RemoteCommand(self.args, stream_log=True)

    # Cleanup.
    if files:
      self._vm.RemoteCommand(['rm', '-rf', DEST_BASE])

    return result

def ParseCommandLine(argv):
  """Parse the command line.

  Args:
    argv: Command arguments.

  Returns:
    List of parsed options for VMTest, and args to pass to VM.
  """

  cros_vm_parser = cros_vm.VM.GetParser()
  parser = argparse.ArgumentParser(description=__doc__,
                                   parents=[cros_vm_parser],
                                   add_help=False)
  parser.add_argument('--start-vm', action='store_true', default=False,
                      help='Start a new VM before running tests.')
  parser.add_argument('--catapult-tests', nargs='+',
                      help='Catapult test pattern to run, passed to run_tests.')
  parser.add_argument('--autotest', nargs='+',
                      help='Autotest test pattern to run, passed to test_that.')
  parser.add_argument('--results-dir', help='Autotest results directory.')
  parser.add_argument('--output', help='Save output to file.')
  parser.add_argument('--guest', action='store_true', default=False,
                      help='Run tests in incognito mode.')
  parser.add_argument('--build-dir',
                      help='Directory for building and deploying chrome.')
  parser.add_argument('--build', action='store_true', default=False,
                      help='Before running tests, build chrome using ninja, '
                      '--build-dir must be specified.')
  parser.add_argument('--deploy', action='store_true', default=False,
                      help='Before running tests, deploy chrome to the VM, '
                      '--build-dir must be specified.')
  parser.add_argument('--cwd', help='Change working directory.'
                      'An absolute path or a path relative to CWD on the host.')
  parser.add_argument('--files', default=[], action='append',
                      help='Files to scp to the VM.')
  parser.add_argument('--files-from',
                      help='File with list of files to copy to the VM.')
  parser.add_argument('--results-src', default=[], action='append',
                      help='Files/Directories to copy from '
                      'the VM into CWD after running the test.')
  parser.add_argument('--results-dest-dir', help='Destination directory to '
                      'copy results to.')
  parser.add_argument('--remote-cmd', action='store_true', default=False,
                      help='Run a command in the VM.')
  parser.add_argument('--host-cmd', action='store_true', default=False,
                      help='Run a command on the host.')

  opts = parser.parse_args(argv)

  if opts.build or opts.deploy:
    if not opts.build_dir:
      parser.error('Must specifiy --build-dir with --build or --deploy.')
    if not os.path.isdir(opts.build_dir):
      parser.error('%s is not a directory.' % opts.build_dir)

  if opts.results_src:
    for src in opts.results_src:
      if not os.path.isabs(src):
        parser.error('results-src must be absolute.')
    if not opts.results_dest_dir:
      parser.error('results-dest-dir must be specified with results-src.')
  if opts.results_dest_dir:
    if not opts.results_src:
      parser.error('results-src must be specified with results-dest-dir.')
    if os.path.isfile(opts.results_dest_dir):
      parser.error('results-dest-dir %s is an existing file.'
                   % opts.results_dest_dir)

  # Ensure command is provided. For eg, to copy out to the VM and run
  # out/unittest:
  # cros_run_vm_test --files out --cwd out --cmd -- ./unittest
  # Treat --cmd as --remote-cmd.
  opts.remote_cmd = opts.remote_cmd or opts.cmd
  if (opts.remote_cmd or opts.host_cmd) and len(opts.args) < 2:
    parser.error('Must specify test command to run.')
  # Verify additional args.
  if opts.args:
    if not opts.remote_cmd and not opts.host_cmd:
      parser.error('Additional args may be specified with either '
                   '--remote-cmd or --host-cmd: %s' % opts.args)
    if opts.args[0] != '--':
      parser.error('Additional args must start with \'--\': %s' % opts.args)

  # Verify CWD.
  if opts.cwd:
    if opts.cwd.startswith('..'):
      parser.error('cwd cannot start with ..')
    if not os.path.isabs(opts.cwd) and not opts.files and not opts.files_from:
      parser.error('cwd must be an absolute path if '
                   '--files or --files-from is not specified')

  # Verify files.
  if opts.files_from:
    if opts.files:
      parser.error('--files and --files-from cannot both be specified')
    if not os.path.isfile(opts.files_from):
      parser.error('%s is not a file' % opts.files_from)
  files = FileList(opts.files, opts.files_from)
  for f in files:
    if os.path.isabs(f):
      parser.error('%s should be a relative path' % f)
    # Restrict paths to under CWD on the host. See crbug.com/829612.
    if f.startswith('..'):
      parser.error('%s cannot start with ..' % f)
    if not os.path.exists(f):
      parser.error('%s does not exist' % f)

  return opts


def FileList(files, files_from):
  """Get list of files from command line args --files and --files-from.

  Args:
    files: files specified directly on the command line.
    files_from: files specified in a file.

  Returns:
    Contents of files_from if it exists, otherwise files.
  """
  if files_from and os.path.isfile(files_from):
    with open(files_from) as f:
      files = [line.rstrip() for line in f]
  return files

def main(argv):
  return VMTest(ParseCommandLine(argv)).Run()
