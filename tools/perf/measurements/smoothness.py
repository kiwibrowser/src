# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry.page import legacy_page_test
from telemetry.timeline import model as model_module
from telemetry.value import trace
from telemetry.web_perf.metrics import smoothness
from telemetry.web_perf import smooth_gesture_util
from telemetry.web_perf import timeline_interaction_record as tir_module
from telemetry.timeline import tracing_config


def _CollectRecordsFromRendererThreads(model, renderer_thread):
  records = []
  for event in renderer_thread.async_slices:
    if tir_module.IsTimelineInteractionRecord(event.name):
      interaction = tir_module.TimelineInteractionRecord.FromAsyncEvent(event)
      # Adjust the interaction record to match the synthetic gesture
      # controller if needed.
      interaction = (
          smooth_gesture_util.GetAdjustedInteractionIfContainGesture(
              model, interaction))
      records.append(interaction)
  return records


class Smoothness(legacy_page_test.LegacyPageTest):

  def __init__(self):
    super(Smoothness, self).__init__()
    self._results = None

  @classmethod
  def CustomizeBrowserOptions(cls, options):
    options.AppendExtraBrowserArgs('--enable-gpu-benchmarking')
    options.AppendExtraBrowserArgs('--touch-events=enabled')

  def WillNavigateToPage(self, page, tab):
    # FIXME: Remove webkit.console when blink.console lands in chromium and
    # the ref builds are updated. crbug.com/386847
    config = tracing_config.TracingConfig()
    config.enable_chrome_trace = True
    config.enable_platform_display_trace = True

    # Basic categories for smoothness.
    custom_categories = [
        'webkit.console', 'blink.console', 'benchmark', 'trace_event_overhead']
    for cat in custom_categories:
      config.chrome_trace_config.category_filter.AddFilterString(cat)

    # Extra categories from commandline flag.
    if self.options and self.options.extra_chrome_categories:
      config.chrome_trace_config.category_filter.AddFilterString(
          self.options.extra_chrome_categories)
    if self.options and self.options.enable_systrace:
      config.chrome_trace_config.SetEnableSystrace()

    tab.browser.platform.tracing_controller.StartTracing(config)

  def ValidateAndMeasurePage(self, _, tab, results):
    self._results = results
    trace_result = tab.browser.platform.tracing_controller.StopTracing()[0]
    trace_value = trace.TraceValue(
        results.current_page, trace_result,
        file_path=results.telemetry_info.trace_local_path,
        remote_path=results.telemetry_info.trace_remote_path,
        upload_bucket=results.telemetry_info.upload_bucket,
        cloud_url=results.telemetry_info.trace_remote_url)
    results.AddValue(trace_value)

    model = model_module.TimelineModel(trace_result)
    renderer_thread = model.GetRendererThreadFromTabId(tab.id)
    records = _CollectRecordsFromRendererThreads(model, renderer_thread)
    metric = smoothness.SmoothnessMetric()
    metric.AddResults(model, renderer_thread, records, results)

  def DidRunPage(self, platform):
    if platform.tracing_controller.is_tracing_running:
      trace_result = platform.tracing_controller.StopTracing()[0]
      if self._results:
        trace_value = trace.TraceValue(
            self._results.current_page, trace_result,
            file_path=self._results.telemetry_info.trace_local_path,
            remote_path=self._results.telemetry_info.trace_remote_path,
            upload_bucket=self._results.telemetry_info.upload_bucket,
            cloud_url=self._results.telemetry_info.trace_remote_url)

        self._results.AddValue(trace_value)
