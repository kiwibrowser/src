# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry import value
from telemetry.web_perf.metrics import timeline_based_metric


class MediaRouterMetric(timeline_based_metric.TimelineBasedMetric):
  """Reports latency of media router dialog from trace event."""

  def AddResults(self, model, renderer_thread, interactions, results):
    browser = model.browser_process
    if not browser:
      return

    events = [e for e in browser.parent.IterAllEvents(
       event_predicate=lambda event: event.name == 'UI')]
    if len(events) > 0:
      results.AddValue(value.scalar.ScalarValue(
        page=results.current_page,
        name='media_router_ui',
        units='ms',
        description=('Latency between Media Router dialog '
                     'creation call and initialization'),
        value=events[0].duration,
        improvement_direction=value.improvement_direction.DOWN))
