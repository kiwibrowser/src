# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module to manage a setup of moblab and its DUT VMs."""

from __future__ import print_function

import collections
import contextlib
import json
import os
import random
import shutil

from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import osutils
from chromite.lib import path_util
from chromite.lib import retry_util


_CONFIG_FILE_NAME = 'moblabvm.json'

# Keys into the config dict persisted to disk
_CONFIG_INITIALIZED = 'initialized'
_CONFIG_STARTED = 'started'
_CONFIG_MOBLAB_IMAGE = 'image_path_moblab'
_CONFIG_DUT_IMAGE = 'image_path_dut'
_CONFIG_MOBLAB_DISK = 'disk_path_moblab'
# Some paths are stored as relative paths in the config so that we can freely
# move around the workspace on disk. They're always used as absolute paths once
# loaded, because absolute paths are easier to work with.
_CONFIG_RELATIVE_PATH_KEYS = (
    _CONFIG_MOBLAB_IMAGE,
    _CONFIG_DUT_IMAGE,
    _CONFIG_MOBLAB_DISK,
)

_CONFIG_MOBLAB_KVM_PID = 'kvm_pid_moblab'
_CONFIG_DUT_KVM_PID = 'kvm_pid_dut'

_CONFIG_MOBLAB_SSH_PORT = 'ssh_port_moblab'
_CONFIG_DUT_SSH_PORT = 'ssh_port_dut'
_CONFIG_NETWORK_BRIDGE = 'network_bridge'
_CONFIG_MOBLAB_TAP_DEV = 'tap_moblab'
_CONFIG_DUT_TAP_DEV = 'tap_dut'
_CONFIG_MOBLAB_TAP_MAC = 'mac_tap_moblab'
_CONFIG_DUT_TAP_MAC = 'mac_tap_dut'

# Paths within the workspace
_WORKSPACE_MOBLAB_DIR = 'moblab_image'
_WORKSPACE_DUT_DIR = 'dut_image'


class MoblabVmError(Exception):
  """Base exception for all ya folks who want to catch-all."""


class RetriableError(MoblabVmError):
  """Some error happened where the best option is perhaps to retry."""


class SetupError(MoblabVmError):
  """Some error happened while setting up the VMs."""


class StartError(MoblabVmError):
  """Some error happened while setting up the VMs."""


