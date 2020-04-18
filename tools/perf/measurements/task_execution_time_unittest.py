# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry import decorators
from telemetry.internal.results import page_test_results
from telemetry.page import page as page_module
from telemetry.testing import options_for_unittests
from telemetry.testing import page_test_test_case
from telemetry.timeline import model as model_module
from telemetry.timeline import slice as slice_data
from telemetry.util import wpr_modes

from measurements import task_execution_time


class TestTaskExecutionTimePage(page_module.Page):

  def __init__(self, page_set, base_dir):
    super(TestTaskExecutionTimePage, self).__init__(
        'file://blank.html', page_set, base_dir, name='blank.html')

  def RunPageInteractions(self, action_runner):
    with action_runner.CreateGestureInteraction('ScrollAction'):
      action_runner.ScrollPage()


# Disable for accessing private API of task_execution_time.
# pylint: disable=protected-access
class TaskExecutionTimeUnitTest(page_test_test_case.PageTestTestCase):

  def setUp(self):
    self._options = options_for_unittests.GetCopy()
    self._options.browser_options.wpr_mode = wpr_modes.WPR_OFF
    self._first_thread_name = (
        task_execution_time.TaskExecutionTime._RENDERER_THREADS[0])
    self._measurement = None
    self._page_set = None

  @decorators.Enabled('android')
  def testSomeResultsReturnedFromDummyPage(self):
    self._GenerateDataForEmptyPageSet()

    results = self.RunMeasurement(self._measurement,
                                  self._page_set,
                                  options=self._options)

    self.assertGreater(len(results.all_page_specific_values), 0)

  # http://crbug.com/466994
  @decorators.Disabled('all')
  def testSlicesConformToRequiredNamingConventionsUsingDummyPage(self):
    """This test ensures the presence of required keywords.

       Some arbitrary keywords are required to generate the names of the top 10
       tasks. The code has a weak dependancy on 'src_func', 'class' and 'line'
       existing; if they exist in a slice's args they are used to generate a
       name, if they don't exists the code falls back to using the name of the
       slice, which is less clear.

       If the code has been refactored and these keywords no longer exist
       the code that relies on them in task_execution_time.py should be
       updated to use the appropriate technique for assertaining this data
       (and this test changed in the same way).
    """
    self._GenerateDataForEmptyPageSet()

    self.RunMeasurement(self._measurement,
                        self._page_set,
                        options=self._options)

    required_keywords = {'src_func': 0, 'class': 0, 'line': 0}

    # Check all slices and count the uses of the required keywords.
    for thread in self._measurement._renderer_process.threads.itervalues():
      for slice_info in thread.IterAllSlices():
        _CheckSliceForKeywords(slice_info, required_keywords)

    # Confirm that all required keywords have at least one instance.
    for use_counts in required_keywords.itervalues():
      self.assertGreater(use_counts, 0)

  def testMockedResultsCorrectlyReturned(self):
    data = self._GenerateResultsFromMockedData()

    # Confirm we get back 4 results (3 tasks and a section-use %).
    self.assertEqual(len(data.results.all_page_specific_values), 4)

    # Check that the 3 tasks we added exist in the resulting output
    # sorted.
    task_prefix = 'process 1:%s:' % (self._first_thread_name)
    slow_result = self._findResultFromName(task_prefix + 'slow', data)
    self.assertEqual(slow_result.value, 1000)

    medium_result = self._findResultFromName(task_prefix + 'medium', data)
    self.assertEqual(medium_result.value, 500)

    fast_result = self._findResultFromName(task_prefix + 'fast', data)
    self.assertEqual(fast_result.value, 1)

  def testNonIdlePercentagesAreCorrect(self):
    data = self._GenerateResultsFromMockedData()

    # Confirm that 100% of tasks are in the normal section.
    percentage_result = self._findResultFromName(
        'process 1:%s:Section_%s' % (
            self._first_thread_name,
            task_execution_time.TaskExecutionTime.NORMAL_SECTION),
        data)
    self.assertEqual(percentage_result.value, 100)

  def testIdleTasksAreReported(self):
    data = self._GenerateResultsFromMockedIdleData()

    # The 'slow_sub_slice' should be inside the Idle section and therefore
    # removed from the results.
    for result in data.results.all_page_specific_values:
      if 'slow_sub_slice' in result.name:
        self.fail('Tasks within idle section should not be reported')

    # The 'not_idle' slice should not have the IDLE_SECTION added to its name
    # and should exist.
    for result in data.results.all_page_specific_values:
      if 'not_idle' in result.name:
        self.assertTrue(
            task_execution_time.TaskExecutionTime.IDLE_SECTION
            not in result.name)
        break
    else:
      self.fail('Task was incorrectly marked as Idle')

  def testIdlePercentagesAreCorrect(self):
    data = self._GenerateResultsFromMockedIdleData()

    # Check the percentage section usage is correctly calculated.
    # Total = 1000 (idle) + 250 (normal), so normal = (250 * 100) / 1250 = 20%.
    normal_percentage_result = self._findResultFromName(
        'process 1:%s:Section_%s' % (
            self._first_thread_name,
            task_execution_time.TaskExecutionTime.NORMAL_SECTION),
        data)
    self.assertEqual(normal_percentage_result.value, 20)
    # Check the percentage section usage is correctly calculated.
    idle_percentage_result = self._findResultFromName(
        'process 1:%s:Section_%s' % (
            self._first_thread_name,
            task_execution_time.TaskExecutionTime.IDLE_SECTION),
        data)
    self.assertEqual(idle_percentage_result.value, 80)

  def testTopNTasksAreCorrectlyReported(self):
    data = self._GenerateDataForEmptyPageSet()

    # Add too many increasing-durtation tasks and confirm we only get the
    # slowest _NUMBER_OF_RESULTS_TO_DISPLAY tasks reported back.
    duration = 0
    extra = 5
    for duration in xrange(
        task_execution_time.TaskExecutionTime._NUMBER_OF_RESULTS_TO_DISPLAY +
        extra):
      data.AddSlice('task' + str(duration), 0, duration)

    # Run the code we are testing.
    self._measurement._AddResults(data.results)

    # Check that the last (i.e. biggest) _NUMBER_OF_RESULTS_TO_DISPLAY get
    # returned in the results.
    for duration in xrange(
        extra,
        extra +
        task_execution_time.TaskExecutionTime._NUMBER_OF_RESULTS_TO_DISPLAY):
      self._findResultFromName(
          'process 1:%s:task%s' % (self._first_thread_name, str(duration)),
          data)

  def _findResultFromName(self, name, data):
    for result in data.results.all_page_specific_values:
      if result.name == name:
        return result
    self.fail('Expected result "%s" missing.' % (name))

  def _GenerateResultsFromMockedData(self):
    data = self._GenerateDataForEmptyPageSet()

    data.AddSlice('fast', 0, 1)
    data.AddSlice('medium', 0, 500)
    data.AddSlice('slow', 0, 1000)

    # Run the code we are testing and return results.
    self._measurement._AddResults(data.results)
    return data

  def _GenerateResultsFromMockedIdleData(self):
    data = self._GenerateDataForEmptyPageSet()

    # Make a slice that looks like an idle task parent.
    slice_start_time = 0
    slow_slice_duration = 1000
    fast_slice_duration = 250
    parent_slice = data.AddSlice(
        task_execution_time.TaskExecutionTime.IDLE_SECTION_TRIGGER,
        slice_start_time,
        slow_slice_duration)
    # Add a sub-slice, this should be reported back as occuring in idle time.
    sub_slice = slice_data.Slice(
        None,
        'category',
        'slow_sub_slice',
        slice_start_time,
        slow_slice_duration,
        slice_start_time,
        slow_slice_duration,
        [])
    parent_slice.sub_slices.append(sub_slice)

    # Add a non-idle task.
    data.AddSlice('not_idle', slice_start_time, fast_slice_duration)

    # Run the code we are testing.
    self._measurement._AddResults(data.results)

    return data

  def _GenerateDataForEmptyPageSet(self):
    self._measurement = task_execution_time.TaskExecutionTime()
    self._page_set = self.CreateEmptyPageSet()
    page = TestTaskExecutionTimePage(self._page_set, self._page_set.base_dir)
    self._page_set.AddStory(page)

    # Get the name of a thread used by task_execution_time metric and set up
    # some dummy execution data pretending to be from that thread & process.
    data = TaskExecutionTestData(self._first_thread_name)
    self._measurement._renderer_process = data._renderer_process

    # Pretend we are about to run the tests to silence lower level asserts.
    data.results.WillRunPage(page)

    return data


def _CheckSliceForKeywords(slice_info, required_keywords):
  for argument in slice_info.args:
    if argument in required_keywords:
      required_keywords[argument] += 1
  # recurse into our sub-slices.
  for sub_slice in slice_info.sub_slices:
    _CheckSliceForKeywords(sub_slice, required_keywords)


class TaskExecutionTestData(object):

  def __init__(self, thread_name):
    self._model = model_module.TimelineModel()
    self._renderer_process = self._model.GetOrCreateProcess(1)
    self._renderer_thread = self._renderer_process.GetOrCreateThread(2)
    self._renderer_thread.name = thread_name
    self._results = page_test_results.PageTestResults()

  @property
  def results(self):
    return self._results

  def AddSlice(self, name, timestamp, duration):
    new_slice = slice_data.Slice(
        None,
        'category',
        name,
        timestamp,
        duration,
        timestamp,
        duration,
        [])
    self._renderer_thread.all_slices.append(new_slice)
    return new_slice
