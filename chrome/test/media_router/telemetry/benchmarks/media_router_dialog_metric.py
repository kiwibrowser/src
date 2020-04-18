# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json

from telemetry.value import histogram
from telemetry.value import histogram_util
from telemetry.value import scalar

from metrics import Metric

HISTOGRAMS_TO_RECORD = [
  {
      'name': 'MediaRouter.Ui.Dialog.LoadedWithData', 'units': 'ms',
      'display_name': 'dialog_loaded_with_data',
      'type': histogram_util.BROWSER_HISTOGRAM,
      'description': 'The latency to render the media router dialog with data.',
  },
  {
      'name': 'MediaRouter.Ui.Dialog.Paint', 'units': 'ms',
      'display_name': 'dialog_paint',
      'type': histogram_util.BROWSER_HISTOGRAM,
      'description': 'The latency to paint the media router dialog.',
  }]


class MediaRouterDialogMetric(Metric):
  "A metric for media router dialog latency from histograms."

  def __init__(self):
    super(MediaRouterDialogMetric, self).__init__()
    self._histogram_start = dict()
    self._histogram_delta = dict()
    self._started = False

  def Start(self, page, tab):
    self._started = True

    for h in HISTOGRAMS_TO_RECORD:
      histogram_data = histogram_util.GetHistogram(
          h['type'], h['name'], tab)
      # Histogram data may not be available
      if not histogram_data:
        continue
      self._histogram_start[h['name']] = histogram_data

  def Stop(self, page, tab):
    assert self._started, 'Must call Start() first'
    for h in HISTOGRAMS_TO_RECORD:
      # Histogram data may not be available
      if h['name'] not in self._histogram_start:
        continue
      histogram_data = histogram_util.GetHistogram(
          h['type'], h['name'], tab)

      if not histogram_data:
        continue
      self._histogram_delta[h['name']] = histogram_util.SubtractHistogram(
          histogram_data, self._histogram_start[h['name']])

  def AddResults(self, tab, results):
    assert self._histogram_delta, 'Must call Stop() first'
    for h in HISTOGRAMS_TO_RECORD:
      # Histogram data may not be available
      if h['name'] not in self._histogram_delta:
        continue
      results.AddValue(histogram.HistogramValue(
          results.current_page, h['display_name'], h['units'],
          raw_value_json=self._histogram_delta[h['name']], important=False,
          description=h.get('description')))
