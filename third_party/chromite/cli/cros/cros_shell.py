# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""cros shell: Open a remote shell on the target device."""

from __future__ import print_function

import argparse

from chromite.cli import command
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import remote_access


@command.CommandDecorator('shell')
class ShellCommand(command.CliCommand):
  """Opens a remote shell over SSH on the target device.

  Can be used to start an interactive session or execute a command
  remotely. Interactive sessions can be terminated like a normal SSH
  session using Ctrl+D, `exit`, or `logout`.

  Unlike other `cros` commands, this allows for both SSH key and user
  password authentication. Because a password may be transmitted, the
  known_hosts file is used by default to protect against connecting to
  the wrong device.

  The exit code will be the same as the last executed command.
  """

  EPILOG = """
Examples:
  Start an interactive session:
    cros shell <ip>
    cros shell <user>@<ip>:<port>

  Non-interactive remote command:
    cros shell <ip> -- cat var/log/messages

Quoting can be tricky; the rules are the same as with ssh:
  Special symbols will end the command unless quoted:
    cros shell <ip> -- cat /var/log/messages > log.txt   (saves locally)
    cros shell <ip> -- "cat /var/log/messages > log.txt" (saves remotely)

  One set of quotes is consumed locally, so remote commands that
  require quotes will need double quoting:
    cros shell <ip> -- sh -c "exit 42"    (executes: sh -c exit 42)
    cros shell <ip> -- sh -c "'exit 42'"  (executes: sh -c 'exit 42')
"""

  def __init__(self, options):
    """Initializes ShellCommand."""
    super(ShellCommand, self).__init__(options)
    # ChromiumOSDevice to connect to.
    self.device = None
    # SSH connection settings.
    self.ssh_hostname = None
    self.ssh_port = None
    self.ssh_username = None
    self.ssh_private_key = None
    # Whether to use the SSH known_hosts file or not.
    self.known_hosts = None
    # How to set SSH StrictHostKeyChecking. Can be 'no', 'yes', or 'ask'. Has
    # no effect if |known_hosts| is not True.
    self.host_key_checking = None
    # The command to execute remotely.
    self.command = None

  @classmethod
  def AddParser(cls, parser):
    """Adds a parser."""
    super(cls, ShellCommand).AddParser(parser)
    cls.AddDeviceArgument(parser)
    parser.add_argument(
        '--private-key', type='path', default=None,
        help='SSH identify file (private key).')
    parser.add_argument(
        '--no-known-hosts', action='store_false', dest='known_hosts',
        default=True, help='Do not use a known_hosts file.')
    parser.add_argument(
        'command', nargs=argparse.REMAINDER,
        help='(optional) Command to execute on the device.')

  def _ReadOptions(self):
    """Processes options and set variables."""
    self.ssh_hostname = self.options.device.hostname
    self.ssh_username = self.options.device.username
    self.ssh_port = self.options.device.port
    self.ssh_private_key = self.options.private_key
    self.known_hosts = self.options.known_hosts
    # By default ask the user if a new key is found. SSH will still reject
    # modified keys for existing hosts without asking the user.
    self.host_key_checking = 'ask'
    # argparse doesn't always handle -- correctly.
    self.command = self.options.command
    if self.command and self.command[0] == '--':
      self.command.pop(0)

  def _ConnectSettings(self):
    """Generates the correct SSH connect settings based on our state."""
    kwargs = {'NumberOfPasswordPrompts': 2}
    if self.known_hosts:
      # Use the default known_hosts and our current key check setting.
      kwargs['UserKnownHostsFile'] = None
      kwargs['StrictHostKeyChecking'] = self.host_key_checking
    return remote_access.CompileSSHConnectSettings(**kwargs)

  def _UserConfirmKeyChange(self):
    """Asks the user whether it's OK that a host key has changed.

    A changed key can be fairly common during Chrome OS development, so
    instead of outright rejecting a modified key like SSH does, this
    provides some common reasons a key may have changed to help the
    user decide whether it was legitimate or not.

    _StartSsh() must have been called before this function so that
    |self.device| is valid.

    Returns:
      True if the user is OK with a changed host key.
    """
    return cros_build_lib.BooleanPrompt(
        prolog='The host ID for "%s" has changed since last connect.\n'
               'Some common reasons for this are:\n'
               ' - Device powerwash.\n'
               ' - Device flash from a USB stick.\n'
               ' - Device flash using "--clobber-stateful".\n'
               'Otherwise, please verify that this is the correct device'
               ' before continuing.' % self.device.hostname)

  def _StartSsh(self):
    """Starts an SSH session or executes a remote command.

    Also creates |self.device| if it doesn't yet exist. It's created
    once and saved so that if the user wants to use the default device,
    we only have to go through the discovery procedure the first time.

    Requires that _ReadOptions() has already been called to provide the
    SSH configuration.

    Returns:
      The SSH return code.

    Raises:
      SSHConnectionError on SSH connect failure.
    """
    # Create the ChromiumOSDevice the first time through this function.
    if not self.device:
      # Set |base_dir| to None to avoid the SSH setup commands which
      # could require the user to enter a password multiple times. We don't
      # need any of the additional functionality that |base_dir| enables.
      self.device = remote_access.ChromiumOSDevice(
          self.ssh_hostname,
          port=self.ssh_port,
          username=self.ssh_username,
          base_dir=None,
          private_key=self.ssh_private_key,
          ping=False)
    return self.device.BaseRunCommand(
        self.command,
        connect_settings=self._ConnectSettings(),
        error_code_ok=True,
        mute_output=False,
        redirect_stderr=True,
        capture_output=False).returncode

  def Run(self):
    """Runs `cros shell`."""
    self.options.Freeze()
    self._ReadOptions()
    try:
      return self._StartSsh()
    except remote_access.SSHConnectionError as e:
      # Handle a mismatched host key; mismatched keys are a bit of a pain to
      # fix manually since `ssh-keygen -R` doesn't work within the chroot.
      if e.IsKnownHostsMismatch():
        # The full SSH error message has extra info for the user.
        logging.warning('\n%s', e)
        if self._UserConfirmKeyChange():
          remote_access.RemoveKnownHost(self.device.hostname)
          # The user already OK'd so we can skip the additional SSH check.
          self.host_key_checking = 'no'
          return self._StartSsh()
        else:
          return 1
      raise
