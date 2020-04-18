# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import sys

from telemetry.util.mac import keychain_helper
from telemetry.value import histogram_util
from telemetry.value import scalar

from metrics import Metric


class KeychainMetric(Metric):
  """KeychainMetric gathers keychain statistics from the browser object.

  This includes the number of times that the keychain was accessed.
  """

  DISPLAY_NAME = 'OSX_Keychain_Access'
  HISTOGRAM_NAME = 'OSX.Keychain.Access'

  @staticmethod
  def _CheckKeychainConfiguration():
    """On OSX, it is possible for a misconfigured keychain to cause the
    Telemetry tests to stall.

    This method confirms that the keychain is in a sane state that will
    not cause this behavior. Three conditions are checked:
      - The keychain is unlocked.
      - The keychain will not auto-lock after a period of time.
      - The ACLs are correctly configured on the relevant keychain items.
    """
    warning_suffix = ('which will cause some Telemetry tests to stall when run'
                      ' on a headless machine (e.g. perf bot).')
    if keychain_helper.IsKeychainLocked():
      logging.warning('The default keychain is locked, %s', warning_suffix)

    if keychain_helper.DoesKeychainHaveTimeout():
      logging.warning('The default keychain is configured to automatically'
                      ' lock itself have a period of time, %s', warning_suffix)

    chrome_acl_configured = (keychain_helper.
                             IsKeychainConfiguredForBotsWithChrome())
    chromium_acl_configured = (keychain_helper.
                               IsKeychainConfiguredForBotsWithChromium())
    acl_warning = ('A commonly used %s key stored in the default keychain does'
                   ' not give decryption access to all applications, %s')
    if not chrome_acl_configured:
      logging.warning(acl_warning, 'Chrome', warning_suffix)
    if not chromium_acl_configured:
      logging.warning(acl_warning, 'Chromium', warning_suffix)

  @classmethod
  def CustomizeBrowserOptions(cls, options):
    """Adds a browser argument that allows for the collection of keychain
    metrics. Has no effect on non-Mac platforms.
    """
    if sys.platform != 'darwin':
      return

    KeychainMetric._CheckKeychainConfiguration()

    options.AppendExtraBrowserArgs(['--enable-stats-collection-bindings'])

  def AddResults(self, tab, results):
    """Adds the number of times that the keychain was accessed to |results|.
    Has no effect on non-Mac platforms.
    """
    if sys.platform != 'darwin':
      return

    access_count = histogram_util.GetHistogramSum(
        histogram_util.BROWSER_HISTOGRAM, KeychainMetric.HISTOGRAM_NAME, tab)
    results.AddValue(scalar.ScalarValue(
        results.current_page, KeychainMetric.DISPLAY_NAME, 'count',
        access_count))
