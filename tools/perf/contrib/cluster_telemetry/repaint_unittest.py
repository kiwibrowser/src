# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from contrib.cluster_telemetry import repaint_helpers


from telemetry import decorators
from telemetry.page import page as page_module
from telemetry.testing import options_for_unittests
from telemetry.testing import page_test_test_case
from telemetry.util import wpr_modes

from measurements import smoothness


class TestRepaintPage(page_module.Page):

  def __init__(self, page_set, base_dir):
    super(TestRepaintPage, self).__init__('file://blank.html',
                                          page_set, base_dir)

  def RunPageInteractions(self, action_runner):
    repaint_helpers.Repaint(action_runner)


class RepaintUnitTest(page_test_test_case.PageTestTestCase):
  """Smoke test for repaint measurement

     Runs repaint measurement on a simple page and verifies
     that all metrics were added to the results. The test is purely functional,
     i.e. it only checks if the metrics are present and non-zero.
  """

  def setUp(self):
    self._options = options_for_unittests.GetCopy()
    self._options.browser_options.wpr_mode = wpr_modes.WPR_OFF

  # Previously this test was disabled on chromeos, see crbug.com/483212.
  @decorators.Disabled("all") # crbug.com/715962
  def testRepaint(self):
    ps = self.CreateEmptyPageSet()
    ps.AddStory(TestRepaintPage(ps, ps.base_dir))
    measurement = smoothness.Smoothness()
    results = self.RunMeasurement(measurement, ps, options=self._options)
    self.assertFalse(results.had_failures)

    frame_times = results.FindAllPageSpecificValuesNamed('frame_times')
    self.assertEquals(len(frame_times), 1)
    self.assertGreater(frame_times[0].GetRepresentativeNumber(), 0)

    mean_frame_time = results.FindAllPageSpecificValuesNamed('mean_frame_time')
    self.assertEquals(len(mean_frame_time), 1)
    self.assertGreater(mean_frame_time[0].GetRepresentativeNumber(), 0)

    frame_time_discrepancy = results.FindAllPageSpecificValuesNamed(
        'frame_time_discrepancy')
    self.assertEquals(len(frame_time_discrepancy), 1)
    self.assertGreater(frame_time_discrepancy[0].GetRepresentativeNumber(), 0)

    percentage_smooth = results.FindAllPageSpecificValuesNamed(
        'percentage_smooth')
    self.assertEquals(len(percentage_smooth), 1)
    self.assertGreaterEqual(percentage_smooth[0].GetRepresentativeNumber(), 0)

    # Make sure that we don't have extra timeline based metrics that are not
    # related to smoothness.
    mainthread_jank = results.FindAllPageSpecificValuesNamed(
        'responsive-total_big_jank_thread_time')
    self.assertEquals(len(mainthread_jank), 0)
