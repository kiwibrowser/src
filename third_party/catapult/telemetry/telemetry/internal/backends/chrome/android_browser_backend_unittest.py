# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry.testing import browser_backend_test_case
from telemetry import decorators


class AndroidBrowserBackendTest(
    browser_backend_test_case.BrowserBackendTestCase):

  @decorators.Enabled('android')
  def testProfileDir(self):
    self.assertIsNotNone(self._browser_backend.profile_directory)
