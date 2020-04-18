# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for the flash module."""

from __future__ import print_function

import mock
import os

from chromite.cli import flash
from chromite.lib import auto_updater
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import cros_test_lib
from chromite.lib import dev_server_wrapper
from chromite.lib import osutils
from chromite.lib import partial_mock
from chromite.lib import remote_access


class RemoteDeviceUpdaterMock(partial_mock.PartialCmdMock):
  """Mock out RemoteDeviceUpdater."""
  TARGET = 'chromite.lib.auto_updater.ChromiumOSFlashUpdater'
  ATTRS = ('UpdateStateful', 'UpdateRootfs', 'SetupRootfsUpdate',
           'RebootAndVerify')

  def __init__(self):
    partial_mock.PartialCmdMock.__init__(self)

  def UpdateStateful(self, _inst, *_args, **_kwargs):
    """Mock out UpdateStateful."""

  def UpdateRootfs(self, _inst, *_args, **_kwargs):
    """Mock out UpdateRootfs."""

  def SetupRootfsUpdate(self, _inst, *_args, **_kwargs):
    """Mock out SetupRootfsUpdate."""

  def RebootAndVerify(self, _inst, *_args, **_kwargs):
    """Mock out RebootAndVerify."""


class RemoteDeviceUpdaterTest(cros_test_lib.MockTempDirTestCase):
  """Test the flow of flash.Flash() with RemoteDeviceUpdater."""

  IMAGE = '/path/to/image'
  DEVICE = commandline.Device(scheme=commandline.DEVICE_SCHEME_SSH,
                              hostname='1.1.1.1')

  def setUp(self):
    """Patches objects."""
    self.updater_mock = self.StartPatcher(RemoteDeviceUpdaterMock())
    self.PatchObject(dev_server_wrapper, 'GenerateXbuddyRequest',
                     return_value='xbuddy/local/latest')
    self.PatchObject(dev_server_wrapper, 'DevServerWrapper')
    self.PatchObject(dev_server_wrapper, 'GetImagePathWithXbuddy',
                     return_value=('taco-paladin/R36/chromiumos_test_image.bin',
                                   'remote/taco-paladin/R36/test'))
    self.PatchObject(dev_server_wrapper, 'GetUpdatePayloads')
    self.PatchObject(remote_access, 'CHECK_INTERVAL', new=0)
    self.PatchObject(remote_access, 'ChromiumOSDevice')

  def testUpdateAll(self):
    """Tests that update methods are called correctly."""
    with mock.patch('os.path.exists', return_value=True):
      flash.Flash(self.DEVICE, self.IMAGE)
      self.assertTrue(self.updater_mock.patched['UpdateStateful'].called)
      self.assertTrue(self.updater_mock.patched['UpdateRootfs'].called)

  def testUpdateStateful(self):
    """Tests that update methods are called correctly."""
    with mock.patch('os.path.exists', return_value=True):
      flash.Flash(self.DEVICE, self.IMAGE, rootfs_update=False)
      self.assertTrue(self.updater_mock.patched['UpdateStateful'].called)
      self.assertFalse(self.updater_mock.patched['UpdateRootfs'].called)

  def testUpdateRootfs(self):
    """Tests that update methods are called correctly."""
    with mock.patch('os.path.exists', return_value=True):
      flash.Flash(self.DEVICE, self.IMAGE, stateful_update=False)
      self.assertFalse(self.updater_mock.patched['UpdateStateful'].called)
      self.assertTrue(self.updater_mock.patched['UpdateRootfs'].called)

  def testMissingPayloads(self):
    """Tests we raise FlashError when payloads are missing."""
    with mock.patch('os.path.exists', return_value=False):
      self.assertRaises(auto_updater.ChromiumOSUpdateError, flash.Flash,
                        self.DEVICE, self.IMAGE)


class USBImagerMock(partial_mock.PartialCmdMock):
  """Mock out USBImager."""
  TARGET = 'chromite.cli.flash.USBImager'
  ATTRS = ('CopyImageToDevice', 'InstallImageToDevice',
           'ChooseRemovableDevice', 'ListAllRemovableDevices',
           'GetRemovableDeviceDescription')
  VALID_IMAGE = True

  def __init__(self):
    partial_mock.PartialCmdMock.__init__(self)

  def CopyImageToDevice(self, _inst, *_args, **_kwargs):
    """Mock out CopyImageToDevice."""

  def InstallImageToDevice(self, _inst, *_args, **_kwargs):
    """Mock out InstallImageToDevice."""

  def ChooseRemovableDevice(self, _inst, *_args, **_kwargs):
    """Mock out ChooseRemovableDevice."""

  def ListAllRemovableDevices(self, _inst, *_args, **_kwargs):
    """Mock out ListAllRemovableDevices."""
    return ['foo', 'taco', 'milk']

  def GetRemovableDeviceDescription(self, _inst, *_args, **_kwargs):
    """Mock out GetRemovableDeviceDescription."""


