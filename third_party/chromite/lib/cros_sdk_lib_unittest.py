# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test the cros_sdk_lib module."""

from __future__ import print_function

import os

from chromite.lib import cros_sdk_lib
from chromite.lib import cros_test_lib
from chromite.lib import osutils
from chromite.lib import partial_mock


class TestGetChrootVersion(cros_test_lib.MockTestCase):
  """Tests GetChrootVersion functionality."""

  def testSimpleBuildroot(self):
    """Verify buildroot arg works"""
    read_mock = self.PatchObject(osutils, 'ReadFile', return_value='12\n')
    ret = cros_sdk_lib.GetChrootVersion(buildroot='/build/root')
    self.assertEqual(ret, '12')
    read_mock.assert_called_with('/build/root/chroot/etc/cros_chroot_version')

  def testSimpleChroot(self):
    """Verify chroot arg works"""
    read_mock = self.PatchObject(osutils, 'ReadFile', return_value='70')
    ret = cros_sdk_lib.GetChrootVersion(chroot='/ch/root')
    self.assertEqual(ret, '70')
    read_mock.assert_called_with('/ch/root/etc/cros_chroot_version')

  def testNoChroot(self):
    """Verify we don't blow up when there is no chroot yet"""
    ret = cros_sdk_lib.GetChrootVersion(chroot='/.$om3/place/nowhere')
    self.assertEqual(ret, None)


class TestFindVolumeGroupForDevice(cros_test_lib.MockTempDirTestCase):
  """Tests the FindVolumeGroupForDevice function."""

  def testExistingDevice(self):
    with cros_test_lib.RunCommandMock() as rc_mock:
      rc_mock.SetDefaultCmdResult(output='''
  wrong_vg1\t/dev/sda1
  test_vg\t/dev/loop1
  wrong_vg2\t/dev/loop0
''')
      vg = cros_sdk_lib.FindVolumeGroupForDevice('/chroot', '/dev/loop1')
      self.assertEqual(vg, 'test_vg')

  def testNoMatchingVolumeGroup(self):
    with cros_test_lib.RunCommandMock() as rc_mock:
      rc_mock.SetDefaultCmdResult(output='''
  wrong_vg1\t/dev/sda1
  wrong_vg2\t/dev/loop0
''')
      vg = cros_sdk_lib.FindVolumeGroupForDevice('/chroot', '')
      self.assertEqual(vg, 'cros_chroot_000')

  def testPhysicalVolumeWithoutVolumeGroup(self):
    with cros_test_lib.RunCommandMock() as rc_mock:
      rc_mock.SetDefaultCmdResult(output='''
  wrong_vg1\t/dev/sda1
  \t/dev/loop0
''')
      vg = cros_sdk_lib.FindVolumeGroupForDevice('/chroot', '/dev/loop0')
      self.assertEqual(vg, 'cros_chroot_000')

  def testMatchingVolumeGroup(self):
    with cros_test_lib.RunCommandMock() as rc_mock:
      rc_mock.SetDefaultCmdResult(output='''
  wrong_vg1\t/dev/sda1
  cros_chroot_000\t/dev/loop1
  wrong_vg2\t/dev/loop0
''')
      vg = cros_sdk_lib.FindVolumeGroupForDevice('/chroot', '')
      self.assertEqual(vg, 'cros_chroot_001')

  def testTooManyVolumeGroups(self):
    with cros_test_lib.RunCommandMock() as rc_mock:
      rc_mock.SetDefaultCmdResult(output='''
  wrong_vg1\t/dev/sda1
%s
  wrong_vg2\t/dev/loop0
''' % '\n'.join(['  cros_chroot_%03d\t/dev/any' % i for i in xrange(1000)]))
      vg = cros_sdk_lib.FindVolumeGroupForDevice('/chroot', '')
      self.assertIsNone(vg)

  def testInvalidChars(self):
    with cros_test_lib.RunCommandMock() as rc_mock:
      rc_mock.SetDefaultCmdResult(output='''
  wrong_vg1\t/dev/sda1
  cros_chroot_000\t/dev/loop1
  wrong_vg2\t/dev/loop0
''')
      vg = cros_sdk_lib.FindVolumeGroupForDevice(
          '//full path /to& "my" /chroot', '')
      self.assertEqual(vg, 'cros_full+path++to+++my+++chroot_000')

  def testInvalidLines(self):
    with cros_test_lib.RunCommandMock() as rc_mock:
      rc_mock.SetDefaultCmdResult(output='''
  \t/dev/sda1

  wrong_vg2\t/dev/loop0\t
''')
      vg = cros_sdk_lib.FindVolumeGroupForDevice('/chroot', '')
      self.assertEqual(vg, 'cros_chroot_000')


