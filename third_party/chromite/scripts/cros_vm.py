# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script for VM Management."""

from __future__ import print_function

import argparse
import distutils.version
import multiprocessing
import os
import re

from chromite.cli.cros import cros_chrome_sdk
from chromite.lib import cache
from chromite.lib import commandline
from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import osutils
from chromite.lib import path_util
from chromite.lib import remote_access
from chromite.lib import retry_util


class VMError(Exception):
  """Exception for VM failures."""

  def __init__(self, message):
    super(VMError, self).__init__()
    logging.error(message)


class VM(object):
  """Class for managing a VM."""

  SSH_PORT = 9222
  IMAGE_FORMAT = 'raw'

  def __init__(self, opts):
    """Initialize VM.

    Args:
      opts: command line options.
    """
    self.qemu_path = opts.qemu_path
    self.qemu_bios_path = opts.qemu_bios_path
    self.qemu_m = opts.qemu_m
    self.qemu_cpu = opts.qemu_cpu
    self.qemu_smp = opts.qemu_smp
    if self.qemu_smp == 0:
      self.qemu_smp = min(8, multiprocessing.cpu_count)
    self.enable_kvm = opts.enable_kvm
    # We don't need sudo access for software emulation or if /dev/kvm is
    # writeable.
    self.use_sudo = self.enable_kvm and not os.access('/dev/kvm', os.W_OK)
    self.display = opts.display
    self.image_path = opts.image_path
    self.image_format = opts.image_format
    self.board = opts.board
    self.ssh_port = opts.ssh_port
    self.dry_run = opts.dry_run

    self.start = opts.start
    self.stop = opts.stop
    self.cmd = opts.args[1:] if opts.cmd else None

    self.cache_dir = os.path.abspath(opts.cache_dir)
    assert os.path.isdir(self.cache_dir), "Cache directory doesn't exist"

    self.vm_dir = opts.vm_dir
    if not self.vm_dir:
      self.vm_dir = os.path.join(osutils.GetGlobalTempDir(),
                                 'cros_vm_%d' % self.ssh_port)
    self._CreateVMDir()

    self.pidfile = os.path.join(self.vm_dir, 'kvm.pid')
    self.kvm_monitor = os.path.join(self.vm_dir, 'kvm.monitor')
    self.kvm_pipe_in = '%s.in' % self.kvm_monitor  # to KVM
    self.kvm_pipe_out = '%s.out' % self.kvm_monitor  # from KVM
    self.kvm_serial = '%s.serial' % self.kvm_monitor

    self.remote = remote_access.RemoteDevice(remote_access.LOCALHOST,
                                             port=self.ssh_port)
    self.device_addr = 'ssh://%s:%d' % (remote_access.LOCALHOST, self.ssh_port)

    # TODO(achuith): support nographics, snapshot, mem_path, usb_passthrough,
    # moblab, etc.

  def _RunCommand(self, *args, **kwargs):
    """Use SudoRunCommand or RunCommand as necessary."""
    if self.use_sudo:
      return cros_build_lib.SudoRunCommand(*args, **kwargs)
    else:
      return cros_build_lib.RunCommand(*args, **kwargs)

  def _CreateVMDir(self):
    """Safely create vm_dir."""
    if not osutils.SafeMakedirs(self.vm_dir, sudo=self.use_sudo):
      # For security, ensure that vm_dir is not a symlink, and is owned by us or
      # by root.
      error_str = ('VM state dir is misconfigured; please recreate: %s'
                   % self.vm_dir)
      assert os.path.isdir(self.vm_dir), error_str
      assert not os.path.islink(self.vm_dir), error_str
      st_uid = os.stat(self.vm_dir).st_uid
      assert st_uid == 0 or st_uid == os.getuid(), error_str

  def _RmVMDir(self):
    """Cleanup vm_dir."""
    osutils.RmDir(self.vm_dir, ignore_missing=True, sudo=self.use_sudo)

  def _GetCachePath(self, cache_name):
    """Return path to cache.

    Args:
      cache_name: Name of cache.

    Returns:
      File path of cache.
    """
    return os.path.join(self.cache_dir,
                        cros_chrome_sdk.COMMAND_NAME,
                        cache_name)

  @cros_build_lib.MemoizedSingleCall
  def _SDKVersion(self):
    """Determine SDK version.

    Check the environment if we're in the SDK shell, and failing that, look at
    the misc cache.

    Returns:
      SDK version.
    """
    sdk_version = os.environ.get(cros_chrome_sdk.SDKFetcher.SDK_VERSION_ENV)
    if not sdk_version and self.board:
      misc_cache = cache.DiskCache(self._GetCachePath(
          cros_chrome_sdk.SDKFetcher.MISC_CACHE))
      with misc_cache.Lookup((self.board, 'latest')) as ref:
        if ref.Exists(lock=True):
          sdk_version = osutils.ReadFile(ref.path).strip()
    return sdk_version

  def _CachePathForKey(self, key):
    """Get cache path for key.

    Args:
      key: cache key.
    """
    tarball_cache = cache.TarballCache(self._GetCachePath(
        cros_chrome_sdk.SDKFetcher.TARBALL_CACHE))
    if self.board and self._SDKVersion():
      cache_key = (self.board, self._SDKVersion(), key)
      with tarball_cache.Lookup(cache_key) as ref:
        if ref.Exists():
          return ref.path
    return None

  @cros_build_lib.MemoizedSingleCall
  def QemuVersion(self):
    """Determine QEMU version.

    Returns:
      QEMU version.
    """
    version_str = self._RunCommand([self.qemu_path, '--version'],
                                   capture_output=True).output
    # version string looks like one of these:
    # QEMU emulator version 2.0.0 (Debian 2.0.0+dfsg-2ubuntu1.36), Copyright (c)
    # 2003-2008 Fabrice Bellard
    #
    # QEMU emulator version 2.6.0, Copyright (c) 2003-2008 Fabrice Bellard
    #
    # qemu-x86_64 version 2.10.1
    # Copyright (c) 2003-2017 Fabrice Bellard and the QEMU Project developers
    m = re.search(r"version ([0-9.]+)", version_str)
    if not m:
      raise VMError('Unable to determine QEMU version from:\n%s.' % version_str)
    return m.group(1)

  def _CheckQemuMinVersion(self):
    """Ensure minimum QEMU version."""
    min_qemu_version = '2.6.0'
    logging.info('QEMU version %s', self.QemuVersion())
    LooseVersion = distutils.version.LooseVersion
    if LooseVersion(self.QemuVersion()) < LooseVersion(min_qemu_version):
      raise VMError('QEMU %s is the minimum supported version. You have %s.'
                    % (min_qemu_version, self.QemuVersion()))

  def _SetQemuPath(self):
    """Find a suitable Qemu executable."""
    qemu_exe = 'qemu-system-x86_64'
    qemu_exe_path = os.path.join('usr/bin', qemu_exe)

    # Check SDK cache.
    if not self.qemu_path:
      qemu_dir = self._CachePathForKey(cros_chrome_sdk.SDKFetcher.QEMU_BIN_KEY)
      if qemu_dir:
        qemu_path = os.path.join(qemu_dir, qemu_exe_path)
        if os.path.isfile(qemu_path):
          self.qemu_path = qemu_path

    # Check chroot.
    if not self.qemu_path:
      qemu_path = os.path.join(
          constants.SOURCE_ROOT, constants.DEFAULT_CHROOT_DIR, qemu_exe_path)
      if os.path.isfile(qemu_path):
        self.qemu_path = qemu_path

    # Check system.
    if not self.qemu_path:
      self.qemu_path = osutils.Which(qemu_exe)

    if not self.qemu_path or not os.path.isfile(self.qemu_path):
      raise VMError('QEMU not found.')
    logging.debug('QEMU path: %s', self.qemu_path)
    self._CheckQemuMinVersion()

  def _GetBuiltVMImagePath(self):
    """Get path of a locally built VM image."""
    vm_image_path = os.path.join(constants.SOURCE_ROOT, 'src/build/images',
                                 cros_build_lib.GetBoard(self.board),
                                 'latest', constants.VM_IMAGE_BIN)
    return vm_image_path if os.path.isfile(vm_image_path) else None

  def _GetCacheVMImagePath(self):
    """Get path of a cached VM image."""
    cache_path = self._CachePathForKey(constants.VM_IMAGE_TAR)
    if cache_path:
      vm_image = os.path.join(cache_path, constants.VM_IMAGE_BIN)
      if os.path.isfile(vm_image):
        return vm_image
    return None

  def _SetVMImagePath(self):
    """Detect VM image path in SDK and chroot."""
    if not self.image_path:
      self.image_path = (self._GetCacheVMImagePath() or
                         self._GetBuiltVMImagePath())
    if not self.image_path:
      raise VMError('No VM image found. Use cros chrome-sdk --download-vm.')
    if not os.path.isfile(self.image_path):
      raise VMError('VM image does not exist: %s' % self.image_path)
    logging.debug('VM image path: %s', self.image_path)

  def Run(self):
    """Performs an action, one of start, stop, or run a command in the VM.

    Returns:
      cmd output.
    """

    if not self.start and not self.stop and not self.cmd:
      raise VMError('Must specify one of start, stop, or cmd.')
    if self.start:
      self.Start()
    if self.cmd:
      return self.RemoteCommand(self.cmd)
    if self.stop:
      self.Stop()

  def Start(self):
    """Start the VM."""

    self.Stop()

    logging.debug('Start VM')
    self._SetQemuPath()
    self._SetVMImagePath()

    self._RmVMDir()
    self._CreateVMDir()
    # Make sure we can read these files later on by creating them as ourselves.
    osutils.Touch(self.kvm_serial)
    for pipe in [self.kvm_pipe_in, self.kvm_pipe_out]:
      os.mkfifo(pipe, 0600)
    osutils.Touch(self.pidfile)

    qemu_args = [self.qemu_path]
    if self.qemu_bios_path:
      if not os.path.isdir(self.qemu_bios_path):
        raise VMError('Invalid QEMU bios path: %s' % self.qemu_bios_path)
      qemu_args += ['-L', self.qemu_bios_path]

    qemu_args += [
        '-m', self.qemu_m, '-smp', str(self.qemu_smp), '-vga', 'virtio',
        '-daemonize', '-usbdevice', 'tablet',
        '-pidfile', self.pidfile,
        '-chardev', 'pipe,id=control_pipe,path=%s' % self.kvm_monitor,
        '-serial', 'file:%s' % self.kvm_serial,
        '-mon', 'chardev=control_pipe',
        # Append 'check' to warn if the requested CPU is not fully supported.
        '-cpu', self.qemu_cpu + ',check',
        '-device', 'virtio-net,netdev=eth0',
        '-netdev', 'user,id=eth0,net=10.0.2.0/27,hostfwd=tcp:127.0.0.1:%d-:22'
        % self.ssh_port,
        '-drive', 'file=%s,index=0,media=disk,cache=unsafe,format=%s'
        % (self.image_path, self.image_format),
    ]
    if self.enable_kvm:
      qemu_args.append('-enable-kvm')
    if not self.display:
      qemu_args.extend(['-display', 'none'])
    logging.info('Pid file: %s', self.pidfile)
    if not self.dry_run:
      self._RunCommand(qemu_args)
    else:
      logging.info(cros_build_lib.CmdToStr(qemu_args))

  def _GetVMPid(self):
    """Get the pid of the VM.

    Returns:
      pid of the VM.
    """
    if not os.path.exists(self.vm_dir):
      logging.debug('%s not present.', self.vm_dir)
      return 0

    if not os.path.exists(self.pidfile):
      logging.info('%s does not exist.', self.pidfile)
      return 0

    pid = osutils.ReadFile(self.pidfile).rstrip()
    if not pid.isdigit():
      # Ignore blank/empty files.
      if pid:
        logging.error('%s in %s is not a pid.', pid, self.pidfile)
      return 0

    return int(pid)

  def IsRunning(self):
    """Returns True if there's a running VM.

    Returns:
      True if there's a running VM.
    """
    pid = self._GetVMPid()
    if not pid:
      return False

    # Make sure the process actually exists.
    return os.path.isdir('/proc/%i' % pid)

  def Stop(self):
    """Stop the VM."""
    logging.debug('Stop VM')

    pid = self._GetVMPid()
    if pid:
      logging.info('Killing %d.', pid)
      if not self.dry_run:
        self._RunCommand(['kill', '-9', str(pid)], error_code_ok=True)
    self._RmVMDir()

  def _WaitForProcs(self):
    """Wait for expected processes to launch."""
    class _TooFewPidsException(Exception):
      """Exception for _GetRunningPids to throw."""

    def _GetRunningPids(exe, numpids):
      pids = self.remote.GetRunningPids(exe, full_path=False)
      logging.info('%s pids: %s', exe, repr(pids))
      if len(pids) < numpids:
        raise _TooFewPidsException()

    def _WaitForProc(exe, numpids):
      try:
        retry_util.RetryException(
            exception=_TooFewPidsException,
            max_retry=20,
            functor=lambda: _GetRunningPids(exe, numpids),
            sleep=2)
      except _TooFewPidsException:
        raise VMError('_WaitForProcs failed: timed out while waiting for '
                      '%d %s processes to start.' % (numpids, exe))

    # We could also wait for session_manager, nacl_helper, etc, but chrome is
    # the long pole. We expect the parent, 2 zygotes, gpu-process, renderer.
    # This could potentially break with Mustash.
    _WaitForProc('chrome', 5)

  def WaitForBoot(self):
    """Wait for the VM to boot up.

    Wait for ssh connection to become active, and wait for all expected chrome
    processes to be launched.
    """
    if not os.path.exists(self.vm_dir):
      self.Start()

    try:
      result = retry_util.RetryException(
          exception=remote_access.SSHConnectionError,
          max_retry=10,
          functor=lambda: self.RemoteCommand(cmd=['echo']),
          sleep=5)
    except remote_access.SSHConnectionError:
      raise VMError('WaitForBoot timed out trying to connect to VM.')

    if result.returncode != 0:
      raise VMError('WaitForBoot failed: %s.' % result.error)

    # Chrome can take a while to start with software emulation.
    if not self.enable_kvm:
      self._WaitForProcs()

  def RemoteCommand(self, cmd, **kwargs):
    """Run a remote command in the VM.

    Args:
      cmd: command to run.
      kwargs: additional args (see documentation for RemoteDevice.RunCommand).
    """
    if not self.dry_run:
      return self.remote.RunCommand(cmd, debug_level=logging.INFO,
                                    combine_stdout_stderr=True,
                                    log_output=True,
                                    error_code_ok=True,
                                    **kwargs)

  @staticmethod
  def GetParser():
    """Parse a list of args.

    Args:
      argv: list of command line arguments.

    Returns:
      List of parsed opts.
    """
    parser = commandline.ArgumentParser(description=__doc__)
    parser.add_argument('--start', action='store_true', default=False,
                        help='Start the VM.')
    parser.add_argument('--stop', action='store_true', default=False,
                        help='Stop the VM.')
    parser.add_argument('--image-path', type=str,
                        help='Path to VM image to launch with --start.')
    parser.add_argument('--image-format', default=VM.IMAGE_FORMAT,
                        help='Format of the VM image (raw, qcow2, ...).')
    parser.add_argument('--qemu-path', type=str,
                        help='Path of qemu binary to launch with --start.')
    parser.add_argument('--qemu-m', type=str, default='8G',
                        help='Memory argument that will be passed to qemu.')
    parser.add_argument('--qemu-smp', type=int, default='0',
                        help='SMP argument that will be passed to qemu. (0 '
                             'means auto-detection.)')
    # TODO(pwang): replace SandyBridge to Haswell-noTSX once lab machine
    # running VMTest all migrate to GCE.
    parser.add_argument('--qemu-cpu', type=str,
                        default='SandyBridge,-invpcid,-tsc-deadline',
                        help='CPU argument that will be passed to qemu.')
    parser.add_argument('--qemu-bios-path', type=str,
                        help='Path of directory with qemu bios files.')
    parser.add_argument('--disable-kvm', dest='enable_kvm',
                        action='store_false', default=True,
                        help='Disable KVM, use software emulation.')
    parser.add_argument('--no-display', dest='display',
                        action='store_false', default=True,
                        help='Do not display video output.')
    parser.add_argument('--ssh-port', type=int, default=VM.SSH_PORT,
                        help='ssh port to communicate with VM.')
    sdk_board_env = os.environ.get(cros_chrome_sdk.SDKFetcher.SDK_BOARD_ENV)
    parser.add_argument('--board', default=sdk_board_env, help='Board to use.')
    parser.add_argument('--cache-dir', type=str,
                        default=path_util.GetCacheDir(),
                        help='Cache directory to use.')
    parser.add_argument('--vm-dir', type=str,
                        help='Temp VM directory to use.')
    parser.add_argument('--dry-run', action='store_true', default=False,
                        help='dry run for debugging.')
    parser.add_argument('--cmd', action='store_true', default=False,
                        help='Run a command in the VM.')
    parser.add_argument('args', nargs=argparse.REMAINDER,
                        help='Command to run in the VM.')
    return parser

def main(argv):
  opts = VM.GetParser().parse_args(argv)
  opts.Freeze()

  vm = VM(opts)
  vm.Run()