class MoblabVm(object):
  """Manage a setup of VMs required for a VM-only moblab setup.

  See cli/cros/cros_moblabvm.py for details about the setup.
  """

  def __init__(self, workspace_dir):
    """Initialize us.

    Args:
      workspace_dir: An existing directory to be used for to drop files
          necessary for the setup.
    """
    self.workspace = workspace_dir
    self._config_path = os.path.join(self.workspace, _CONFIG_FILE_NAME)
    self._config = self._LoadConfig()

  @property
  def initialized(self):
    """Is this MoblabVm initialized? Returns bool."""
    return bool(self._config[_CONFIG_INITIALIZED])

  @property
  def running(self):
    """Is this MoblabVm running? Returns bool."""
    return (bool(self._config[_CONFIG_STARTED]) and
            bool(self._config[_CONFIG_MOBLAB_KVM_PID]))

  @property
  def moblab_ssh_port(self):
    """The (str) SSH port to connect to the moblab VM."""
    return self._config[_CONFIG_MOBLAB_SSH_PORT]

  @property
  def moblab_internal_mac(self):
    """The MAC address used by moblab VM for the internal network.

    This is the network that moblab uses to connect to its DUTs.
    """
    return self._config[_CONFIG_MOBLAB_TAP_MAC]

  @property
  def dut_running(self):
    """Is the sub-DUT VM running? Returns bool.

    A moblabvm may be launched without any sub-DUTs.
    """
    return (bool(self._config[_CONFIG_STARTED]) and
            bool(self._config[_CONFIG_DUT_KVM_PID]))

  @property
  def dut_ssh_port(self):
    """The (str) SSH port to connect to the sub-DUT VM."""
    return self._config[_CONFIG_DUT_SSH_PORT]

  @property
  def dut_internal_mac(self):
    """The MAC address used by sub-DUT VM for the internal network.

    This is the network that moblab uses to connect to its DUTs.
    """
    return self._config[_CONFIG_DUT_TAP_MAC]

  def Create(self, moblab_image_dir, dut_image_dir='', create_vm_images=True):
    """Create a new moblabvm setup.

    Args:
      moblab_image_dir: A directory containing the image for moblab. This
          directory should ideally contain all the files output by build_image
          because they are needed to manipulate the image properly.
      dut_image_dir: A directory containing the image for the sub-DUT. Only
          required if a DUT should be attached to the moblab VM. This directory
          should ideally contain all the files output by build_image because
          they are needed to manipulate the image properly.
      create_vm_images: If True, the source directories contain test images that
          need to be converted to vm images. If False, the source directories
          should already contain VM images to be used directly.
    """
    if self._config[_CONFIG_INITIALIZED]:
      raise SetupError('Cannot overwrite existing setup at %s' %
                       self.workspace)

    logging.notice('Initializing workspace in %s', self.workspace)
    logging.notice('This involves creating some VM images. '
                   'May take a few minutes.')

    logging.notice('Preparing moblab image...')
    moblab_dir = os.path.join(self.workspace, _WORKSPACE_MOBLAB_DIR)
    osutils.SafeMakedirsNonRoot(moblab_dir)
    if create_vm_images:
      self._config[_CONFIG_MOBLAB_IMAGE] = _CreateVMImage(
          moblab_image_dir, moblab_dir)
    else:
      self._config[_CONFIG_MOBLAB_IMAGE] = self._CopyVMImage(
          moblab_image_dir, moblab_dir)

    logging.notice('Generating moblab external disk...')
    moblab_disk = _CreateMoblabDisk(moblab_dir)
    self._config[_CONFIG_MOBLAB_DISK] = moblab_disk

    if dut_image_dir:
      logging.notice('Preparing dut image...')
      dut_dir = os.path.join(self.workspace, _WORKSPACE_DUT_DIR)
      osutils.SafeMakedirsNonRoot(dut_dir)
      if create_vm_images:
        self._config[_CONFIG_DUT_IMAGE] = _CreateVMImage(
            dut_image_dir, dut_dir)
      else:
        self._config[_CONFIG_DUT_IMAGE] = self._CopyVMImage(
            dut_image_dir, dut_dir)

    self._config[_CONFIG_INITIALIZED] = 'true'
    self._Persist()
    logging.notice('All Done!')

  def Start(self):
    """Launch the VM(s) in this setup.

    This involves grabbing some global resources on the host for networking
    between the launched VMs.
    """
    if not self._config[_CONFIG_INITIALIZED]:
      raise StartError('No moblabvm initialized at %s' % self.workspace)

    if self._config[_CONFIG_STARTED]:
      raise StartError('Can not start (partially) started VMs. '
                       'Pelase call Stop first.')

    # Start grabs system global resources. Try hard to persist even partial
    # attempts so that Stop can cleanup everything we do setup correctly.
    try:
      self._Start()
      self._config[_CONFIG_STARTED] = 'true'
    finally:
      self._Persist()

  def Stop(self):
    """Stop the VM(s) in this setup.

    Also releases global network resources.
    """
    # These should strictly be in LIFO order of setup done in Start.
    # Moreover, it should be able to cleanup after Start fails after partial
    # setup.
    if self._config[_CONFIG_DUT_KVM_PID]:
      _StopKvmIgnoringErrors(self._config[_CONFIG_DUT_KVM_PID])
      del self._config[_CONFIG_DUT_KVM_PID]
    if self._config[_CONFIG_MOBLAB_KVM_PID]:
      _StopKvmIgnoringErrors(self._config[_CONFIG_MOBLAB_KVM_PID])
      del self._config[_CONFIG_MOBLAB_KVM_PID]
    if self._config[_CONFIG_DUT_TAP_DEV]:
      _RemoveTapDeviceIgnoringErrors(self._config[_CONFIG_DUT_TAP_DEV])
      del self._config[_CONFIG_DUT_TAP_DEV]
    if self._config[_CONFIG_MOBLAB_TAP_DEV]:
      _RemoveTapDeviceIgnoringErrors(self._config[_CONFIG_MOBLAB_TAP_DEV])
      del self._config[_CONFIG_MOBLAB_TAP_DEV]
    if self._config[_CONFIG_NETWORK_BRIDGE]:
      _RemoveNetworkBridgeIgnoringErrors(self._config[_CONFIG_NETWORK_BRIDGE])
      del self._config[_CONFIG_NETWORK_BRIDGE]

    if self._config[_CONFIG_STARTED]:
      del self._config[_CONFIG_STARTED]
    self._Persist()

  def Destroy(self):
    """Completely destroy this moblabvm setup. Cleans out workspace_dir."""
    self.Stop()
    osutils.EmptyDir(self.workspace, ignore_missing=True, sudo=True)
    self._config = collections.defaultdict(str)
    # Don't self._Persist since the workspace is now clean.

  @contextlib.contextmanager
  def MountedMoblabDiskContext(self):
    """A contextmanager to mount the already prepared moblab disk.

    This can be used to modify the external disk mounted by the moblab VM, while
    the VMs are not running. It is often much more performance to modify the
    disk on the host than SCP'ing large files into the VM.

    vms = moblab_vm.MoblabVm(workspace_dir)
    vms.Create(...)
    with vms.MountedMoblabDiskContext() as moblab_disk_dir:
      # Copy stuff into the directory at moblab_disk_dir
    vms.Start()
    ...
    """
    if not self.initialized:
      raise MoblabVmError('Uninitialized workspace %s. Can not mount disk.'
                          % self.workspace)
    if self.running:
      raise MoblabVmError(
          'VM at %s is already running. Stop() before mounting disk.'
          % self.workspace)

    with osutils.TempDir() as tempdir:
      osutils.MountDir(self._config[_CONFIG_MOBLAB_DISK], tempdir, 'ext4',
                       skip_mtab=True)
      try:
        yield tempdir
      finally:
        osutils.UmountDir(tempdir)

  def _LoadConfig(self):
    """Attempts to load the config from workspace, or sets up defaults."""
    config = collections.defaultdict(str)
    if os.path.isfile(self._config_path):
      with open(self._config_path, 'r') as config_file:
        config_data = json.load(config_file)
      self._ConfigRelativePathsToAbsolute(config_data,
                                          os.path.dirname(self._config_path))
      config.update(config_data)
    return config

  def _Persist(self):
    """Dump our config to disk for later MoblabVm objects."""
    # We do not want to modify config dict for the object.
    config = dict(self._config)
    self._ConfigAbsolutePathsToRelative(config,
                                        os.path.dirname(self._config_path))
    with open(self._config_path, 'w') as config_file:
      json.dump(config, config_file, sort_keys=True, indent=4,
                separators=(',', ': '))

  def _ConfigRelativePathsToAbsolute(self, config_data, workspace_dir):
    """Converts all the relative paths loaded from config to absolute paths."""
    for key, relpath in config_data.iteritems():
      if key not in _CONFIG_RELATIVE_PATH_KEYS:
        continue
      config_data[key] = os.path.realpath(os.path.join(workspace_dir, relpath))

  def _ConfigAbsolutePathsToRelative(self, config_data, workspace_dir):
    """Converts all absolute paths to relative paths for persisting on disk."""
    workspace_dir = os.path.realpath(workspace_dir)
    for key, abspath in config_data.iteritems():
      if key not in _CONFIG_RELATIVE_PATH_KEYS:
        continue
      abspath = os.path.realpath(abspath)
      config_data[key] = os.path.relpath(abspath, workspace_dir)

  def _Start(self):
    """Actually start the VM(s), without persisting the updated config."""
    # Mulitple moblabvms should be able to run in parallel. At the same time, we
    # want to minimize flaky failures due to unavailable system global
    # resources. We attempt to create a bridge device a few times. Once we get a
    # hold of that, we base all other system global constants off of that to not
    # step our other moblabvms' toes.
    for _ in xrange(5):
      try:
        next_num, bridge_name = _TryCreateBridgeDevice()
        break
      except RetriableError:
        pass
    else:
      raise SetupError('Failed to create bridge device. '
                       'You seem to have many moblabvms running in parallel.')

    self._config[_CONFIG_NETWORK_BRIDGE] = bridge_name
    self._config[_CONFIG_MOBLAB_TAP_DEV] = _CreateTapDevice(next_num)
    next_num += 1
    self._config[_CONFIG_DUT_TAP_DEV] = _CreateTapDevice(next_num)
    next_num += 1
    _ConnectDeviceToBridge(self._config[_CONFIG_MOBLAB_TAP_DEV],
                           self._config[_CONFIG_NETWORK_BRIDGE])
    _ConnectDeviceToBridge(self._config[_CONFIG_DUT_TAP_DEV],
                           self._config[_CONFIG_NETWORK_BRIDGE])
    _DeviceUp(self._config[_CONFIG_NETWORK_BRIDGE])

    next_num = self._StartMoblabVm(next_num)
    if self._config[_CONFIG_DUT_IMAGE]:
      next_num = self._StartDutVm(next_num)

  def _CopyVMImage(self, source_dir, target_dir):
    """Converts or copies VM images from source_dir to target_dir."""
    source_path = os.path.join(source_dir, constants.VM_IMAGE_BIN)
    if not os.path.isfile(source_path):
      raise SetupError('Could not find VM image at %s' % source_path)

    target_path = os.path.join(target_dir, constants.VM_IMAGE_BIN)
    shutil.copyfile(source_path, target_path)
    return target_path

  def _StartMoblabVm(self, next_num):
    """Starts a VM running moblab.

    Args:
      next_num: A counter used to generate names without conflict.

    Returns:
      updated next_num.
    """
    ssh_port = str(next_num)
    next_num += 1
    # We want a unicast, locally configured address with an obviously bogus
    # organisation id.
    tap_mac_addr = '02:00:00:99:99:01'
    self._WriteToMoblabDiskImage('private-network-macaddr.conf', [tap_mac_addr])
    disk_path = self._config[_CONFIG_MOBLAB_DISK]
    # Create a dedicated scsi controller for the external disk, and attach the
    # disk to that bus. This separates us from any scsi controllers that may be
    # created for the boot disk.
    qemu_args = ['-drive', 'id=moblabdisk,if=none,file=%s' % disk_path,
                 '-device', 'virtio-scsi-pci,id=scsiext',
                 '-device', 'scsi-hd,bus=scsiext.0,drive=moblabdisk']
    # moblab grabs some extra consecutive ports for forwarding AFE and devserver
    # to the host.
    next_num += 10
    kvm_pid = _StartKvm(
        os.path.join(self.workspace, _WORKSPACE_MOBLAB_DIR),
        self._config[_CONFIG_MOBLAB_IMAGE],
        ssh_port,
        self._config[_CONFIG_MOBLAB_TAP_DEV],
        tap_mac_addr,
        is_moblab=True,
        qemu_args=qemu_args,
    )
    # Update config _after_ we've successfully launched the VM so that we don't
    # _Persist incorrect information.
    self._config[_CONFIG_MOBLAB_KVM_PID] = kvm_pid
    self._config[_CONFIG_MOBLAB_SSH_PORT] = ssh_port
    self._config[_CONFIG_MOBLAB_TAP_MAC] = tap_mac_addr
    return next_num

  def _StartDutVm(self, next_num):
    """Starts a VM running the sub-DUT image.

    Args:
      next_num: A counter used to generate names without conflict.

    Returns:
      updated next_num.
    """
    ssh_port = str(next_num)
    next_num += 1
    # We want a unicast, locally configured address with an obviously bogus
    # organisation id.
    tap_mac_addr = '02:00:00:99:99:51'
    kvm_pid = _StartKvm(
        os.path.join(self.workspace, _WORKSPACE_DUT_DIR),
        self._config[_CONFIG_DUT_IMAGE],
        ssh_port,
        self._config[_CONFIG_DUT_TAP_DEV],
        tap_mac_addr,
        is_moblab=False,
    )
    # Update config _after_ we've successfully launched the VM so that we don't
    # _Persist incorrect information.
    self._config[_CONFIG_DUT_KVM_PID] = kvm_pid
    self._config[_CONFIG_DUT_SSH_PORT] = ssh_port
    self._config[_CONFIG_DUT_TAP_MAC] = tap_mac_addr
    return next_num

  def _WriteToMoblabDiskImage(self, target_path, lines):
    """Write a file in the moblab external disk.

    Args:
      target_path: Path within the mounted disk to write. This path will be
          overwritten.
      lines: An iterator of lines to write.
    """
    with self.MountedMoblabDiskContext() as disk_dir:
      osutils.WriteFile(os.path.join(disk_dir, target_path), lines, sudo=True)