class USBImagerTest(cros_test_lib.MockTempDirTestCase):
  """Test the flow of flash.Flash() with USBImager."""
  IMAGE = '/path/to/image'

  def Device(self, path):
    """Create a USB device for passing to flash.Flash()."""
    return commandline.Device(scheme=commandline.DEVICE_SCHEME_USB,
                              path=path)

  def setUp(self):
    """Patches objects."""
    self.usb_mock = USBImagerMock()
    self.imager_mock = self.StartPatcher(self.usb_mock)
    self.PatchObject(dev_server_wrapper, 'GenerateXbuddyRequest',
                     return_value='xbuddy/local/latest')
    self.PatchObject(dev_server_wrapper, 'DevServerWrapper')
    self.PatchObject(dev_server_wrapper, 'GetImagePathWithXbuddy',
                     return_value=('taco-paladin/R36/chromiumos_test_image.bin',
                                   'remote/taco-paladin/R36/test'))
    self.PatchObject(os.path, 'exists', return_value=True)
    self.isgpt_mock = self.PatchObject(flash, '_IsFilePathGPTDiskImage',
                                       return_value=True)

  def testLocalImagePathCopy(self):
    """Tests that imaging methods are called correctly."""
    with mock.patch('os.path.isfile', return_value=True):
      flash.Flash(self.Device('/dev/foo'), self.IMAGE)
      self.assertTrue(self.imager_mock.patched['CopyImageToDevice'].called)

  def testLocalImagePathInstall(self):
    """Tests that imaging methods are called correctly."""
    with mock.patch('os.path.isfile', return_value=True):
      flash.Flash(self.Device('/dev/foo'), self.IMAGE, board='taco',
                  install=True)
      self.assertTrue(self.imager_mock.patched['InstallImageToDevice'].called)

  def testLocalBadImagePath(self):
    """Tests that using an image not having the magic bytes has prompt."""
    self.isgpt_mock.return_value = False
    with mock.patch('os.path.isfile', return_value=True):
      with mock.patch.object(cros_build_lib, 'BooleanPrompt') as mock_prompt:
        mock_prompt.return_value = False
        flash.Flash(self.Device('/dev/foo'), self.IMAGE)
        self.assertTrue(mock_prompt.called)

  def testNonLocalImagePath(self):
    """Tests that we try to get the image path using xbuddy."""
    with mock.patch.object(
        dev_server_wrapper,
        'GetImagePathWithXbuddy',
        return_value=('translated/xbuddy/path',
                      'resolved/xbuddy/path')) as mock_xbuddy:
      with mock.patch('os.path.isfile', return_value=False):
        with mock.patch('os.path.isdir', return_value=False):
          flash.Flash(self.Device('/dev/foo'), self.IMAGE)
          self.assertTrue(mock_xbuddy.called)

  def testConfirmNonRemovableDevice(self):
    """Tests that we ask user to confirm if the device is not removable."""
    with mock.patch.object(cros_build_lib, 'BooleanPrompt') as mock_prompt:
      flash.Flash(self.Device('/dev/dummy'), self.IMAGE)
      self.assertTrue(mock_prompt.called)

  def testSkipPromptNonRemovableDevice(self):
    """Tests that we skip the prompt for non-removable with --yes."""
    with mock.patch.object(cros_build_lib, 'BooleanPrompt') as mock_prompt:
      flash.Flash(self.Device('/dev/dummy'), self.IMAGE, yes=True)
      self.assertFalse(mock_prompt.called)

  def testChooseRemovableDevice(self):
    """Tests that we ask user to choose a device if none is given."""
    flash.Flash(self.Device(''), self.IMAGE)
    self.assertTrue(self.imager_mock.patched['ChooseRemovableDevice'].called)


