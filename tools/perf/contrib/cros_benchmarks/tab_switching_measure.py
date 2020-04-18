# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""The tab switching measurement.

This measurement record the MPArch.RWH_TabSwitchPaintDuration histogram
of each tab swithcing.
"""

from telemetry.page import legacy_page_test
from telemetry.value import histogram
from telemetry.value import histogram_util

from contrib.cros_benchmarks import cros_utils


class CrosTabSwitchingMeasurement(legacy_page_test.LegacyPageTest):
  """Measures tab switching performance."""

  def __init__(self):
    super(CrosTabSwitchingMeasurement, self).__init__()
    self._first_histogram = None

  def CustomizeBrowserOptions(self, options):
    """Adding necessary browser flag to collect histogram."""
    options.AppendExtraBrowserArgs(['--enable-stats-collection-bindings'])

  def DidNavigateToPage(self, page, tab):
    """Record the starting histogram."""
    self._first_histogram = cros_utils.GetTabSwitchHistogramRetry(tab.browser)

  def ValidateAndMeasurePage(self, page, tab, results):
    """Record the ending histogram for the tab switching metric."""
    last_histogram = cros_utils.GetTabSwitchHistogramRetry(tab.browser)
    total_diff_histogram = histogram_util.SubtractHistogram(
        last_histogram, self._first_histogram)

    display_name = 'MPArch_RWH_TabSwitchPaintDuration'
    results.AddSummaryValue(
        histogram.HistogramValue(
            None, display_name, 'ms',
            raw_value_json=total_diff_histogram,
            important=False))
