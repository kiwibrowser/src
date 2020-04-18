# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from telemetry import decorators
from telemetry.page import legacy_page_test
from telemetry.testing import options_for_unittests
from telemetry.testing import page_test_test_case
from telemetry.util import wpr_modes

from measurements import rasterize_and_record_micro


class RasterizeAndRecordMicroUnitTest(page_test_test_case.PageTestTestCase):
  """Smoke test for rasterize_and_record_micro measurement

     Runs rasterize_and_record_micro measurement on a simple page and verifies
     that all metrics were added to the results. The test is purely functional,
     i.e. it only checks if the metrics are present and non-zero.
  """

  def setUp(self):
    self._options = options_for_unittests.GetCopy()
    self._options.browser_options.wpr_mode = wpr_modes.WPR_OFF

  @decorators.Disabled('win', 'chromeos')
  def testRasterizeAndRecordMicro(self):
    ps = self.CreateStorySetFromFileInUnittestDataDir('blank.html')
    measurement = rasterize_and_record_micro.RasterizeAndRecordMicro(
        rasterize_repeat=1, record_repeat=1, start_wait_time=0.0,
        report_detailed_results=True)
    try:
      results = self.RunMeasurement(measurement, ps, options=self._options)
    except legacy_page_test.TestNotSupportedOnPlatformError as failure:
      logging.warning(str(failure))
      return
    self.assertFalse(results.had_failures)

    rasterize_time = results.FindAllPageSpecificValuesNamed('rasterize_time')
    self.assertEquals(len(rasterize_time), 1)
    self.assertGreater(rasterize_time[0].GetRepresentativeNumber(), 0)

    record_time = results.FindAllPageSpecificValuesNamed('record_time')
    self.assertEquals(len(record_time), 1)
    self.assertGreater(record_time[0].GetRepresentativeNumber(), 0)

    rasterized_pixels = results.FindAllPageSpecificValuesNamed(
        'pixels_rasterized')
    self.assertEquals(len(rasterized_pixels), 1)
    self.assertGreater(rasterized_pixels[0].GetRepresentativeNumber(), 0)

    recorded_pixels = results.FindAllPageSpecificValuesNamed('pixels_recorded')
    self.assertEquals(len(recorded_pixels), 1)
    self.assertGreater(recorded_pixels[0].GetRepresentativeNumber(), 0)

    pixels_rasterized_with_non_solid_color = \
        results.FindAllPageSpecificValuesNamed(
            'pixels_rasterized_with_non_solid_color')
    self.assertEquals(len(pixels_rasterized_with_non_solid_color), 1)
    self.assertGreater(
        pixels_rasterized_with_non_solid_color[0].GetRepresentativeNumber(), 0)

    pixels_rasterized_as_opaque = \
        results.FindAllPageSpecificValuesNamed('pixels_rasterized_as_opaque')
    self.assertEquals(len(pixels_rasterized_as_opaque), 1)
    self.assertGreater(
        pixels_rasterized_as_opaque[0].GetRepresentativeNumber(), 0)

    total_layers = results.FindAllPageSpecificValuesNamed('total_layers')
    self.assertEquals(len(total_layers), 1)
    self.assertGreater(total_layers[0].GetRepresentativeNumber(), 0)

    total_picture_layers = \
        results.FindAllPageSpecificValuesNamed('total_picture_layers')
    self.assertEquals(len(total_picture_layers), 1)
    self.assertGreater(total_picture_layers[0].GetRepresentativeNumber(), 0)

    total_picture_layers_with_no_content = \
        results.FindAllPageSpecificValuesNamed(
            'total_picture_layers_with_no_content')
    self.assertEquals(len(total_picture_layers_with_no_content), 1)
    self.assertGreater(
        total_picture_layers_with_no_content[0].GetRepresentativeNumber(), 0)

    total_picture_layers_off_screen = \
        results.FindAllPageSpecificValuesNamed(
            'total_picture_layers_off_screen')
    self.assertEquals(len(total_picture_layers_off_screen), 1)
    self.assertEqual(
        total_picture_layers_off_screen[0].GetRepresentativeNumber(), 0)

    painter_memory_usage = results.FindAllPageSpecificValuesNamed(
        'painter_memory_usage')
    self.assertEquals(len(painter_memory_usage), 1)
    self.assertGreater(painter_memory_usage[0].GetRepresentativeNumber(), 0)

    paint_op_memory_usage = results.FindAllPageSpecificValuesNamed(
        'paint_op_memory_usage')
    self.assertEquals(len(paint_op_memory_usage), 1)
    self.assertGreater(paint_op_memory_usage[0].GetRepresentativeNumber(), 0)

    paint_op_count = results.FindAllPageSpecificValuesNamed(
        'paint_op_count')
    self.assertEquals(len(paint_op_count), 1)
    self.assertGreater(paint_op_count[0].GetRepresentativeNumber(), 0)
