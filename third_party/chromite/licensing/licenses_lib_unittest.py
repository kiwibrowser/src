# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test the license_lib module."""

from __future__ import print_function

import os

from chromite.lib import cros_test_lib
from chromite.lib import osutils
from chromite.licensing import licenses_lib


class LicenseLibTest(cros_test_lib.TempDirTestCase):
  """Limited tests for license_lib."""

  def testReadUnknownEncodedFile(self):
    """Validate the fix for crbug.com/654894."""
    bad_license = os.path.join(self.tempdir, 'license.rtf')
    osutils.WriteFile(bad_license, u'Foo\x00Bar')
    result = licenses_lib.ReadUnknownEncodedFile(bad_license)
    self.assertEqual(result, 'FooBar')
