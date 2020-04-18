# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utilities for setting up and cleaning up the chroot environment."""

from __future__ import print_function

import os
import re
import time

from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import osutils


# Name of the LV that contains the active chroot inside the chroot.img file.
CHROOT_LV_NAME = 'chroot'


# Name of the thin pool used for the chroot and snapshots inside chroot.img.
CHROOT_THINPOOL_NAME = 'thinpool'


# Max times to recheck the result of an lvm command that doesn't finish quickly.
_MAX_LVM_RETRIES = 3


def GetChrootVersion(chroot=None, buildroot=None):
  """Extract the version of the chroot.

  Args:
    chroot: Full path to the chroot to examine.
    buildroot: If |chroot| is not set, find it relative to |buildroot|.

  Returns:
    The version of the chroot dir.
  """
  if chroot is None and buildroot is None:
    raise ValueError('need either |chroot| or |buildroot| to search')

  if chroot is None:
    chroot = os.path.join(buildroot, constants.DEFAULT_CHROOT_DIR)
  ver_path = os.path.join(chroot, 'etc', 'cros_chroot_version')
  try:
    return osutils.ReadFile(ver_path).strip()
  except IOError:
    logging.warning('could not read %s', ver_path)
    return None


def FindVolumeGroupForDevice(chroot_path, chroot_dev):
  """Find a usable VG name for a given path and device.

  If there is an existing VG associated with the device, it will be returned
  even if the path doesn't match.  If not, find an unused name in the format
  cros_<safe_path>_NNN, where safe_path is an escaped version of the last 90
  characters of the path and NNN is a counter.  Example:
    /home/user/cros/chroot/ -> cros_home+user+cros+chroot_000.
  If no unused name with this pattern can be found, return None.

  A VG with the returned name will not necessarily exist.  The caller should
  call vgs or otherwise check the name before attempting to use it.

  Args:
    chroot_path: Path where the chroot will be mounted.
    chroot_dev: Device that should hold the VG, e.g. /dev/loop0.

  Returns:
    A VG name that can be used for the chroot/device pair, or None if no name
    can be found.
  """

  safe_path = re.sub(r'[^A-Za-z0-9_+.-]', '+', chroot_path.strip('/'))[-90:]
  vg_prefix = 'cros_%s_' % safe_path

  cmd = ['pvs', '-q', '--noheadings', '-o', 'vg_name,pv_name', '--unbuffered',
         '--separator', '\t']
  result = cros_build_lib.SudoRunCommand(
      cmd, capture_output=True, print_cmd=False)
  existing_vgs = set()
  for line in result.output.strip().splitlines():
    # Typical lines are '  vg_name\tpv_name\n'.  Match with a regex
    # instead of split because the first field can be empty or missing when
    # a VG isn't completely set up.
    match = re.match(r'([^\t]+)\t(.*)$', line.strip(' '))
    if not match:
      continue
    vg_name, pv_name = match.group(1), match.group(2)
    if chroot_dev == pv_name:
      return vg_name
    elif vg_name.startswith(vg_prefix):
      existing_vgs.add(vg_name)

  for i in xrange(1000):
    vg_name = '%s%03d' % (vg_prefix, i)
    if vg_name not in existing_vgs:
      return vg_name

  logging.error('Unable to find an unused VG with prefix %s', vg_prefix)
  return None


def _DeviceFromFile(chroot_image):
  """Finds the loopback device associated with |chroot_image|.

  Returns:
    The path to a loopback device (e.g. /dev/loop0) attached to |chroot_image|
    if one is found, or None if no device is found.
  """
  chroot_dev = None
  cmd = ['losetup', '-j', chroot_image]
  result = cros_build_lib.SudoRunCommand(
      cmd, capture_output=True, error_code_ok=True, print_cmd=False)
  if result.returncode == 0:
    match = re.match(r'/dev/loop\d+', result.output)
    if match:
      chroot_dev = match.group(0)
  return chroot_dev