def _CreateVMImage(src_dir, dest_dir):
  """Creates a VM image from a given chromiumos image.

  Args:
    src_dir: Path to the directory containing (non-VM) image. Defaults to None
        to use the latest image for the board.
    dest_dir: Path to the directory where the VM image should be written.

  Returns:
    The path of the created VM image.
  """
  # image_to_vm.sh only runs in chroot, but src_dir / dest_dir may not be
  # reachable from chroot. Also, image_to_vm.sh needs all the contents of
  # src_dir to work correctly (it silently does the wrong thing if some files
  # are missing).
  # So, create a tempdir reachable from chroot, copy everything to that path,
  # create vm image there and finally move it all to dest_dir.
  with _TempDirInChroot() as tempdir:
    logging.debug('Copying images from %s to %s.', src_dir, tempdir)
    osutils.CopyDirContents(src_dir, tempdir)
    # image_to_vm.sh doesn't let us provide arbitrary names for the input image.
    # Instead, it picks the name based on whether we pass in --test_image or not
    # (and doesn't use that flag for anything else).
    cmd = [
        path_util.ToChrootPath(
            os.path.join(constants.CROSUTILS_DIR, 'image_to_vm.sh')),
        '--from=%s' % path_util.ToChrootPath(tempdir),
        '--test_image',
    ]
    try:
      cros_build_lib.RunCommand(cmd, enter_chroot=True,
                                cwd=constants.SOURCE_ROOT)
    except cros_build_lib.RunCommandError as e:
      raise SetupError('Failed to create VM image for %s: %s' % (src_dir, e))

    # Preserve most content, although we should need only the generated VM
    # image. Other files like boot.desc might be needed elsewhere, but the
    # source images should no longer be needed.
    osutils.SafeUnlink(os.path.join(tempdir, constants.BASE_IMAGE_BIN),
                       sudo=True)
    osutils.SafeUnlink(os.path.join(tempdir, constants.TEST_IMAGE_BIN),
                       sudo=True)
    osutils.CopyDirContents(tempdir, dest_dir)
  # The exact name of the output image is hard-coded in image_to_vm.sh
  return os.path.join(dest_dir, constants.VM_IMAGE_BIN)


