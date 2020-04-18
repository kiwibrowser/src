# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Wrapper for running gdb.

This handles the fun details like running against the right sysroot, via
qemu, bind mounts, etc...
"""

from __future__ import print_function

import argparse
import contextlib
import errno
import os
import pipes
import sys
import tempfile

from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import namespaces
from chromite.lib import osutils
from chromite.lib import qemu
from chromite.lib import remote_access
from chromite.lib import retry_util
from chromite.lib import toolchain

class GdbException(Exception):
  """Base exception for this module."""


class GdbBadRemoteDeviceError(GdbException):
  """Raised when remote device does not exist or is not responding."""


class GdbMissingSysrootError(GdbException):
  """Raised when path to sysroot cannot be found in chroot."""


class GdbMissingInferiorError(GdbException):
  """Raised when the binary to be debugged cannot be found."""


class GdbMissingDebuggerError(GdbException):
  """Raised when cannot find correct version of debugger."""


class GdbCannotFindRemoteProcessError(GdbException):
  """Raised when cannot find requested executing process on remote device."""


class GdbUnableToStartGdbserverError(GdbException):
  """Raised when error occurs trying to start gdbserver on remote device."""


class GdbTooManyPidsError(GdbException):
  """Raised when more than one matching pid is found running on device."""


class GdbEarlyExitError(GdbException):
  """Raised when user requests to exit early."""


class GdbCannotDetectBoardError(GdbException):
  """Raised when board isn't specified and can't be automatically determined."""

class GdbSimpleChromeBinaryError(GdbException):
  """Raised when none or multiple chrome binaries are under out_${board} dir."""

class BoardSpecificGdb(object):
  """Framework for running gdb."""

  _BIND_MOUNT_PATHS = ('dev', 'dev/pts', 'proc', 'mnt/host/source', 'sys')
  _GDB = '/usr/bin/gdb'
  _EXTRA_SSH_SETTINGS = {
      'CheckHostIP': 'no',
      'BatchMode': 'yes',
      'LogLevel': 'QUIET'
  }
  _MISSING_DEBUG_INFO_MSG = """
%(inf_cmd)s is stripped and %(debug_file)s does not exist on your local machine.
  The debug symbols for that package may not be installed.  To install the debug
 symbols for %(package)s only, run:

   cros_install_debug_syms --board=%(board)s %(package)s