class TestMountChroot(cros_test_lib.MockTempDirTestCase):
  """Tests various partial setups for MountChroot."""

  _VGS_LOOKUP = ['sudo', '--', 'vgs', partial_mock.Ignore()]
  _VGCREATE = ['sudo', '--', 'vgcreate', '-q', partial_mock.Ignore(),
               partial_mock.Ignore()]
  _VGCHANGE = ['sudo', '--', 'vgchange', '-q', '-ay', partial_mock.Ignore()]
  _LVS_LOOKUP = ['sudo', '--', 'lvs', partial_mock.Ignore()]
  _LVCREATE = ['sudo', '--', 'lvcreate', '-q', '-L499G', '-T',
               partial_mock.Ignore(), '-V500G', '-n', partial_mock.Ignore()]
  _MKE2FS = ['sudo', '--', 'mke2fs', '-q', '-m', '0', '-t', 'ext4',
             partial_mock.Ignore()]
  _MOUNT = []  # Set correctly in setUp.
  _LVM_FAILURE_CODE = 5  # Shell exit code when lvm commands fail.
  _LVM_SUCCESS_CODE = 0  # Shell exit code when lvm commands succeed.

  def _makeImageFile(self, chroot_img):
    with open(chroot_img, 'w') as f:
      f.seek(2**30)
      f.write('\0')

  def _mockFindVolumeGroupForDevice(self):
    m = self.PatchObject(cros_sdk_lib, 'FindVolumeGroupForDevice')
    m.return_value = 'cros_test_chroot_000'
    return m

  def _mockAttachDeviceToFile(self, loop_dev='loop0'):
    m = self.PatchObject(cros_sdk_lib, '_AttachDeviceToFile')
    m.return_value = '/dev/%s' % loop_dev
    return m

  def _mockDeviceFromFile(self, dev):
    m = self.PatchObject(cros_sdk_lib, '_DeviceFromFile')
    m.return_value = dev
    return m

  def setUp(self):
    self.chroot_path = os.path.join(self.tempdir, 'chroot')
    osutils.SafeMakedirsNonRoot(self.chroot_path)
    self.chroot_img = self.chroot_path + '.img'

    self._MOUNT = ['sudo', '--', 'mount', '-text4', '-onoatime',
                   partial_mock.Ignore(), self.chroot_path]

  def testFromScratch(self):
    # Create the whole setup from nothing.
    # Should call losetup, vgs, vgcreate, lvs, lvcreate, mke2fs, mount
    m = self._mockFindVolumeGroupForDevice()
    m2 = self._mockAttachDeviceToFile()

    with cros_test_lib.RunCommandMock() as rc_mock:
      rc_mock.AddCmdResult(self._VGS_LOOKUP, returncode=self._LVM_FAILURE_CODE)
      rc_mock.AddCmdResult(self._VGCREATE, output='')
      rc_mock.AddCmdResult(self._LVS_LOOKUP, returncode=self._LVM_FAILURE_CODE)
      rc_mock.AddCmdResult(self._LVCREATE)
      rc_mock.AddCmdResult(self._MKE2FS)
      rc_mock.AddCmdResult(self._MOUNT)

      success = cros_sdk_lib.MountChroot(self.chroot_path)
      self.assertTrue(success)

    m.assert_called_with(self.chroot_path, '/dev/loop0')
    m2.assert_called_with(self.chroot_img)

  def testMissingMount(self):
    # Re-mount an image that has a loopback and VG active but isn't mounted.
    # This can happen if the person unmounts the chroot or calls
    # osutils.UmountTree() on the path.
    # Should call losetup, vgchange, lvs, mount
    self._makeImageFile(self.chroot_img)

    m = self._mockFindVolumeGroupForDevice()
    m2 = self._mockDeviceFromFile('/dev/loop1')

    with cros_test_lib.RunCommandMock() as rc_mock:
      rc_mock.AddCmdResult(self._VGS_LOOKUP, returncode=self._LVM_SUCCESS_CODE)
      rc_mock.AddCmdResult(self._VGCHANGE)
      rc_mock.AddCmdResult(self._LVS_LOOKUP, returncode=self._LVM_SUCCESS_CODE)
      rc_mock.AddCmdResult(self._MOUNT)

      success = cros_sdk_lib.MountChroot(self.chroot_path)
      self.assertTrue(success)

    m.assert_called_with(self.chroot_path, '/dev/loop1')
    m2.assert_called_with(self.chroot_img)

  def testImageAfterReboot(self):
    # Re-mount an image that has everything setup, but doesn't have anything
    # attached, e.g. after reboot.
    # Should call losetup -j, losetup -f, vgs, vgchange, lvs, lvchange, mount
    self._makeImageFile(self.chroot_img)

    m = self._mockFindVolumeGroupForDevice()
    m2 = self._mockDeviceFromFile('')
    m3 = self._mockAttachDeviceToFile('loop1')

    with cros_test_lib.RunCommandMock() as rc_mock:
      rc_mock.AddCmdResult(self._VGS_LOOKUP, returncode=self._LVM_SUCCESS_CODE)
      rc_mock.AddCmdResult(self._VGCHANGE)
      rc_mock.AddCmdResult(self._LVS_LOOKUP, returncode=self._LVM_SUCCESS_CODE)
      rc_mock.AddCmdResult(self._MOUNT)

      success = cros_sdk_lib.MountChroot(self.chroot_path)
      self.assertTrue(success)

    m.assert_called_with(self.chroot_path, '/dev/loop1')
    m2.assert_called_with(self.chroot_img)
    m3.assert_called_with(self.chroot_img)

  def testImagePresentNotSetup(self):
    # Mount an image that is present but doesn't have anything set up.  This
    # can't arise in normal usage, but could happen if cros_sdk crashes in the
    # middle of setup.
    # Should call losetup -j, losetup -f, vgs, vgcreate, lvs, lvcreate, mke2fs,
    # mount
    self._makeImageFile(self.chroot_img)

    m = self._mockFindVolumeGroupForDevice()
    m2 = self._mockAttachDeviceToFile()
    m3 = self._mockDeviceFromFile('')

    with cros_test_lib.RunCommandMock() as rc_mock:
      rc_mock.AddCmdResult(self._VGS_LOOKUP, returncode=self._LVM_FAILURE_CODE)
      rc_mock.AddCmdResult(self._VGCREATE)
      rc_mock.AddCmdResult(self._LVS_LOOKUP, returncode=self._LVM_FAILURE_CODE)
      rc_mock.AddCmdResult(self._LVCREATE)
      rc_mock.AddCmdResult(self._MKE2FS)
      rc_mock.AddCmdResult(self._MOUNT)

      success = cros_sdk_lib.MountChroot(self.chroot_path)
      self.assertTrue(success)

    m.assert_called_with(self.chroot_path, '/dev/loop0')
    m2.assert_called_with(self.chroot_img)
    m3.assert_called_with(self.chroot_img)

  def testImagePresentOnlyLoopbackSetup(self):
    # Mount an image that is present and attached to a loopback device, but
    # doesn't have anything else set up.  This can't arise in normal usage, but
    # could happen if cros_sdk crashes in the middle of setup.
    # Should call losetup, vgs, vgcreate, lvs, lvcreate, mke2fs, mount
    self._makeImageFile(self.chroot_img)

    m = self._mockFindVolumeGroupForDevice()
    m2 = self._mockDeviceFromFile('/dev/loop0')

    with cros_test_lib.RunCommandMock() as rc_mock:
      rc_mock.AddCmdResult(self._VGS_LOOKUP, returncode=self._LVM_FAILURE_CODE)
      rc_mock.AddCmdResult(self._VGCREATE)
      rc_mock.AddCmdResult(self._LVS_LOOKUP, returncode=self._LVM_FAILURE_CODE)
      rc_mock.AddCmdResult(self._LVCREATE)
      rc_mock.AddCmdResult(self._MKE2FS)
      rc_mock.AddCmdResult(self._MOUNT)

      success = cros_sdk_lib.MountChroot(self.chroot_path)
      self.assertTrue(success)

    m.assert_called_with(self.chroot_path, '/dev/loop0')
    m2.assert_called_with(self.chroot_img)

  def testImagePresentOnlyVgSetup(self):
    # Mount an image that is present, attached to a loopback device, and has a
    # VG, but doesn't have anything else set up.  This can't arise in normal
    # usage, but could happen if cros_sdk crashes in the middle of setup.
    # Should call losetup, vgs, vgchange, lvs, lvcreate, mke2fs, mount
    self._makeImageFile(self.chroot_img)

    m = self._mockFindVolumeGroupForDevice()
    m2 = self._mockDeviceFromFile('/dev/loop0')

    with cros_test_lib.RunCommandMock() as rc_mock:
      rc_mock.AddCmdResult(self._VGS_LOOKUP, returncode=self._LVM_SUCCESS_CODE)
      rc_mock.AddCmdResult(self._VGCHANGE)
      rc_mock.AddCmdResult(self._LVS_LOOKUP, returncode=self._LVM_FAILURE_CODE)
      rc_mock.AddCmdResult(self._LVCREATE)
      rc_mock.AddCmdResult(self._MKE2FS)
      rc_mock.AddCmdResult(self._MOUNT)

      success = cros_sdk_lib.MountChroot(self.chroot_path)
      self.assertTrue(success)

    m.assert_called_with(self.chroot_path, '/dev/loop0')
    m2.assert_called_with(self.chroot_img)

  def testMissingNoCreate(self):
    # Chroot image isn't present, but create is False.
    # Should return False without running any commands.
    success = cros_sdk_lib.MountChroot(self.chroot_path, create=False)
    self.assertFalse(success)

  def testExistingChroot(self):
    # Chroot version file exists in the chroot.
    # Should return True without running any commands.
    osutils.Touch(os.path.join(self.chroot_path, 'etc', 'cros_chroot_version'),
                  makedirs=True)

    success = cros_sdk_lib.MountChroot(self.chroot_path, create=False)
    self.assertTrue(success)

    success = cros_sdk_lib.MountChroot(self.chroot_path, create=True)
    self.assertTrue(success)

  def testEmptyChroot(self):
    # Chroot mounted from proper image but without the version file present,
    # e.g. if cros_sdk fails in the middle of populating the chroot.
    # Should return True without running any commands.
    proc_mounts = os.path.join(self.tempdir, 'proc_mounts')
    with open(proc_mounts, 'w') as f:
      f.write('/dev/mapper/cros_test_000-chroot %s ext4 rw 0 0\n' %
              self.chroot_path)

    success = cros_sdk_lib.MountChroot(
        self.chroot_path, create=False, proc_mounts=proc_mounts)
    self.assertTrue(success)

    success = cros_sdk_lib.MountChroot(
        self.chroot_path, create=True, proc_mounts=proc_mounts)
    self.assertTrue(success)

  def testBadMount(self):
    # Chroot with something else mounted on it.
    # Should return False without running any commands.
    proc_mounts = os.path.join(self.tempdir, 'proc_mounts')
    with open(proc_mounts, 'w') as f:
      f.write('/dev/sda1 %s ext4 rw 0 0\n' % self.chroot_path)

    success = cros_sdk_lib.MountChroot(
        self.chroot_path, create=False, proc_mounts=proc_mounts)
    self.assertFalse(success)

    success = cros_sdk_lib.MountChroot(
        self.chroot_path, create=True, proc_mounts=proc_mounts)
    self.assertFalse(success)


