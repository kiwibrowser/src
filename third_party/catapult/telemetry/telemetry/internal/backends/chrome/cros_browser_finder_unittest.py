# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import itertools
import mock
import unittest

from telemetry.testing import options_for_unittests
from telemetry.internal.browser import browser_options as browser_options_module
from telemetry.internal.backends.chrome import cros_browser_finder


class CrOSBrowserMockCreationTest(unittest.TestCase):
  """Tests that a CrOS browser can be created by the Telemetry APIs.

  The platform and various other components are mocked out so these tests can
  be run even without access to an actual cros device.
  """
  def setUp(self):
    self.finder_options = options_for_unittests.GetCopy()

    # Use a mock platform, so no actions are performed on the actual platform.
    # Also get the cri used by the browser_backend to interact with the mock
    # cros device.
    self.mock_platform = mock.Mock()
    self.cri = self.mock_platform._platform_backend.cri

    # The browser session waits for IsCryptohomeMounted to become True while
    # starting, and then False when closing.
    self.cri.IsCryptohomeMounted.side_effect = itertools.cycle([True, False])
    # We expect the browser to be restarted, and it's pid change, a few times.
    self.cri.GetChromePid.side_effect = itertools.count(123)
    # This value is used when reading the DevToolsActivePort file.
    self.cri.GetFileContents.return_value = '8888\n'
    # This value is used to compute the browser_directory.
    self.cri.GetChromeProcess.return_value = {
        'path': '/path/to/browser',
        'pid': 3456,
        'args': 'chrome --foo',
    }

    # Mock the DevToolsClientConfig class, so we don't actually try to connect
    # to a devtools agent.
    self.devtools_config = self._PatchClass(
        'telemetry.internal.backends.chrome_inspector.'
        'devtools_client_backend.DevToolsClientConfig')

    # Mock the MiscWebContentsBackend, this is used to check for OOBE.
    self.misc_web_contents_backend = self._PatchClass(
        'telemetry.internal.backends.chrome.'
        'misc_web_contents_backend.MiscWebContentsBackend')
    # We expect the OOBE to appear and then be dismissed.
    type(self.misc_web_contents_backend).oobe_exists = mock.PropertyMock(
        side_effect=itertools.cycle([True, False]))

  def _PatchClass(self, target):
    """Patch a class importable as the given target.

    Returns the mock instance that the class would return when instantiated.
    """
    patcher = mock.patch(target, autospec=True)
    self.addCleanup(patcher.stop)
    return patcher.start().return_value

  def _CreateBrowser(self, browser_type, with_oobe=False):
    browser_options = self.finder_options.browser_options
    browser_options.browser_type = browser_type
    # This will "cast" browser_options to the correct CrosBrowserOptions class.
    browser_options = browser_options_module.CreateChromeBrowserOptions(
        browser_options)
    self.finder_options.browser_options = browser_options

    browser_options.create_browser_with_oobe = with_oobe
    possible_browser = cros_browser_finder.PossibleCrOSBrowser(
        browser_type, self.finder_options, self.mock_platform,
        is_guest=browser_type.endswith('-guest'))
    return possible_browser.BrowserSession(browser_options)

  def testCreateCrOSBrowser(self):
    with self._CreateBrowser('cros-chrome') as browser:
      self.assertIsNotNone(browser)

  def testCreateCrOSBrowserAsGuest(self):
    with self._CreateBrowser('cros-chrome-guest') as browser:
      self.assertIsNotNone(browser)

  def testCreateCrOSBrowserWithOOBE(self):
    with self._CreateBrowser('cros-chrome', with_oobe=True) as browser:
      self.assertIsNotNone(browser)