def _CreateMoblabDisk(dest_dir):
  """Creates an empty moblab virtual disk at the given path.

  Args:
    dest_dir: Directory where the disk image should be created.

  Returns:
    Path to the created disk image.
  """
  dest_path = os.path.join(dest_dir, 'moblab_disk')
  cros_build_lib.RunCommand(
      ['qemu-img', 'create', '-f', 'raw', dest_path, '35g'])
  cros_build_lib.RunCommand(['mkfs.ext4', '-F', dest_path])
  cros_build_lib.RunCommand(['e2label', dest_path, 'MOBLAB-STORAGE'])
  return dest_path


def _TryCreateBridgeDevice():
  """Creates a bridge device named moblabvmbrXX, generating XX randomly.

  Returns:
    num, name: Counter to be used to generate unique names, name of the bridge.
  """
  # Keep this number between 1024-49151 (with padding) because:
  # - We use it to reserve networking ports, so stay within user range.
  # - We use it to name kernel devices, which has length restrictions.
  num = random.randint(10000, 30000)
  # device names can only be 16 char long. So suffix can be between 0 - 10^9.
  name = 'brmob%s' % num
  cros_build_lib.SudoRunCommand(['ip', 'link', 'add', name, 'type', 'bridge'],
                                quiet=True)
  return num + 1, name