class TestFindChrootMountSource(cros_test_lib.MockTempDirTestCase):
  """Tests the FindChrootMountSource function."""
  def testNoMatchingMount(self):
    proc_mounts = os.path.join(self.tempdir, 'proc_mounts')
    with open(proc_mounts, 'w') as f:
      f.write('sysfs /sys sysfs rw 0 0\n')

    vg, lv = cros_sdk_lib.FindChrootMountSource('/chroot',
                                                proc_mounts=proc_mounts)
    self.assertIsNone(vg)
    self.assertIsNone(lv)

  def testMatchWrongName(self):
    proc_mounts = os.path.join(self.tempdir, 'proc_mounts')
    with open(proc_mounts, 'w') as f:
      f.write('/dev/sda1 /chroot ext4 rw 0 0\n')

    vg, lv = cros_sdk_lib.FindChrootMountSource('/chroot',
                                                proc_mounts=proc_mounts)
    self.assertIsNone(vg)
    self.assertIsNone(lv)

  def testMatchRightName(self):
    proc_mounts = os.path.join(self.tempdir, 'proc_mounts')
    with open(proc_mounts, 'w') as f:
      f.write('/dev/mapper/cros_vg_name-lv_name /chroot ext4 rw 0 0\n')

    vg, lv = cros_sdk_lib.FindChrootMountSource('/chroot',
                                                proc_mounts=proc_mounts)
    self.assertEqual(vg, 'cros_vg_name')
    self.assertEqual(lv, 'lv_name')

  def testMatchMultipleMounts(self):
    proc_mounts = os.path.join(self.tempdir, 'proc_mounts')
    with open(proc_mounts, 'w') as f:
      f.write('''/dev/mapper/cros_first_mount-lv_name /chroot ext4 rw 0 0
/dev/mapper/cros_inner_mount-lv /chroot/inner ext4 rw 0 0
/dev/mapper/cros_second_mount-lv_name /chroot ext4 rw 0 0
''')

    vg, lv = cros_sdk_lib.FindChrootMountSource('/chroot',
                                                proc_mounts=proc_mounts)
    self.assertEqual(vg, 'cros_second_mount')
    self.assertEqual(lv, 'lv_name')


