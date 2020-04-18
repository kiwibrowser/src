# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry.value import scalar

from metrics import Metric


NETWORK_DATA_NOT_FOUND = 'Network data could not be found.'


# This is experimental. crbug.com/480512
# Will not be supported once network data is ported to TimelineBasedMetric.
class NetworkMetric(Metric):
  """NetworkMetrics gathers network statistics."""

  def __init__(self, platform):
    super(NetworkMetric, self).__init__()
    self._network_snd = None
    self._network_rcv = None
    self._platform = platform
    self._browser = None

  def Start(self, _, tab):
    """Start the per-page preparation for this metric.

    Here, this consists of recording the start value.
    """
    self._browser = tab.browser
    if not self._platform.CanMonitorNetworkData():
      return

    data = self._platform.GetNetworkData(self._browser)
    if data is not None:
      self._network_snd, self._network_rcv = data

  def Stop(self, _, tab):
    """Prepare the results for this page.

    The results are the differences between the current values
    and the values when Start() was called.
    """
    if not self._platform.CanMonitorNetworkData():
      return

    data = self._platform.GetNetworkData(self._browser)
    if data is not None:
      snd, rcv = data
      if self._network_snd is not None:
        self._network_snd = snd - self._network_snd
      if self._network_rcv is not None:
        self._network_rcv = rcv - self._network_rcv
    else:  # If end data cannot be found, report none.
      self._network_snd = None
      self._network_rcv = None

  def AddResults(self, tab, results):
    none_value_reason = (
        None if self._network_snd is not None else NETWORK_DATA_NOT_FOUND)
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'network_data_sent', 'kb', self._network_snd,
        important=False, none_value_reason=none_value_reason))
    none_value_reason = (
        None if self._network_rcv is not None else NETWORK_DATA_NOT_FOUND)
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'network_data_received', 'kb', self._network_rcv,
        important=False, none_value_reason=none_value_reason))
