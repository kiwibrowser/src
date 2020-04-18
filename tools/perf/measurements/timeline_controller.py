# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry.page import legacy_page_test
from telemetry.timeline.model import TimelineModel
from telemetry.timeline import tracing_config
from telemetry.value import trace
from telemetry.web_perf import smooth_gesture_util
from telemetry.web_perf import timeline_interaction_record as tir_module


RUN_SMOOTH_ACTIONS = 'RunSmoothAllActions'


class TimelineController(object):

  def __init__(self, enable_auto_issuing_record=True):
    super(TimelineController, self).__init__()
    self.trace_categories = None
    self.enable_systrace = False
    self._model = None
    self._renderer_process = None
    self._smooth_records = []
    self._interaction = None
    self._enable_auto_issuing_record = enable_auto_issuing_record

  def SetUp(self, page, tab):
    """Starts gathering timeline data.

    """
    del page  # unused
    # Resets these member variables incase this object is reused.
    self._model = None
    self._renderer_process = None
    if not tab.browser.platform.tracing_controller.IsChromeTracingSupported():
      raise Exception('Not supported')
    config = tracing_config.TracingConfig()
    config.chrome_trace_config.category_filter.AddFilterString(
        self.trace_categories)
    if self.enable_systrace:
      config.chrome_trace_config.SetEnableSystrace()
    config.enable_chrome_trace = True
    tab.browser.platform.tracing_controller.StartTracing(config)

  def Start(self, tab):
    # Start the smooth marker for all actions.
    if self._enable_auto_issuing_record:
      self._interaction = tab.action_runner.CreateInteraction(
          RUN_SMOOTH_ACTIONS)
      self._interaction.Begin()

  def Stop(self, tab, results):
    # End the smooth marker for all actions.
    if self._enable_auto_issuing_record:
      self._interaction.End()
    # Stop tracing.
    timeline_data = tab.browser.platform.tracing_controller.StopTracing()[0]

    # TODO(#763375): Rely on results.telemetry_info.trace_local_path/etc.
    kwargs = {}
    if hasattr(results.telemetry_info, 'trace_local_path'):
      kwargs['file_path'] = results.telemetry_info.trace_local_path
      kwargs['remote_path'] = results.telemetry_info.trace_remote_path
      kwargs['upload_bucket'] = results.telemetry_info.upload_bucket
      kwargs['cloud_url'] = results.telemetry_info.trace_remote_url
    results.AddValue(trace.TraceValue(
        results.current_page, timeline_data, **kwargs))

    self._model = TimelineModel(timeline_data)
    self._renderer_process = self._model.GetRendererProcessFromTabId(tab.id)
    renderer_thread = self.model.GetRendererThreadFromTabId(tab.id)

    run_smooth_actions_record = None
    self._smooth_records = []
    for event in renderer_thread.async_slices:
      if not tir_module.IsTimelineInteractionRecord(event.name):
        continue
      r = tir_module.TimelineInteractionRecord.FromAsyncEvent(event)
      if r.label == RUN_SMOOTH_ACTIONS:
        assert run_smooth_actions_record is None, (
            'TimelineController cannot issue more than 1 %s record' %
            RUN_SMOOTH_ACTIONS)
        run_smooth_actions_record = r
      else:
        self._smooth_records.append(
            smooth_gesture_util.GetAdjustedInteractionIfContainGesture(
                self.model, r))

    # If there is no other smooth records, we make measurements on time range
    # marked by timeline_controller itself.
    # TODO(nednguyen): when crbug.com/239179 is marked fixed, makes sure that
    # page sets are responsible for issueing the markers themselves.
    if len(self._smooth_records) == 0 and run_smooth_actions_record:
      self._smooth_records = [run_smooth_actions_record]

    if len(self._smooth_records) == 0:
      raise legacy_page_test.Failure('No interaction record was created.')

  def CleanUp(self, platform):
    if platform.tracing_controller.is_tracing_running:
      platform.tracing_controller.StopTracing()

  @property
  def model(self):
    return self._model

  @property
  def renderer_process(self):
    return self._renderer_process

  @property
  def smooth_records(self):
    return self._smooth_records