def _AttachDeviceToFile(chroot_image):
  """Attaches a new loopback device to |chroot_image|.

  Returns:
    The path to the new loopback device.

  Raises:
    RunCommandError: The losetup command failed to attach a new device.
  """
  cmd = ['losetup', '--show', '-f', chroot_image]
  # Result should be '/dev/loopN\n' for whatever loop device is chosen.
  result = cros_build_lib.SudoRunCommand(
      cmd, capture_output=True, print_cmd=False)
  chroot_dev = result.output.strip()

  # Force rescanning the new device in case lvmetad doesn't pick it up.
  _RescanDeviceLvmMetadata(chroot_dev)
  return chroot_dev


def MountChroot(chroot=None, buildroot=None, create=True,
                proc_mounts='/proc/mounts'):
  """Mount a chroot image on |chroot| if it doesn't already contain a chroot.

  This function does not populate the chroot.  If there is an existing .img
  file, it will be mounted on |chroot|.  Otherwise a new empty filesystem will
  be mounted.  This function is a no-op if |chroot| already appears to contain
  a populated chroot.

  Args:
    chroot: Full path to the chroot to examine, or None to find it relative
        to |buildroot|.
    buildroot: Ignored if |chroot| is set.  If |chroot| is None, find the chroot
        relative to |buildroot|.
    create: Create a new image file if needed.  If False, only mount an
        existing image.
    proc_mounts: Full path to a file containing a list of mounted filesystems.
        Intended for testing only.

  Returns:
    True if the chroot is mounted, or False if not.

  Raises:
    RunCommandError: An external command failed.
  """
  if chroot is None and buildroot is None:
    raise ValueError('need either |chroot| or |buildroot| to search')
  if chroot is None:
    chroot = os.path.join(buildroot, constants.DEFAULT_CHROOT_DIR)

  # If there's a version file, this chroot is already set up.
  ver_path = os.path.join(chroot, 'etc', 'cros_chroot_version')
  if os.path.exists(ver_path):
    return True

  # Even if there isn't a version file in the chroot, there might already
  # be an image mounted on it.
  chroot_vg, chroot_lv = FindChrootMountSource(chroot, proc_mounts=proc_mounts)
  if chroot_vg and chroot_lv:
    return True

  # Make sure nothing else is mounted on the chroot.  We could mount over the
  # top, but this seems likely to be an error, so we'll bail out instead.
  chroot_mounts = [m.source
                   for m in osutils.IterateMountPoints(proc_file=proc_mounts)
                   if m.destination == chroot]
  if chroot_mounts:
    logging.error('Found %s mounted on %s.  Not mounting a chroot over the top',
                  ','.join(chroot_mounts), chroot)
    return False

  # Create a sparse 500GB file to hold the chroot image.  If we create an
  # image, immediately attach to a loopback device to skip one call to losetup.
  chroot_image = chroot + '.img'
  chroot_dev = None
  if not os.path.exists(chroot_image):
    if not create:
      return False

    logging.debug('Creating image %s', chroot_image)
    with open(chroot_image, 'w') as f:
      f.seek(500 * 2**30)  # 500GB sparse image.
      f.write('\0')

    chroot_dev = _AttachDeviceToFile(chroot_image)

  # Attach the image to a loopback device.
  if not chroot_dev:
    chroot_dev = _DeviceFromFile(chroot_image)
    if chroot_dev:
      logging.debug('Used existing device %s for %s', chroot_dev, chroot_image)
    else:
      chroot_dev = _AttachDeviceToFile(chroot_image)
  logging.debug('Loopback device is %s', chroot_dev)

  # Make sure there is a VG on the loopback device.
  chroot_vg = FindVolumeGroupForDevice(chroot, chroot_dev)
  if not chroot_vg:
    logging.error('Unable to find a VG for %s on %s', chroot, chroot_dev)
    return False
  cmd = ['vgs', chroot_vg]
  result = cros_build_lib.SudoRunCommand(
      cmd, capture_output=True, error_code_ok=True, print_cmd=False)
  if result.returncode == 0:
    logging.debug('Activating existing VG %s', chroot_vg)
    cmd = ['vgchange', '-q', '-ay', chroot_vg]
    # Sometimes LVM's internal thin volume check won't finish quickly enough
    # and this command will fail.  When this is the case, it will succeed if
    # we retry.  If it fails three times in a row, assume there's a real error
    # and re-raise the exception.
    try_count = xrange(1, 4)
    for i in try_count:
      try:
        cros_build_lib.SudoRunCommand(cmd, capture_output=True, print_cmd=False)
        break
      except cros_build_lib.RunCommandError:
        logging.warning('Failed to activate VG on try %d.', i)
        if i == len(try_count):
          raise
  else:
    cmd = ['vgcreate', '-q', chroot_vg, chroot_dev]
    cros_build_lib.SudoRunCommand(cmd, capture_output=True, print_cmd=False)

  # Make sure there is an LV containing a filesystem in our VG.
  chroot_lv = '%s/chroot' % chroot_vg
  chroot_dev_path = '/dev/%s' % chroot_lv
  cmd = ['lvs', chroot_lv]
  result = cros_build_lib.SudoRunCommand(
      cmd, capture_output=True, error_code_ok=True, print_cmd=False)
  if result.returncode == 0:
    logging.debug('Activating existing LV %s', chroot_lv)
    cmd = ['lvchange', '-q', '-ay', chroot_lv]
  else:
    cmd = ['lvcreate', '-q', '-L499G', '-T',
           '%s/%s' % (chroot_vg, CHROOT_THINPOOL_NAME), '-V500G',
           '-n', CHROOT_LV_NAME]
    cros_build_lib.SudoRunCommand(cmd, capture_output=True, print_cmd=False)

    cmd = ['mke2fs', '-q', '-m', '0', '-t', 'ext4', chroot_dev_path]
    cros_build_lib.SudoRunCommand(cmd, capture_output=True, print_cmd=False)

  osutils.SafeMakedirsNonRoot(chroot)

  # Sometimes lvchange can take a few seconds to run.  Try to wait for the
  # device to appear before mounting it.
  count = 0
  while not os.path.exists(chroot_dev_path):
    if count > _MAX_LVM_RETRIES:
      logging.error('Device %s still does not exist.  Expect mounting the '
                    'filesystem to fail.', chroot_dev_path)
      break

    count += 1
    logging.warning('Device file %s does not exist yet on try %d.',
                    chroot_dev_path, count)
    time.sleep(1)

  cmd = ['mount', '-text4', '-onoatime', chroot_dev_path, chroot]
  cros_build_lib.SudoRunCommand(cmd, capture_output=True, print_cmd=False)

  return True