To install the debug symbols for all available packages, run:

   cros_install_debug_syms --board=%(board)s --all"""

  def __init__(self, board, gdb_args, inf_cmd, inf_args, remote, pid,
               remote_process_name, cgdb_flag, ping, binary):
    self.board = board
    self.sysroot = None
    self.prompt = '(gdb) '
    self.inf_cmd = inf_cmd
    self.run_as_root = False
    self.gdb_args = gdb_args
    self.inf_args = inf_args
    self.remote = remote.hostname if remote else None
    self.pid = pid
    self.remote_process_name = remote_process_name
    # Port used for sending ssh commands to DUT.
    self.remote_port = remote.port if remote else None
    # Port for communicating between gdb & gdbserver.
    self.gdbserver_port = remote_access.GetUnusedPort()
    self.ssh_settings = remote_access.CompileSSHConnectSettings(
        **self._EXTRA_SSH_SETTINGS)
    self.cgdb = cgdb_flag
    self.framework = 'auto'
    self.qemu = None
    self.device = None
    self.cross_gdb = None
    self.ping = ping
    self.binary = binary
    self.in_chroot = None
    self.chrome_path = None
    self.sdk_path = None

  def IsInChroot(self):
    """Decide whether we are in chroot or chrome-sdk."""
    return os.path.exists("/mnt/host/source/chromite/")

  def SimpleChromeGdb(self):
    """Get the name of the cross gdb based on board name."""
    bin_path = self.board + '+' + os.environ['SDK_VERSION'] + '+' + \
               'target_toolchain'
    bin_path = os.path.join(self.sdk_path, bin_path, 'bin')
    for f in os.listdir(bin_path):
      if f.endswith('gdb'):
        return os.path.join(bin_path, f)
    raise GdbMissingDebuggerError('Cannot find cros gdb for %s.'
                                  % self.board)

  def SimpleChromeSysroot(self):
    """Get the sysroot in simple chrome."""
    sysroot = self.board + '+' + os.environ['SDK_VERSION'] + \
              '+' + 'sysroot_chromeos-base_chromeos-chrome.tar.xz'
    sysroot = os.path.join(self.sdk_path, sysroot)
    if not os.path.isdir(sysroot):
      raise GdbMissingSysrootError('Cannot find sysroot for %s at.'
                                   ' %s' % self.board, sysroot)
    return sysroot

  def GetSimpleChromeBinary(self):
    """Get path to the  binary in simple chrome."""
    if self.binary:
      return self.binary

    output_dir = os.path.join(self.chrome_path, 'src',
                              'out_{}'.format(self.board))
    target_binary = None
    binary_name = os.path.basename(self.inf_cmd)
    for root, _, files in os.walk(output_dir):
      for f in files:
        if f == binary_name:
          if target_binary == None:
            target_binary = os.path.join(root, f)
          else:
            raise GdbSimpleChromeBinaryError(
                'There are multiple %s under %s. Please specify the path to '
                'the binary via --binary'% binary_name, output_dir)
    if target_binary == None:
      raise GdbSimpleChromeBinaryError('There is no %s under %s.'
                                       % binary_name, output_dir)
    return target_binary

  def VerifyAndFinishInitialization(self, device):
    """Verify files/processes exist and flags are correct."""
    if not self.board:
      if self.remote:
        self.board = cros_build_lib.GetBoard(device_board=device.board,
                                             override_board=self.board)
      else:
        raise GdbCannotDetectBoardError('Cannot determine which board to use. '
                                        'Please specify the with --board flag.')
    self.in_chroot = self.IsInChroot()
    self.prompt = '(%s-gdb) ' % self.board
    if self.in_chroot:
      self.sysroot = cros_build_lib.GetSysroot(board=self.board)
      self.inf_cmd = self.RemoveSysrootPrefix(self.inf_cmd)
      self.cross_gdb = self.GetCrossGdb()
    else:
      self.chrome_path = os.path.realpath(os.path.join(os.path.dirname(
          os.path.realpath(__file__)), "../../../.."))
      self.sdk_path = os.path.join(self.chrome_path,
                                   '.cros_cache/chrome-sdk/tarballs/')
      self.sysroot = self.SimpleChromeSysroot()
      self.cross_gdb = self.SimpleChromeGdb()

    if self.remote:

      # If given remote process name, find pid & inf_cmd on remote device.
      if self.remote_process_name or self.pid:
        self._FindRemoteProcess(device)

      # Verify that sysroot is valid (exists).
      if not os.path.isdir(self.sysroot):
        raise GdbMissingSysrootError('Sysroot does not exist: %s' %
                                     self.sysroot)

    self.device = device
    if not self.in_chroot:
      return

    sysroot_inf_cmd = ''
    if self.inf_cmd:
      sysroot_inf_cmd = os.path.join(self.sysroot,
                                     self.inf_cmd.lstrip('/'))

    # Verify that inf_cmd, if given, exists.
    if sysroot_inf_cmd and not os.path.exists(sysroot_inf_cmd):
      raise GdbMissingInferiorError('Cannot find file %s (in sysroot).' %
                                    sysroot_inf_cmd)

    # Check to see if inf_cmd is stripped, and if so, check to see if debug file
    # exists.  If not, tell user and give them the option of quitting & getting
    # the debug info.
    if sysroot_inf_cmd:
      stripped_info = cros_build_lib.RunCommand(['file', sysroot_inf_cmd],
                                                capture_output=True).output
      if not ' not stripped' in stripped_info:
        debug_file = os.path.join(self.sysroot, 'usr/lib/debug',
                                  self.inf_cmd.lstrip('/'))
        debug_file += '.debug'
        if not os.path.exists(debug_file):
          equery = 'equery-%s' % self.board
          package = cros_build_lib.RunCommand([equery, '-q', 'b',
                                               self.inf_cmd],
                                              capture_output=True).output
          logging.info(self._MISSING_DEBUG_INFO_MSG % {
              'board': self.board,
              'inf_cmd': self.inf_cmd,
              'package': package,
              'debug_file': debug_file})
          answer = cros_build_lib.BooleanPrompt()
          if not answer:
            raise GdbEarlyExitError('Exiting early, at user request.')

    # Set up qemu, if appropriate.
    qemu_arch = qemu.Qemu.DetectArch(self._GDB, self.sysroot)
    if qemu_arch is None:
      self.framework = 'ldso'
    else:
      self.framework = 'qemu'
      self.qemu = qemu.Qemu(self.sysroot, arch=qemu_arch)

    if self.remote:
      # Verify cgdb flag info.
      if self.cgdb:
        if osutils.Which('cgdb') is None:
          raise GdbMissingDebuggerError('Cannot find cgdb.  Please install '
                                        'cgdb first.')

  def RemoveSysrootPrefix(self, path):
    """Returns the given path with any sysroot prefix removed."""
    # If the sysroot is /, then the paths are already normalized.
    if self.sysroot != '/' and path.startswith(self.sysroot):
      path = path.replace(self.sysroot, '', 1)
    return path

  @staticmethod
  def GetNonRootAccount():
    """Return details about the non-root account we want to use.

    Returns:
      A tuple of (username, uid, gid, home).
    """
    return (
        os.environ.get('SUDO_USER', 'nobody'),
        int(os.environ.get('SUDO_UID', '65534')),
        int(os.environ.get('SUDO_GID', '65534')),
        # Should we find a better home?
        '/tmp/portage',
    )

  @staticmethod
  @contextlib.contextmanager
  def LockDb(db):
    """Lock an account database.

    We use the same algorithm as shadow/user.eclass.  This way we don't race
    and corrupt things in parallel.
    """
    lock = '%s.lock' % db
    _, tmplock = tempfile.mkstemp(prefix='%s.platform.' % lock)

    # First try forever to grab the lock.
    retry = lambda e: e.errno == errno.EEXIST
    # Retry quickly at first, but slow down over time.
    try:
      retry_util.GenericRetry(retry, 60, os.link, tmplock, lock, sleep=0.1)
    except Exception as e:
      raise Exception('Could not grab lock %s. %s' % (lock, e))

    # Yield while holding the lock, but try to clean it no matter what.
    try:
      os.unlink(tmplock)
      yield lock
    finally:
      os.unlink(lock)

  def SetupUser(self):
    """Propogate the user name<->id mapping from outside the chroot.

    Some unittests use getpwnam($USER), as does bash.  If the account
    is not registered in the sysroot, they get back errors.
    """
    MAGIC_GECOS = 'Added by your friendly platform test helper; do not modify'
    # This is kept in sync with what sdk_lib/make_chroot.sh generates.
    SDK_GECOS = 'ChromeOS Developer'

    user, uid, gid, home = self.GetNonRootAccount()
    if user == 'nobody':
      return

    passwd_db = os.path.join(self.sysroot, 'etc', 'passwd')
    with self.LockDb(passwd_db):
      data = osutils.ReadFile(passwd_db)
      accts = data.splitlines()
      for acct in accts:
        passwd = acct.split(':')
        if passwd[0] == user:
          # Did the sdk make this account?
          if passwd[4] == SDK_GECOS:
            # Don't modify it (see below) since we didn't create it.
            return

          # Did we make this account?
          if passwd[4] != MAGIC_GECOS:
            raise RuntimeError('your passwd db (%s) has unmanaged acct %s' %
                               (passwd_db, user))

          # Maybe we should see if it needs to be updated?  Like if they
          # changed UIDs?  But we don't really check that elsewhere ...
          return

      acct = '%(name)s:x:%(uid)s:%(gid)s:%(gecos)s:%(homedir)s:%(shell)s' % {
          'name': user,
          'uid': uid,
          'gid': gid,
          'gecos': MAGIC_GECOS,
          'homedir': home,
          'shell': '/bin/bash',
      }
      with open(passwd_db, 'a') as f:
        if data[-1] != '\n':
          f.write('\n')
        f.write('%s\n' % acct)

  def _FindRemoteProcess(self, device):
    """Find a named process (or a pid) running on a remote device."""
    if not self.remote_process_name and not self.pid:
      return

    if self.remote_process_name:
      # Look for a process with the specified name on the remote device; if
      # found, get its pid.
      pname = self.remote_process_name
      if pname == 'browser':
        all_chrome_pids = set(device.GetRunningPids(
            '/opt/google/chrome/chrome'))
        sandbox_pids = set(device.GetRunningPids(
            '/opt/google/chrome/chrome-sandbox'))
        non_main_chrome_pids = set(device.GetRunningPids('type='))
        pids = list(all_chrome_pids - sandbox_pids - non_main_chrome_pids)
      elif pname == 'renderer' or pname == 'gpu-process':
        pids = device.GetRunningPids('type=%s'% pname)
      else:
        pids = device.GetRunningPids(pname)

      if pids:
        if len(pids) == 1:
          self.pid = pids[0]
        else:
          raise GdbTooManyPidsError('Multiple pids found for %s process: %s. '
                                    'You must specify the correct pid.'
                                    % (pname, repr(pids)))
      else:
        raise GdbCannotFindRemoteProcessError('Cannot find pid for "%s" on %s' %
                                              (pname, self.remote))

    # Find full path for process, from pid (and verify pid).
    command = [
        'readlink',
        '-e', '/proc/%s/exe' % self.pid,
    ]
    try:
      res = device.RunCommand(command, capture_output=True)
      if res.returncode == 0:
        self.inf_cmd = res.output.rstrip('\n')
    except cros_build_lib.RunCommandError:
      raise GdbCannotFindRemoteProcessError('Unable to find name of process '
                                            'with pid %s on %s' %
                                            (self.pid, self.remote))

  def GetCrossGdb(self):
    """Find the appropriate cross-version of gdb for the board."""
    toolchains = toolchain.GetToolchainsForBoard(self.board)
    tc = toolchain.FilterToolchains(toolchains, 'default', True).keys()
    cross_gdb = tc[0] + '-gdb'
    if not osutils.Which(cross_gdb):
      raise GdbMissingDebuggerError('Cannot find %s; do you need to run '
                                    'setup_board?' % cross_gdb)
    return cross_gdb

  def GetGdbInitCommands(self, inferior_cmd, device=None):
    """Generate list of commands with which to initialize the gdb session."""
    gdb_init_commands = []

    if self.remote:
      sysroot_var = self.sysroot
    else:
      sysroot_var = '/'

    gdb_init_commands = [
        'set sysroot %s' % sysroot_var,
        'set prompt %s' % self.prompt,
    ]
    if self.in_chroot:
      gdb_init_commands += [
          'set solib-absolute-prefix %s' % sysroot_var,
          'set solib-search-path %s' % sysroot_var,
          'set debug-file-directory %s/usr/lib/debug' % sysroot_var,
      ]

    if device:
      ssh_cmd = device.GetAgent().GetSSHCommand(self.ssh_settings)

      ssh_cmd.extend(['--', 'gdbserver'])

      if self.pid:
        ssh_cmd.extend(['--attach', 'stdio', str(self.pid)])
        target_type = 'remote'
      elif inferior_cmd:
        ssh_cmd.extend(['-', inferior_cmd])
        ssh_cmd.extend(self.inf_args)
        target_type = 'remote'
      else:
        ssh_cmd.extend(['--multi', 'stdio'])
        target_type = 'extended-remote'

      ssh_cmd = ' '.join(map(pipes.quote, ssh_cmd))

      if self.in_chroot:
        if inferior_cmd:
          gdb_init_commands.append(
              'file %s' % os.path.join(sysroot_var,
                                       inferior_cmd.lstrip(os.sep)))
      else:
        gdb_init_commands.append('file %s' % self.GetSimpleChromeBinary())

      gdb_init_commands.append('target %s | %s' % (target_type, ssh_cmd))
    else:
      if inferior_cmd:
        gdb_init_commands.append('file %s ' % inferior_cmd)
        gdb_init_commands.append('set args %s' % ' '.join(self.inf_args))

    return gdb_init_commands

  def RunRemote(self):
    """Handle remote debugging, via gdbserver & cross debugger."""
    device = None
    try:
      device = remote_access.ChromiumOSDeviceHandler(
          self.remote,
          port=self.remote_port,
          connect_settings=self.ssh_settings,
          ping=self.ping).device
    except remote_access.DeviceNotPingableError:
      raise GdbBadRemoteDeviceError('Remote device %s is not responding to '
                                    'ping.' % self.remote)

    self.VerifyAndFinishInitialization(device)
    gdb_cmd = self.cross_gdb

    gdb_commands = self.GetGdbInitCommands(self.inf_cmd, device)
    gdb_args = ['--quiet'] + ['--eval-command=%s' % x for x in gdb_commands]
    gdb_args += self.gdb_args

    if self.cgdb:
      gdb_args = ['-d', gdb_cmd, '--'] + gdb_args
      gdb_cmd = 'cgdb'

    logging.debug('Running: %s', [gdb_cmd] + gdb_args)

    os.chdir(self.sysroot)
    sys.exit(os.execvp(gdb_cmd, gdb_args))

  def Run(self):
    """Runs the debugger in a proper environment (e.g. qemu)."""

    self.VerifyAndFinishInitialization(None)
    self.SetupUser()
    if self.framework == 'qemu':
      self.qemu.Install(self.sysroot)
      self.qemu.RegisterBinfmt()

    for mount in self._BIND_MOUNT_PATHS:
      path = os.path.join(self.sysroot, mount)
      osutils.SafeMakedirs(path)
      osutils.Mount('/' + mount, path, 'none', osutils.MS_BIND)

    gdb_cmd = self._GDB
    inferior_cmd = self.inf_cmd

    gdb_argv = self.gdb_args[:]
    if gdb_argv:
      gdb_argv[0] = self.RemoveSysrootPrefix(gdb_argv[0])
    # Some programs expect to find data files via $CWD, so doing a chroot
    # and dropping them into / would make them fail.
    cwd = self.RemoveSysrootPrefix(os.getcwd())

    os.chroot(self.sysroot)
    os.chdir(cwd)
    # The TERM the user is leveraging might not exist in the sysroot.
    # Force a sane default that supports standard color sequences.
    os.environ['TERM'] = 'ansi'
    # Some progs want this like bash else they get super confused.
    os.environ['PWD'] = cwd
    if not self.run_as_root:
      _, uid, gid, home = self.GetNonRootAccount()
      os.setgid(gid)
      os.setuid(uid)
      os.environ['HOME'] = home

    gdb_commands = self.GetGdbInitCommands(inferior_cmd)

    gdb_args = [gdb_cmd, '--quiet'] + ['--eval-command=%s' % x
                                       for x in gdb_commands]
    gdb_args += self.gdb_args

    sys.exit(os.execvp(gdb_cmd, gdb_args))


def _ReExecuteIfNeeded(argv, ns_net=False, ns_pid=False):
  """Re-execute gdb as root.

  We often need to do things as root, so make sure we're that.  Like chroot
  for proper library environment or do bind mounts.

  Also unshare the mount namespace so as to ensure that doing bind mounts for
  tests don't leak out to the normal chroot.  Also unshare the UTS namespace
  so changes to `hostname` do not impact the host.
  """
  if os.geteuid() != 0:
    cmd = ['sudo', '-E', '--'] + argv
    os.execvp(cmd[0], cmd)
  else:
    namespaces.SimpleUnshare(net=ns_net, pid=ns_pid)


def FindInferior(arg_list):
  """Look for the name of the inferior (to be debugged) in arg list."""

  program_name = ''
  new_list = []
  for item in arg_list:
    if item[0] == '-':
      new_list.append(item)
    elif not program_name:
      program_name = item
    else:
      raise RuntimeError('Found multiple program names: %s  %s'
                         % (program_name, item))

  return program_name, new_list


def main(argv):

  parser = commandline.ArgumentParser(description=__doc__)

  parser.add_argument('--board', default=None,
                      help='board to debug for')
  parser.add_argument('-g', '--gdb_args', action='append', default=[],
                      help='Arguments to gdb itself.  If multiple arguments are'
                      ' passed, each argument needs a separate \'-g\' flag.')
  parser.add_argument(
      '--remote', default=None,
      type=commandline.DeviceParser(commandline.DEVICE_SCHEME_SSH),
      help='Remote device on which to run the binary. Use'
      ' "--remote=localhost:9222" to debug in a ChromeOS image in an'
      ' already running local virtual machine.')
  parser.add_argument('--pid', default='',
                      help='Process ID of the (already) running process on the'
                      ' remote device to which to attach.')
  parser.add_argument('--remote_pid', dest='pid', default='',
                      help='Deprecated alias for --pid.')
  parser.add_argument('--no-ping', dest='ping', default=True,
                      action='store_false',
                      help='Do not ping remote before attempting to connect.')
  parser.add_argument('--attach', dest='attach_name', default='',
                      help='Name of existing process to which to attach, on'
                      ' remote device (remote debugging only). "--attach'
                      ' browser" will find the main chrome browser process;'
                      ' "--attach renderer" will find a chrome renderer'
                      ' process; "--attach gpu-process" will find the chrome'
                      ' gpu process.')
  parser.add_argument('--cgdb', default=False,
                      action='store_true',
                      help='Use cgdb curses interface rather than plain gdb.'
                      'This option is only valid for remote debugging.')
  parser.add_argument('inf_args', nargs=argparse.REMAINDER,
                      help='Arguments for gdb to pass to the program being'
                      ' debugged. These are positional and must come at the end'
                      ' of the command line.  This will not work if attaching'
                      ' to an already running program.')
  parser.add_argument('--binary', default='',
                      help='full path to the binary being debuged.'
                      ' This is only useful for simple chrome.'
                      ' An example is --bianry /home/out_falco/chrome.')

  options = parser.parse_args(argv)
  options.Freeze()

  gdb_args = []
  inf_args = []
  inf_cmd = ''

  if options.inf_args:
    inf_cmd = options.inf_args[0]
    inf_args = options.inf_args[1:]

  if options.gdb_args:
    gdb_args = options.gdb_args

  if inf_cmd:
    fname = os.path.join(cros_build_lib.GetSysroot(options.board),
                         inf_cmd.lstrip('/'))
    if not os.path.exists(fname):
      cros_build_lib.Die('Cannot find program %s.' % fname)
  else:
    if inf_args:
      parser.error('Cannot specify arguments without a program.')

  if inf_args and (options.pid or options.attach_name):
    parser.error('Cannot pass arguments to an already'
                 ' running process (--remote-pid or --attach).')

  if options.remote:
    if options.attach_name and options.attach_name == 'browser':
      inf_cmd = '/opt/google/chrome/chrome'
  else:
    if options.cgdb:
      parser.error('--cgdb option can only be used with remote debugging.')
    if options.pid:
      parser.error('Must specify a remote device (--remote) if you want '
                   'to attach to a remote pid.')
    if options.attach_name:
      parser.error('Must specify remote device (--remote) when using'
                   ' --attach option.')
  if options.binary:
    if not os.path.exists(options.binary):
      parser.error('%s does not exist.' % options.binary)

  # Once we've finished sanity checking args, make sure we're root.
  if not options.remote:
    _ReExecuteIfNeeded([sys.argv[0]] + argv)

  gdb = BoardSpecificGdb(options.board, gdb_args, inf_cmd, inf_args,
                         options.remote, options.pid, options.attach_name,
                         options.cgdb, options.ping, options.binary)

  try:
    if options.remote:
      gdb.RunRemote()
    else:
      gdb.Run()

  except GdbException as e:
    if options.debug:
      raise
    else:
      raise cros_build_lib.Die(str(e))
