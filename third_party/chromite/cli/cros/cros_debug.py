# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""cros debug: Debug the applications on the target device."""

from __future__ import print_function

import os

from chromite.cli import command
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import remote_access


@command.CommandDecorator('debug')
class DebugCommand(command.CliCommand):
  """Use GDB to debug a process running on the target device.

  This command starts a GDB session to debug a remote process running on the
  target device. The remote process can either be an existing process or newly
  started by calling this command.

  This command can also be used to find out information about all running
  processes of an executable on the target device.
  """

  EPILOG = """
To list all running processes of an executable:
  cros debug device --list --exe=/path/to/executable

To debug an executable:
  cros debug device --exe=/path/to/executable

To debug a process by its pid:
  cros debug device --pid=1234
"""

  def __init__(self, options):
    """Initialize DebugCommand."""
    super(DebugCommand, self).__init__(options)
    # SSH connection settings.
    self.ssh_hostname = None
    self.ssh_port = None
    self.ssh_username = None
    self.ssh_private_key = None
    # The board name of the target device.
    self.board = None
    # Settings of the process to debug.
    self.list = False
    self.exe = None
    self.pid = None
    # The command for starting gdb.
    self.gdb_cmd = None

  @classmethod
  def AddParser(cls, parser):
    """Add parser arguments."""
    super(cls, DebugCommand).AddParser(parser)
    cls.AddDeviceArgument(parser)
    parser.add_argument(
        '--board', default=None, help='The board to use. By default it is '
        'automatically detected. You can override the detected board with '
        'this option.')
    parser.add_argument(
        '--private-key', type='path', default=None,
        help='SSH identity file (private key).')
    parser.add_argument(
        '-l', '--list', action='store_true', default=False,
        help='List running processes of the executable on the target device.')
    parser.add_argument(
        '--exe', help='Full path of the executable on the target device.')
    parser.add_argument(
        '-p', '--pid', type=int,
        help='The pid of the process on the target device.')

  def _ListProcesses(self, device, pids):
    """Provided with a list of pids, print out information of the processes."""
    if not pids:
      logging.info(
          'No running process of %s on device %s', self.exe, self.ssh_hostname)
      return

    try:
      result = device.BaseRunCommand(['ps', 'aux'])
      lines = result.output.splitlines()
      try:
        header, procs = lines[0], lines[1:]
        info = os.linesep.join([p for p in procs if int(p.split()[1]) in pids])
      except ValueError:
        cros_build_lib.Die('Parsing output failed:\n%s', result.output)

      print('\nList running processes of %s on device %s:\n%s\n%s' %
            (self.exe, self.ssh_hostname, header, info))
    except cros_build_lib.RunCommandError:
      cros_build_lib.Die(
          'Failed to find any running process on device %s', self.ssh_hostname)

  def _DebugNewProcess(self):
    """Start a new process on the target device and attach gdb to it."""
    logging.info(
        'Ready to start and debug %s on device %s', self.exe, self.ssh_hostname)
    cros_build_lib.RunCommand(self.gdb_cmd + ['--remote_file', self.exe])

  def _DebugRunningProcess(self, pid):
    """Start gdb and attach it to the remote running process with |pid|."""
    logging.info(
        'Ready to debug process %d on device %s', pid, self.ssh_hostname)
    cros_build_lib.RunCommand(self.gdb_cmd + ['--remote_pid', str(pid)])

  def _ReadOptions(self):
    """Process options and set variables."""
    if self.options.device:
      self.ssh_hostname = self.options.device.hostname
      self.ssh_username = self.options.device.username
      self.ssh_port = self.options.device.port
    self.ssh_private_key = self.options.private_key
    self.list = self.options.list
    self.exe = self.options.exe
    self.pid = self.options.pid

  def Run(self):
    """Run cros debug."""
    commandline.RunInsideChroot(self)
    self.options.Freeze()
    self._ReadOptions()
    with remote_access.ChromiumOSDeviceHandler(
        self.ssh_hostname, port=self.ssh_port, username=self.ssh_username,
        private_key=self.ssh_private_key) as device:
      self.board = cros_build_lib.GetBoard(device_board=device.board,
                                           override_board=self.options.board)
      logging.info('Board is %s', self.board)

      self.gdb_cmd = [
          'gdb_remote', '--ssh',
          '--board', self.board,
          '--remote', self.ssh_hostname,
      ]
      if self.ssh_port:
        self.gdb_cmd.extend(['--ssh_port', str(self.ssh_port)])

      if not (self.pid or self.exe):
        cros_build_lib.Die(
            'Must use --exe or --pid to specify the process to debug.')

      if self.pid:
        if self.list or self.exe:
          cros_build_lib.Die(
              '--list and --exe are disallowed when --pid is used.')
        self._DebugRunningProcess(self.pid)
        return

      if not self.exe.startswith('/'):
        cros_build_lib.Die('--exe must have a full pathname.')
      logging.debug('Executable path is %s', self.exe)
      if not device.IsFileExecutable(self.exe):
        cros_build_lib.Die(
            'File path "%s" does not exist or is not executable on device %s',
            self.exe, self.ssh_hostname)

      pids = device.GetRunningPids(self.exe)
      self._ListProcesses(device, pids)

      if self.list:
        # If '--list' flag is on, do not launch GDB.
        return

      if pids:
        choices = ['Start a new process under GDB']
        choices.extend(pids)
        idx = cros_build_lib.GetChoice(
            'Please select the process pid to debug (select [0] to start a '
            'new process):', choices)
        if idx == 0:
          self._DebugNewProcess()
        else:
          self._DebugRunningProcess(pids[idx - 1])
      else:
        self._DebugNewProcess()
