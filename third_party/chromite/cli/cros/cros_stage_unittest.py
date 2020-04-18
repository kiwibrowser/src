# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module tests the cros stage command and subfunctions."""

from __future__ import print_function

from chromite.cli.cros import cros_stage
from chromite.lib import cros_test_lib


class GSURLRegexHelperTest(cros_test_lib.TestCase):
  """Test class for the GSURLRegexHelper function."""

  def testCorrectInputs(self):
    """Ensure expected inputs work."""
    gsurls = [('gs://chromeos-image-archive/peppy-release/R42-6744.0.0',
               'peppy', 'R42-6744.0.0'),
              ('gs://chromeos-image-archive/peppy-release/R42-6744.0.0/',
               'peppy', 'R42-6744.0.0'),
              ('gs://chromeos-image-archive/trybot-peppy-release/'
               'R42-6744.0.0-b77/', 'peppy', 'R42-6744.0.0-b77')]
    for (gsurl, board, build) in gsurls:
      match = cros_stage.GSURLRegexHelper(gsurl)
      self.assertNotEqual(match, None)
      self.assertEqual(match.group('board'), board)
      self.assertEqual(match.group('build_name'), build)

  def testBadInputs(self):
    """Ensure unexpected inputs don't work."""
    gsurls = ['gs://chromeos-image-archive/',
              'gs://chromeos-image-archive/peppy-release/',
              'gs://chromeos-image-archive/peppy-release/6744.0.0/'
              'gs://chromeos-image-archive/peppy/R42-6744.0.0/',
              'gs://chromeos-image-archive/peppy-release/LATEST',
              'http://my_server_name:8080/peppy-release/R42-6744.0.0']
    for gsurl in gsurls:
      match = cros_stage.GSURLRegexHelper(gsurl)
      self.assertEqual(match, None)
