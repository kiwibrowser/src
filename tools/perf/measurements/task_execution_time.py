# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry.page import legacy_page_test
from telemetry.timeline.model import TimelineModel
from telemetry.timeline import tracing_config
from telemetry.util import statistics
from telemetry.value import scalar


class TaskExecutionTime(legacy_page_test.LegacyPageTest):

  IDLE_SECTION_TRIGGER = 'SingleThreadIdleTaskRunner::RunTask'
  IDLE_SECTION = 'IDLE'
  NORMAL_SECTION = 'NORMAL'

  _TIME_OUT_IN_SECONDS = 60
  _NUMBER_OF_RESULTS_TO_DISPLAY = 10
  _BROWSER_THREADS = ['Chrome_ChildIOThread',
                      'Chrome_IOThread']
  _RENDERER_THREADS = ['Chrome_ChildIOThread',
                       'Chrome_IOThread',
                       'CrRendererMain']
  _CATEGORIES = ['benchmark',
                 'blink',
                 'blink.console',
                 'blink_gc',
                 'cc',
                 'gpu',
                 'ipc',
                 'renderer.scheduler',
                 'toplevel',
                 'v8',
                 'webkit.console']

  def __init__(self):
    super(TaskExecutionTime, self).__init__()
    self._renderer_process = None
    self._browser_process = None
    self._results = None

  def WillNavigateToPage(self, page, tab):
    del page  # unused
    config = tracing_config.TracingConfig()
    for category in self._CATEGORIES:
      config.chrome_trace_config.category_filter.AddIncludedCategory(
          category)
    config.enable_chrome_trace = True

    tab.browser.platform.tracing_controller.StartTracing(
        config, self._TIME_OUT_IN_SECONDS)

  def ValidateAndMeasurePage(self, page, tab, results):
    del page  # unused
    trace_data = tab.browser.platform.tracing_controller.StopTracing()[0]
    timeline_model = TimelineModel(trace_data)

    self._renderer_process = timeline_model.GetRendererProcessFromTabId(tab.id)
    self._browser_process = timeline_model.browser_process
    self._AddResults(results)

  def _AddResults(self, results):
    self._results = results

    for thread in self._BROWSER_THREADS:
      self._AddTasksFromThreadToResults(self._browser_process, thread)

    for thread in self._RENDERER_THREADS:
      self._AddTasksFromThreadToResults(self._renderer_process, thread)

  def _AddTasksFromThreadToResults(self, process, thread_name):
    if process is None:
      return

    sections = TaskExecutionTime._GetSectionsForThread(process, thread_name)

    self._ReportSectionPercentages(sections.values(),
                                   '%s:%s' % (process.name, thread_name))

    # Create list with top |_NUMBER_OF_RESULTS_TO_DISPLAY| for each section.
    for section in sections.itervalues():
      if section.name == TaskExecutionTime.IDLE_SECTION:
        # Skip sections we don't report.
        continue
      self._AddSlowestTasksToResults(section.tasks.values())

  def _AddSlowestTasksToResults(self, tasks):
    sorted_tasks = sorted(
        tasks,
        key=lambda slice: slice.median_self_duration,
        reverse=True)

    for task in sorted_tasks[:self.GetExpectedResultCount()]:
      self._results.AddValue(scalar.ScalarValue(
          self._results.current_page,
          task.name,
          'ms',
          task.median_self_duration,
          description='Slowest tasks'))

  def _ReportSectionPercentages(self, section_values, metric_prefix):
    all_sectionstotal_duration = sum(
        section.total_duration for section in section_values)

    if not all_sectionstotal_duration:
      # Nothing was recorded, so early out.
      return

    for section in section_values:
      section_name = section.name or TaskExecutionTime.NORMAL_SECTION
      section_percentage_of_total = (
          (section.total_duration * 100.0) / all_sectionstotal_duration)
      self._results.AddValue(scalar.ScalarValue(
          self._results.current_page,
          '%s:Section_%s' % (metric_prefix, section_name),
          '%',
          section_percentage_of_total,
          description='Idle task percentage'))

  @staticmethod
  def _GetSectionsForThread(process, target_thread):
    sections = {}

    for thread in process.threads.itervalues():
      if thread.name != target_thread:
        continue
      for task_slice in thread.IterAllSlices():
        _ProcessTasksForThread(
            sections,
            '%s:%s' % (process.name, thread.name),
            task_slice)

    return sections

  @staticmethod
  def GetExpectedResultCount():
    return TaskExecutionTime._NUMBER_OF_RESULTS_TO_DISPLAY


def _ProcessTasksForThread(
    sections,
    thread_name,
    task_slice,
    section_name=None):
  if task_slice.self_thread_time is None:
    # Early out if this slice is a TRACE_EVENT_INSTANT, as it has no duration.
    return

  # Note: By setting a different section below we split off this task into
  # a different sorting bucket. Too add extra granularity (e.g. tasks executed
  # during page loading) add logic to set a different section name here. The
  # section name is set before the slice's data is recorded so the triggering
  # event will be included in its own section (i.e. the idle trigger will be
  # recorded as an idle event).

  if task_slice.name == TaskExecutionTime.IDLE_SECTION_TRIGGER:
    section_name = TaskExecutionTime.IDLE_SECTION

  # Add the thread name and section (e.g. 'Idle') to the test name
  # so it is human-readable.
  reported_name = thread_name + ':'
  if section_name:
    reported_name += section_name + ':'

  if 'src_func' in task_slice.args:
    # Data contains the name of the timed function, use it as the name.
    reported_name += task_slice.args['src_func']
  elif 'line' in task_slice.args:
    # Data contains IPC class and line numbers, use these as the name.
    reported_name += 'IPC_Class_' + str(task_slice.args['class'])
    reported_name += ':Line_' + str(task_slice.args['line'])
  else:
    # Fall back to use the name of the task slice.
    reported_name += task_slice.name.lower()

  # Replace any '.'s with '_'s as V8 uses them and it confuses the dashboard.
  reported_name = reported_name.replace('.', '_')

  # If this task is in a new section create a section object and add it to the
  # section dictionary.
  if section_name not in sections:
    sections[section_name] = Section(section_name)

  sections[section_name].AddTask(reported_name, task_slice.self_thread_time)

  # Process sub slices recursively, passing the current section down.
  for sub_slice in task_slice.sub_slices:
    _ProcessTasksForThread(
        sections,
        thread_name,
        sub_slice,
        section_name)


class NameAndDurations(object):

  def __init__(self, name, self_duration):
    self.name = name
    self.self_durations = [self_duration]

  def Update(self, self_duration):
    self.self_durations.append(self_duration)

  @property
  def median_self_duration(self):
    return statistics.Median(self.self_durations)


class Section(object):

  def __init__(self, name):
    # A section holds a dictionary, keyed on task name, of all the tasks that
    # exist within it and the total duration of those tasks.
    self.name = name
    self.tasks = {}
    self.total_duration = 0

  def AddTask(self, name, duration):
    if name in self.tasks:
      # section_tasks already contains an entry for this (e.g. from an earlier
      # slice), add the new duration so we can calculate a median value later.
      self.tasks[name].Update(duration)
    else:
      # This is a new task so create a new entry for it.
      self.tasks[name] = NameAndDurations(name, duration)
    # Accumulate total duration for all tasks in this section.
    self.total_duration += duration
