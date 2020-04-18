# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest
import mock

from telemetry.internal.backends.chrome import chrome_startup_args
from telemetry.internal.browser import browser_options as browser_options_module
from telemetry.util import wpr_modes


class FakeBrowserOptions(browser_options_module.BrowserOptions):
  def __init__(self, wpr_mode=wpr_modes.WPR_OFF):
    super(FakeBrowserOptions, self).__init__()
    self.wpr_mode = wpr_mode
    self.browser_type = 'chrome'
    self.browser_user_agent_type = 'desktop'
    self.disable_background_networking = False
    self.disable_component_extensions_with_background_pages = False
    self.disable_default_apps = False


class StartupArgsTest(unittest.TestCase):
  """Test expected inputs for GetBrowserStartupArgs."""

  def testFeaturesMerged(self):
    browser_options = FakeBrowserOptions()
    browser_options.AppendExtraBrowserArgs([
        '--disable-features=Feature1,Feature2',
        '--disable-features=Feature2,Feature3',
        '--enable-features=Feature4,Feature5',
        '--enable-features=Feature5,Feature6',
        '--foo'])

    startup_args = chrome_startup_args.GetFromBrowserOptions(browser_options)
    self.assertTrue('--foo' in startup_args)
    # Make sure there's only once instance of --enable/disable-features and it
    # contains all values
    disable_count = 0
    enable_count = 0
    # Merging is done using using sets, so any order is correct
    for arg in startup_args:
      if arg.startswith('--disable-features='):
        split_arg = arg.split('=', 1)[1].split(',')
        self.assertEquals({'Feature1', 'Feature2', 'Feature3'}, set(split_arg))
        disable_count += 1
      elif arg.startswith('--enable-features='):
        split_arg = arg.split('=', 1)[1].split(',')
        self.assertEquals({'Feature4', 'Feature5', 'Feature6'}, set(split_arg))
        enable_count += 1
    self.assertEqual(1, disable_count)
    self.assertEqual(1, enable_count)


class ReplayStartupArgsTest(unittest.TestCase):
  """Test expected inputs for GetReplayArgs."""
  def testReplayOffGivesEmptyArgs(self):
    network_backend = mock.Mock()
    network_backend.is_open = False
    network_backend.forwarder = None

    self.assertEqual([], chrome_startup_args.GetReplayArgs(network_backend))

  def testReplayArgsBasic(self):
    network_backend = mock.Mock()
    network_backend.is_open = True
    network_backend.use_live_traffic = False
    network_backend.forwarder.remote_port = 789

    expected_args = [
        '--proxy-server=socks://localhost:789',
        '--ignore-certificate-errors-spki-list='
        'PhrPvGIaAMmd29hj8BCZOq096yj7uMpRNHpn5PDxI6I=']
    self.assertItemsEqual(
        expected_args,
        chrome_startup_args.GetReplayArgs(network_backend))

  def testReplayArgsNoSpkiSupport(self):
    network_backend = mock.Mock()
    network_backend.is_open = True
    network_backend.use_live_traffic = False
    network_backend.forwarder.remote_port = 789

    expected_args = [
        '--proxy-server=socks://localhost:789',
        '--ignore-certificate-errors']
    self.assertItemsEqual(
        expected_args,
        chrome_startup_args.GetReplayArgs(network_backend, False))

  def testReplayArgsUseLiveTrafficWithSpkiSupport(self):
    network_backend = mock.Mock()
    network_backend.is_open = True
    network_backend.use_live_traffic = True
    network_backend.forwarder.remote_port = 789

    expected_args = [
        '--proxy-server=socks://localhost:789']
    self.assertItemsEqual(
        expected_args,
        chrome_startup_args.GetReplayArgs(network_backend,
                                          supports_spki_list=True))

  def testReplayArgsUseLiveTrafficWithNoSpkiSupport(self):
    network_backend = mock.Mock()
    network_backend.is_open = True
    network_backend.use_live_traffic = True
    network_backend.forwarder.remote_port = 123

    expected_args = [
        '--proxy-server=socks://localhost:123']
    self.assertItemsEqual(
        expected_args,
        chrome_startup_args.GetReplayArgs(network_backend,
                                          supports_spki_list=False))