def _CreateTapDevice(suffix):
  """Create a tap device using the suffix provided."""
  # device names can only be 16 char long. So suffix can be between 0 - 10^10.
  name = 'tapmob%s' % suffix
  cros_build_lib.SudoRunCommand(['ip', 'tuntap', 'add', 'mode', 'tap', name])

  # Creating the device can take some time, and trying to create another device
  # in the meantime returns 'Device or resource busy'. So, wait for device
  # creation to complete.
  @retry_util.WithRetry(max_retry=3, sleep=0.2, exception=RetriableError)
  def _VerifyDeviceCreated(name):
    result = cros_build_lib.RunCommand(['ip', 'tuntap', 'show'],
                                       redirect_stdout=True)
    if name not in result.output:
      raise RetriableError('Device %s not found in output "%s".' %
                           (name, result.output))

  _VerifyDeviceCreated(name)
  return name


def _ConnectDeviceToBridge(device, bridge):
  """Connects link device to network bridge."""
  cros_build_lib.SudoRunCommand(['ip', 'link', 'set', device, 'master', bridge])


def _DeviceUp(device):
  """Bring up the given link device."""
  cros_build_lib.SudoRunCommand(['ip', 'link', 'set', 'dev', device, 'up'])


def _StartKvm(workdir, image_path, ssh_port, tap_dev, tap_mac_addr, is_moblab,
              qemu_args=()):
  """Starts a KVM instance.

  Args:
    workdir: A directory to drop temporary files in.
    image_path: Path to the OS image to use.
    ssh_port: (str) port to use for SSH.
    tap_dev: Name of the tap device to use for secondary network.
    tap_mac_addr: The MAC address of the secondary network device.
    is_moblab: Whether we're starting a moblab VM.
    qemu_args: A list of args to be passed through to qemu.

  Returns:
    The path to the pidfile for launch KVM instance.
  """
  kvm_pid = os.path.join(workdir, 'kvm_pid')
  cmd = [
      os.path.join(constants.CROSUTILS_DIR, 'bin', 'cros_start_vm'),
      '--image_path', image_path,
      '--kvm_pid', kvm_pid,
      '--ssh_port', ssh_port,
      '--scsi',
  ]
  if is_moblab:
    cmd.append('--moblab')

  extra_args = list(qemu_args)
  extra_args += [
      '-net', 'nic,macaddr=%s' % tap_mac_addr,
      '-net', 'tap,ifname=%s' % tap_dev,
  ]
  cmd = cmd + ['--'] + extra_args
  cros_build_lib.RunCommand(cmd)
  return kvm_pid


