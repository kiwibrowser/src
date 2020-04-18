# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import mock
import unittest

from telemetry.internal.backends import android_browser_backend_settings as \
    backends

from devil.android.sdk import version_codes


class AndroidBackendSettingsUnittest(unittest.TestCase):
  def testUniqueBrowserTypes(self):
    browser_types = {}
    for new in backends.ANDROID_BACKEND_SETTINGS:
      old = browser_types.get(new.browser_type)
      self.assertIsNone(
          old,
          'duplicate browser type %s: %s and %s' % (new.browser_type, old, new))
      browser_types[new.browser_type] = new

  def testChromeApkOnMarshmallow(self):
    device = mock.Mock()
    device.build_version_sdk = version_codes.MARSHMALLOW
    self.assertEqual(
        backends.ANDROID_CHROME.GetApkName(device),
        'Chrome.apk')

  def testMonochromeApkOnNougat(self):
    device = mock.Mock()
    device.build_version_sdk = version_codes.NOUGAT
    self.assertEqual(
        backends.ANDROID_CHROME.GetApkName(device),
        'Monochrome.apk')

  def testSystemWebViewApkOnMarshmallow(self):
    device = mock.Mock()
    device.build_version_sdk = version_codes.MARSHMALLOW
    self.assertEqual(
        backends.ANDROID_WEBVIEW.GetApkName(device),
        'SystemWebView.apk')

  def testMonochromePublicApkOnNougat(self):
    device = mock.Mock()
    device.build_version_sdk = version_codes.NOUGAT
    self.assertEqual(
        backends.ANDROID_WEBVIEW.GetApkName(device),
        'MonochromePublic.apk')

  def testSystemWebViewGoogleApkOnMarshmallow(self):
    device = mock.Mock()
    device.build_version_sdk = version_codes.MARSHMALLOW
    self.assertEqual(
        backends.ANDROID_WEBVIEW_GOOGLE.GetApkName(device),
        'SystemWebViewGoogle.apk')

  def testMonochromeApkForWebViewOnNougat(self):
    device = mock.Mock()
    device.build_version_sdk = version_codes.NOUGAT
    self.assertEqual(
        backends.ANDROID_WEBVIEW_GOOGLE.GetApkName(device),
        'Monochrome.apk')