class UsbImagerOperationTest(cros_test_lib.RunCommandTestCase):
  """Tests for flash.UsbImagerOperation."""
  # pylint: disable=protected-access

  def setUp(self):
    self.PatchObject(flash.UsbImagerOperation, '__init__', return_value=None)

  def testUsbImagerOperationCalled(self):
    """Test that flash.UsbImagerOperation is called when log level <= NOTICE."""
    expected_cmd = ['dd', 'if=foo', 'of=bar', 'bs=4M', 'iflag=fullblock',
                    'oflag=sync']
    usb_imager = flash.USBImager('dummy_device', 'board', 'foo')
    run_mock = self.PatchObject(flash.UsbImagerOperation, 'Run')
    self.PatchObject(logging.Logger, 'getEffectiveLevel',
                     return_value=logging.NOTICE)
    usb_imager.CopyImageToDevice('foo', 'bar')

    # Check that flash.UsbImagerOperation.Run() is called correctly.
    run_mock.assert_called_with(cros_build_lib.SudoRunCommand, expected_cmd,
                                debug_level=logging.NOTICE, update_period=0.5)

  def testSudoRunCommandCalled(self):
    """Test that SudoRunCommand is called when log level > NOTICE."""
    expected_cmd = ['dd', 'if=foo', 'of=bar', 'bs=4M', 'iflag=fullblock',
                    'oflag=sync']
    usb_imager = flash.USBImager('dummy_device', 'board', 'foo')
    run_mock = self.PatchObject(cros_build_lib, 'SudoRunCommand')
    self.PatchObject(logging.Logger, 'getEffectiveLevel',
                     return_value=logging.WARNING)
    usb_imager.CopyImageToDevice('foo', 'bar')

    # Check that SudoRunCommand() is called correctly.
    run_mock.assert_any_call(expected_cmd, debug_level=logging.NOTICE,
                             print_cmd=False)

  def testPingDD(self):
    """Test that UsbImagerOperation._PingDD() sends the correct signal."""
    expected_cmd = ['kill', '-USR1', '5']
    run_mock = self.PatchObject(cros_build_lib, 'SudoRunCommand')
    op = flash.UsbImagerOperation('foo')
    op._PingDD(5)

    # Check that SudoRunCommand was called correctly.
    run_mock.assert_called_with(expected_cmd, print_cmd=False)

  def testGetDDPidFound(self):
    """Check that the expected pid is returned for _GetDDPid()."""
    expected_pid = 5
    op = flash.UsbImagerOperation('foo')
    self.PatchObject(osutils, 'IsChildProcess', return_value=True)
    self.rc.AddCmdResult(partial_mock.Ignore(),
                         output='%d\n10\n' % expected_pid)

    pid = op._GetDDPid()

    # Check that the correct pid was returned.
    self.assertEqual(pid, expected_pid)

  def testGetDDPidNotFound(self):
    """Check that -1 is returned for _GetDDPid() if the pids aren't valid."""
    expected_pid = -1
    op = flash.UsbImagerOperation('foo')
    self.PatchObject(osutils, 'IsChildProcess', return_value=False)
    self.rc.AddCmdResult(partial_mock.Ignore(), output='5\n10\n')

    pid = op._GetDDPid()

    # Check that the correct pid was returned.
    self.assertEqual(pid, expected_pid)


class FlashUtilTest(cros_test_lib.MockTempDirTestCase):
  """Tests the helpers from cli.flash."""

  def testChooseImage(self):
    """Tests that we can detect a GPT image."""
    # pylint: disable=protected-access

    with self.PatchObject(flash, '_IsFilePathGPTDiskImage', return_value=True):
      # No images defined. Choosing the image should raise an error.
      with self.assertRaises(ValueError):
        flash._ChooseImageFromDirectory(self.tempdir)

      file_a = os.path.join(self.tempdir, 'a')
      osutils.Touch(file_a)
      # Only one image available, it should be selected automatically.
      self.assertEqual(file_a, flash._ChooseImageFromDirectory(self.tempdir))

      osutils.Touch(os.path.join(self.tempdir, 'b'))
      file_c = os.path.join(self.tempdir, 'c')
      osutils.Touch(file_c)
      osutils.Touch(os.path.join(self.tempdir, 'd'))

      # Multiple images available, we should ask the user to select the right
      # image.
      with self.PatchObject(cros_build_lib, 'GetChoice', return_value=2):
        self.assertEqual(file_c, flash._ChooseImageFromDirectory(self.tempdir))

  def testIsFilePathGPTDiskImage(self):
    """Tests the GPT image probing."""
    # pylint: disable=protected-access

    INVALID_PMBR = ' ' * 0x200
    INVALID_GPT = ' ' * 0x200
    VALID_PMBR = (' ' * 0x1fe) + '\x55\xaa'
    VALID_GPT = 'EFI PART' + (' ' * 0x1f8)
    TESTCASES = (
        (False, False, INVALID_PMBR + INVALID_GPT),
        (False, False, VALID_PMBR + INVALID_GPT),
        (False, True, INVALID_PMBR + VALID_GPT),
        (True, True, VALID_PMBR + VALID_GPT),
    )

    img = os.path.join(self.tempdir, 'img.bin')
    for exp_pmbr_t, exp_pmbr_f, data in TESTCASES:
      osutils.WriteFile(img, data)
      self.assertEqual(
          flash._IsFilePathGPTDiskImage(img, require_pmbr=True), exp_pmbr_t)
      self.assertEqual(
          flash._IsFilePathGPTDiskImage(img, require_pmbr=False), exp_pmbr_f)