class TestCleanupChrootMount(cros_test_lib.MockTempDirTestCase):
  """Tests the CleanupChrootMount function."""

  _VGS_DEV_LOOKUP = ['sudo', '--', 'vgs', '-q', '--noheadings', '-o', 'pv_name',
                     '--unbuffered', partial_mock.Ignore()]
  _VGS_VG_LOOKUP = ['sudo', '--', 'vgs', partial_mock.Ignore()]
  _LOSETUP_FIND = ['sudo', '--', 'losetup', '-j', partial_mock.Ignore()]
  _LOSETUP_DETACH = ['sudo', '--', 'losetup', '-d', partial_mock.Ignore()]
  _VGCHANGE_N = ['sudo', '--', 'vgchange', '-an', partial_mock.Ignore()]
  _LVM_FAILURE_CODE = 5  # Shell exit code when lvm commands fail.
  _LVM_SUCCESS_CODE = 0  # Shell exit code when lvm commands succeed.

  def setUp(self):
    self.chroot_path = os.path.join(self.tempdir, 'chroot')
    osutils.SafeMakedirsNonRoot(self.chroot_path)
    self.chroot_img = self.chroot_path + '.img'

  def testMountedCleanup(self):
    m = self.PatchObject(osutils, 'UmountTree')
    m2 = self.PatchObject(cros_sdk_lib, '_RescanDeviceLvmMetadata')

    proc_mounts = os.path.join(self.tempdir, 'proc_mounts')
    with open(proc_mounts, 'w') as f:
      f.write('/dev/mapper/cros_vg_name-chroot %s ext4 rw 0 0\n' %
              self.chroot_path)

    with cros_test_lib.RunCommandMock() as rc_mock:
      rc_mock.AddCmdResult(self._VGS_DEV_LOOKUP, output='  /dev/loop0')
      rc_mock.AddCmdResult(self._VGCHANGE_N)
      rc_mock.AddCmdResult(self._LOSETUP_DETACH)

      cros_sdk_lib.CleanupChrootMount(
          self.chroot_path, None, proc_mounts=proc_mounts)

    m.assert_called_with(self.chroot_path)
    m2.assert_called_with('/dev/loop0')

  def testMountedCleanupByBuildroot(self):
    m = self.PatchObject(osutils, 'UmountTree')
    m2 = self.PatchObject(cros_sdk_lib, '_RescanDeviceLvmMetadata')

    proc_mounts = os.path.join(self.tempdir, 'proc_mounts')
    with open(proc_mounts, 'w') as f:
      f.write('/dev/mapper/cros_vg_name-chroot %s ext4 rw 0 0\n' %
              self.chroot_path)

    with cros_test_lib.RunCommandMock() as rc_mock:
      rc_mock.AddCmdResult(self._VGS_DEV_LOOKUP, output='  /dev/loop0')
      rc_mock.AddCmdResult(self._VGCHANGE_N)
      rc_mock.AddCmdResult(self._LOSETUP_DETACH)

      cros_sdk_lib.CleanupChrootMount(
          None, self.tempdir, proc_mounts=proc_mounts)

    m.assert_called_with(self.chroot_path)
    m2.assert_called_with('/dev/loop0')

  def testMountedCleanupWithDelete(self):
    m = self.PatchObject(osutils, 'UmountTree')
    m2 = self.PatchObject(osutils, 'SafeUnlink')
    m3 = self.PatchObject(cros_sdk_lib, '_RescanDeviceLvmMetadata')

    proc_mounts = os.path.join(self.tempdir, 'proc_mounts')
    with open(proc_mounts, 'w') as f:
      f.write('/dev/mapper/cros_vg_name-chroot %s ext4 rw 0 0\n' %
              self.chroot_path)

    with cros_test_lib.RunCommandMock() as rc_mock:
      rc_mock.AddCmdResult(self._VGS_DEV_LOOKUP, output='  /dev/loop0')
      rc_mock.AddCmdResult(self._VGCHANGE_N)
      rc_mock.AddCmdResult(self._LOSETUP_DETACH)

      cros_sdk_lib.CleanupChrootMount(
          self.chroot_path, None, delete_image=True, proc_mounts=proc_mounts)

    m.assert_called_with(self.chroot_path)
    m2.assert_called_with(self.chroot_img)
    m3.assert_called_with('/dev/loop0')

  def testUnmountedCleanup(self):
    m = self.PatchObject(osutils, 'UmountTree')
    m2 = self.PatchObject(cros_sdk_lib, 'FindVolumeGroupForDevice')
    m2.return_value = 'cros_chroot_001'
    m3 = self.PatchObject(cros_sdk_lib, '_RescanDeviceLvmMetadata')

    proc_mounts = os.path.join(self.tempdir, 'proc_mounts')
    with open(proc_mounts, 'w') as f:
      f.write('sysfs /sys sysfs rw 0 0\n')

    with cros_test_lib.RunCommandMock() as rc_mock:
      rc_mock.AddCmdResult(self._LOSETUP_FIND, output='/dev/loop1')
      rc_mock.AddCmdResult(self._VGS_VG_LOOKUP,
                           returncode=self._LVM_SUCCESS_CODE)
      rc_mock.AddCmdResult(self._VGCHANGE_N)
      rc_mock.AddCmdResult(self._LOSETUP_DETACH)

      cros_sdk_lib.CleanupChrootMount(
          self.chroot_path, None, proc_mounts=proc_mounts)

    m.assert_called_with(self.chroot_path)
    m2.assert_called_with(self.chroot_path, '/dev/loop1')
    m3.assert_called_with('/dev/loop1')

  def testDevOnlyCleanup(self):
    m = self.PatchObject(osutils, 'UmountTree')
    m2 = self.PatchObject(cros_sdk_lib, 'FindVolumeGroupForDevice')
    m2.return_value = 'cros_chroot_001'
    m3 = self.PatchObject(cros_sdk_lib, '_RescanDeviceLvmMetadata')

    proc_mounts = os.path.join(self.tempdir, 'proc_mounts')
    with open(proc_mounts, 'w') as f:
      f.write('sysfs /sys sysfs rw 0 0\n')

    with cros_test_lib.RunCommandMock() as rc_mock:
      rc_mock.AddCmdResult(self._LOSETUP_FIND, output='/dev/loop1')
      rc_mock.AddCmdResult(self._VGS_VG_LOOKUP,
                           returncode=self._LVM_FAILURE_CODE)
      rc_mock.AddCmdResult(self._LOSETUP_DETACH)

      cros_sdk_lib.CleanupChrootMount(
          self.chroot_path, None, proc_mounts=proc_mounts)

    m.assert_called_with(self.chroot_path)
    m2.assert_called_with(self.chroot_path, '/dev/loop1')
    m3.assert_called_with('/dev/loop1')

  def testNothingCleanup(self):
    m = self.PatchObject(osutils, 'UmountTree')
    m2 = self.PatchObject(cros_sdk_lib, 'FindVolumeGroupForDevice')
    m2.return_value = 'cros_chroot_001'

    proc_mounts = os.path.join(self.tempdir, 'proc_mounts')
    with open(proc_mounts, 'w') as f:
      f.write('sysfs /sys sysfs rw 0 0\n')

    with cros_test_lib.RunCommandMock() as rc_mock:
      rc_mock.AddCmdResult(self._LOSETUP_FIND, returncode=1)
      rc_mock.AddCmdResult(self._VGS_VG_LOOKUP,
                           returncode=self._LVM_FAILURE_CODE)

      cros_sdk_lib.CleanupChrootMount(
          self.chroot_path, None, proc_mounts=proc_mounts)

    m.assert_called_with(self.chroot_path)
    m2.assert_called_with(self.chroot_path, None)

  def testNothingCleanupWithDelete(self):
    m = self.PatchObject(osutils, 'UmountTree')
    m2 = self.PatchObject(cros_sdk_lib, 'FindVolumeGroupForDevice')
    m2.return_value = 'cros_chroot_001'
    m3 = self.PatchObject(osutils, 'SafeUnlink')

    proc_mounts = os.path.join(self.tempdir, 'proc_mounts')
    with open(proc_mounts, 'w') as f:
      f.write('sysfs /sys sysfs rw 0 0\n')

    with cros_test_lib.RunCommandMock() as rc_mock:
      rc_mock.AddCmdResult(self._LOSETUP_FIND, returncode=1)
      rc_mock.AddCmdResult(self._VGS_VG_LOOKUP,
                           returncode=self._LVM_FAILURE_CODE)

      cros_sdk_lib.CleanupChrootMount(
          self.chroot_path, None, delete_image=True, proc_mounts=proc_mounts)

    m.assert_called_with(self.chroot_path)
    m2.assert_called_with(self.chroot_path, None)
    m3.assert_called_with(self.chroot_img)
