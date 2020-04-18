# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import shutil
import tempfile

from telemetry import decorators
from telemetry.testing import options_for_unittests
from telemetry.testing import page_test_test_case
from telemetry.util import image_util
from contrib.cluster_telemetry import screenshot

class ScreenshotUnitTest(page_test_test_case.PageTestTestCase):

  def setUp(self):
    self._options = options_for_unittests.GetCopy()
    self._png_outdir = tempfile.mkdtemp('_png_test')

  def tearDown(self):
    shutil.rmtree(self._png_outdir)

  @decorators.Enabled('linux')
  def testScreenshot(self):
    # Screenshots for Cluster Telemetry purposes currently only supported on
    # Linux platform.
    page_set = self.CreateStorySetFromFileInUnittestDataDir(
      'screenshot_test.html')
    measurement = screenshot.Screenshot(self._png_outdir)
    self.RunMeasurement(measurement, page_set, options=self._options)

    path = self._png_outdir + '/' + page_set.stories[0].file_safe_name + '.png'
    self.assertTrue(os.path.exists(path))
    self.assertTrue(os.path.isfile(path))
    self.assertTrue(os.access(path, os.R_OK))

    image = image_util.FromPngFile(path)
    screenshot_pixels = image_util.Pixels(image)
    special_colored_pixel = bytearray([217, 115, 43])
    self.assertTrue(special_colored_pixel in screenshot_pixels)

  @decorators.Enabled('linux')
  def testIsScreenshotWithinDynamicContentThreshold(self):
    # TODO(lchoi): This unit test fails on Windows due to an apparent platform
    # dependent image decoding behavior that will need to be investigated in the
    # future if Cluster Telemetry ever becomes compatible with Windows.
    width = 2
    height = 1
    num_total_pixels = width * height

    content_pixels = bytearray([0, 0, 0, 128, 128, 128])
    base_screenshot = image_util.FromRGBPixels(width, height, content_pixels)

    next_pixels = bytearray([1, 1, 1, 128, 128, 128])
    next_screenshot = image_util.FromRGBPixels(width, height, next_pixels)
    expected_pixels = bytearray([0, 255, 255, 128, 128, 128])
    self.assertTrue(screenshot.IsScreenshotWithinDynamicContentThreshold(
                    base_screenshot, next_screenshot, content_pixels,
                    num_total_pixels, 0.51))
    self.assertTrue(expected_pixels == content_pixels)

    next_pixels = bytearray([0, 0, 0, 1, 1, 1])
    next_screenshot = image_util.FromRGBPixels(2, 1, next_pixels)
    expected_pixels = bytearray([0, 255, 255, 0, 255, 255])
    self.assertTrue(screenshot.IsScreenshotWithinDynamicContentThreshold(
                    base_screenshot, next_screenshot, content_pixels,
                    num_total_pixels, 0.51))
    self.assertTrue(expected_pixels == content_pixels)
    self.assertFalse(screenshot.IsScreenshotWithinDynamicContentThreshold(
                     base_screenshot, next_screenshot, content_pixels,
                     num_total_pixels, 0.49))