def FindChrootMountSource(chroot_path, proc_mounts='/proc/mounts'):
  """Find the VG and LV mounted on |chroot_path|.

  Args:
    chroot_path: The full path to a mounted chroot.
    proc_mounts: The path to a list of mounts to read (intended for testing).

  Returns:
    A tuple containing the VG and LV names, or (None, None) if an appropriately
    named device mounted on |chroot_path| isn't found.
  """
  mount = [m for m in osutils.IterateMountPoints(proc_file=proc_mounts)
           if m.destination == chroot_path]
  if not mount:
    return (None, None)

  # Take the last mount entry because it's the one currently visible.
  # Expected VG/LV source path is /dev/mapper/cros_XX_NNN-LV.
  # See FindVolumeGroupForDevice for details.
  mount_source = mount[-1].source
  match = re.match(r'/dev.*/(cros[^-]+)-(.+)', mount_source)
  if not match:
    return (None, None)

  return (match.group(1), match.group(2))


def CleanupChrootMount(chroot=None, buildroot=None, delete_image=False,
                       proc_mounts='/proc/mounts'):
  """Unmounts a chroot and cleans up attached devices.

  This function attempts to perform all of the cleanup steps even if the chroot
  directory and/or image isn't present.  This ensures that a partially destroyed
  chroot can still be cleaned up.  This function does not remove the actual
  chroot directory (or its content for non-loopback chroots).

  Args:
    chroot: Full path to the chroot to examine, or None to find it relative
        to |buildroot|.
    buildroot: Ignored if |chroot| is set.  If |chroot| is None, find the chroot
        relative to |buildroot|.
    delete_image: Also delete the .img file after cleaning up.  If
        |delete_image| is False, the chroot contents will still be present and
        can be immediately re-mounted without recreating a fresh chroot.
    proc_mounts: The path to a list of mounts to read (intended for testing).
  """
  if chroot is None and buildroot is None:
    raise ValueError('need either |chroot| or |buildroot| to search')
  if chroot is None:
    chroot = os.path.join(buildroot, constants.DEFAULT_CHROOT_DIR)
  chroot_img = chroot + '.img'

  # Try to find the VG that might already be mounted on the chroot before we
  # unmount it.
  vg_name, _ = FindChrootMountSource(chroot, proc_mounts=proc_mounts)

  osutils.UmountTree(chroot)

  # Find the loopback device by either matching the VG or the image.
  chroot_dev = None
  if vg_name:
    cmd = ['vgs', '-q', '--noheadings', '-o', 'pv_name', '--unbuffered',
           vg_name]
    result = cros_build_lib.SudoRunCommand(
        cmd, capture_output=True, error_code_ok=True, print_cmd=False)
    if result.returncode == 0:
      chroot_dev = result.output.strip()
    else:
      vg_name = None
  if not chroot_dev:
    chroot_dev = _DeviceFromFile(chroot_img)

  # If we didn't find a mounted VG before but we did find a loopback device,
  # re-check for a VG attached to the loopback.
  if not vg_name:
    vg_name = FindVolumeGroupForDevice(chroot, chroot_dev)
    if vg_name:
      cmd = ['vgs', vg_name]
      result = cros_build_lib.SudoRunCommand(
          cmd, capture_output=True, error_code_ok=True, print_cmd=False)
      if result.returncode != 0:
        vg_name = None

  # Clean up all the pieces we found above.
  if vg_name:
    cmd = ['vgchange', '-an', vg_name]
    cros_build_lib.SudoRunCommand(cmd, capture_output=True, print_cmd=False)
  if chroot_dev:
    cmd = ['losetup', '-d', chroot_dev]
    cros_build_lib.SudoRunCommand(cmd, capture_output=True, print_cmd=False)
  if delete_image:
    osutils.SafeUnlink(chroot_img)
  if chroot_dev:
    # Force a rescan after everything is gone to make sure lvmetad is updated.
    _RescanDeviceLvmMetadata(chroot_dev)


def _RescanDeviceLvmMetadata(chroot_dev):
  """Forces lvmetad to rescan a device.

  After attaching or detaching a loopback device, lvmetad is supposed to
  automatically scan it.  This doesn't always happen reliably, so this function
  lets you force an LVM rescan.  This is intended for cases where the whole
  device will be used as an LVM PV, not for cases where you want to rescan a
  device's partition table.  For manipulating loopback device partitions, see
  the image_lib.LoopbackPartitions class.

  Args:
    chroot_dev: Full path to the device that should be rescanned.
  """
  # This may fail if lvmetad isn't in use, but it's faster to ignore the
  # exit code than to check if we should actually run the command.
  cmd = ['pvscan', '--cache', chroot_dev]
  cros_build_lib.SudoRunCommand(
      cmd, capture_output=True, print_cmd=False, error_code_ok=True)
