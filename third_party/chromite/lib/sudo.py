# -*- coding: utf-8 -*-
# Copyright (c) 2011-2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Helper methods and classes related to managing sudo."""

from __future__ import print_function

import errno
import os
import signal
import subprocess
import sys

from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging


class SudoKeepAlive(cros_build_lib.MasterPidContextManager):
  """Keep sudo auth cookie fresh.

  This refreshes the sudo auth cookie; this is implemented this
  way to ensure that sudo has access to both invoking tty, and
  will update the user's tty-less cookie.
  see crosbug/18393.
  """

  def __init__(self, ttyless_sudo=True, repeat_interval=4):
    """Run sudo with a noop, to reset the sudo timestamp.

    Args:
      ttyless_sudo: Whether to update the tty-less cookie.
      repeat_interval: In minutes, the frequency to run the update.
    """
    cros_build_lib.MasterPidContextManager.__init__(self)
    self._ttyless_sudo = ttyless_sudo
    self._repeat_interval = repeat_interval
    self._proc = None
    self._existing_keepalive_value = None

  @staticmethod
  def _IdentifyTTY():
    for source in (sys.stdin, sys.stdout, sys.stderr):
      try:
        return os.ttyname(source.fileno())
      except EnvironmentError as e:
        if e.errno not in (errno.EINVAL, errno.ENOTTY):
          raise

    return 'unknown'

  def _DaemonNeeded(self):
    """Discern which TTYs require sudo keep alive code.

    Returns:
      A string representing the set of ttys we need daemons for.
      This will be the empty string if no daemon is needed.
    """
    existing = os.environ.get('CROS_SUDO_KEEP_ALIVE')
    needed = set([self._IdentifyTTY()])
    if self._ttyless_sudo:
      needed.add('unknown')
    if existing is not None:
      needed -= set(existing.split(':'))
    return ':'.join(needed)

  def _enter(self):
    if os.getuid() == 0:
      cros_build_lib.Die('This script cannot be run as root.')

    start_for_tty = self._DaemonNeeded()
    if not start_for_tty:
      # Daemon is already started.
      return

    # Note despite the impulse to use 'sudo -v' instead of 'sudo true', the
    # builder's sudoers configuration is slightly whacked resulting in it
    # asking for password everytime.  As such use 'sudo true' instead.
    cmds = ['sudo -n true 2>/dev/null',
            'sudo -n true < /dev/null > /dev/null 2>&1']

    # First check to see if we're already authed.  If so, then we don't
    # need to prompt the user for their password.
    for idx, cmd in enumerate(cmds):
      ret = cros_build_lib.RunCommand(
          cmd, print_cmd=False, shell=True, error_code_ok=True)

      if ret.returncode != 0:
        tty_msg = 'Please disable tty_tickets using these instructions: %s'
        if os.path.exists("/etc/goobuntu"):
          url = 'https://goto.google.com/chromeos-sudoers'
        else:
          url = 'https://goo.gl/fz9YW'

        # If ttyless sudo is not strictly required for this script, don't
        # prompt for a password a second time. Instead, just complain.
        if idx > 0:
          logging.error(tty_msg, url)
          if not self._ttyless_sudo:
            break

        # We need to go interactive and allow sudo to ask for credentials.
        interactive_cmd = cmd.replace(' -n', '')
        cros_build_lib.RunCommand(interactive_cmd, shell=True, print_cmd=False)

        # Verify that sudo access is set up properly.
        try:
          cros_build_lib.RunCommand(cmd, shell=True, print_cmd=False)
        except cros_build_lib.RunCommandError:
          if idx == 0:
            raise
          cros_build_lib.Die('tty_tickets must be disabled. ' + tty_msg, url)

    # Anything other than a timeout results in us shutting down.
    repeat_interval = self._repeat_interval * 60
    cmd = ('while :; do read -t %i; [ $? -le 128 ] && exit; %s; done' %
           (repeat_interval, '; '.join(cmds)))

    def ignore_sigint():
      # We don't want our sudo process shutdown till we shut it down;
      # since it's part of the session group it however gets SIGINT.
      # Thus suppress it (which bash then inherits).
      signal.signal(signal.SIGINT, signal.SIG_IGN)

    self._proc = subprocess.Popen(['bash', '-c', cmd], shell=False,
                                  close_fds=True, preexec_fn=ignore_sigint,
                                  stdin=subprocess.PIPE)

    self._existing_keepalive_value = os.environ.get('CROS_SUDO_KEEP_ALIVE')
    os.environ['CROS_SUDO_KEEP_ALIVE'] = start_for_tty

  # pylint: disable=W0613
  def _exit(self, exc_type, exc_value, traceback):
    if self._proc is None:
      return

    try:
      self._proc.terminate()
      self._proc.wait()
    except EnvironmentError as e:
      if e.errno != errno.ESRCH:
        raise

    if self._existing_keepalive_value is not None:
      os.environ['CROS_SUDO_KEEP_ALIVE'] = self._existing_keepalive_value
    else:
      os.environ.pop('CROS_SUDO_KEEP_ALIVE', None)


def SetFileContents(path, value, cwd=None):
  """Set a given filepath contents w/ the passed in value."""
  cros_build_lib.SudoRunCommand(['tee', path], redirect_stdout=True,
                                print_cmd=False, input=value, cwd=cwd)