def _RemoveNetworkBridgeIgnoringErrors(name):
  """Removes a previously created network bridge. Ignores errors."""
  _RunIgnoringErrors(['ip', 'link', 'del', name, 'type', 'bridge'])


def _RemoveTapDeviceIgnoringErrors(name):
  """Removes a previously created tap device. Ignores errors."""
  _RunIgnoringErrors(['ip', 'tuntap', 'del', 'mode', 'tap', name])


def _StopKvmIgnoringErrors(kvm_pid):
  """Stops a running KVM instance. Ignores errors."""
  logging.notice('Stopping KVM. This may take a minute.')
  _RunIgnoringErrors([
      os.path.join(constants.CROSUTILS_DIR, 'bin', 'cros_stop_vm'),
      '--kvm_pid', kvm_pid,
  ])


def _RunIgnoringErrors(cmd):
  """Runs the given command, ignores errors but warns user."""
  try:
    cros_build_lib.SudoRunCommand(cmd, quiet=True)
  except cros_build_lib.RunCommandError as e:
    logging.error('Cleanup operation failed. Please run "%s" manually.',
                  ' '.join(cmd))
    logging.debug('Encountered error: %s', e)


@contextlib.contextmanager
def _TempDirInChroot():
  """A context to create and use a tempdir inside the chroot.

  Yields:
    A host path (not chroot path) to a tempdir inside the chroot. This tempdir
    is cleaned up when exiting the context.
  """
  chroot_tempdir = cros_build_lib.RunCommand(
      ['mktemp', '-d'],
      capture_output=True,
      enter_chroot=True,
  ).output.strip()
  tempdir = path_util.FromChrootPath(chroot_tempdir)
  try:
    yield tempdir
  finally:
    osutils.RmDir(tempdir, ignore_missing=True)
