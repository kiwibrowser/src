# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import re
import sys
import StringIO
import time
import unittest

from telemetry import benchmark
from telemetry import story
from telemetry.core import exceptions
from telemetry.core import util
from telemetry import decorators
from telemetry.internal.browser import browser_finder
from telemetry.internal.browser import user_agent
from telemetry.internal.results import results_options
from telemetry.internal import story_runner
from telemetry.internal.testing.test_page_sets import example_domain
from telemetry.internal.util import exception_formatter
from telemetry.page import page as page_module
from telemetry.page import legacy_page_test
from telemetry.page import shared_page_state
from telemetry.page import traffic_setting as traffic_setting_module
from telemetry.util import image_util
from telemetry.testing import fakes
from telemetry.testing import options_for_unittests
from telemetry.testing import system_stub

from py_utils import tempfile_ext


class DummyTest(legacy_page_test.LegacyPageTest):

  def ValidateAndMeasurePage(self, *_):
    pass


def SetUpStoryRunnerArguments(options):
  parser = options.CreateParser()
  story_runner.AddCommandLineArgs(parser)
  options.MergeDefaultValues(parser.get_default_values())
  story_runner.ProcessCommandLineArgs(parser, options)


class EmptyMetadataForTest(benchmark.BenchmarkMetadata):

  def __init__(self):
    super(EmptyMetadataForTest, self).__init__('')


def GetSuccessfulPageRuns(results):
  return [run for run in results.all_page_runs if run.ok or run.skipped]


def CaptureStderr(func, output_buffer):
  def wrapper(*args, **kwargs):
    original_stderr, sys.stderr = sys.stderr, output_buffer
    try:
      return func(*args, **kwargs)
    finally:
      sys.stderr = original_stderr
  return wrapper


# TODO: remove test cases that use real browsers and replace with a
# story_runner or shared_page_state unittest that tests the same logic.
class ActualPageRunEndToEndTests(unittest.TestCase):
  # TODO(nduca): Move the basic "test failed, test succeeded" tests from
  # page_test_unittest to here.

  def setUp(self):
    self._story_runner_logging_stub = None
    self._formatted_exception_buffer = StringIO.StringIO()
    self._original_formatter = exception_formatter.PrintFormattedException

  def tearDown(self):
    self.RestoreExceptionFormatter()

  def CaptureFormattedException(self):
    exception_formatter.PrintFormattedException = CaptureStderr(
        exception_formatter.PrintFormattedException,
        self._formatted_exception_buffer)
    self._story_runner_logging_stub = system_stub.Override(
        story_runner, ['logging'])

  @property
  def formatted_exception(self):
    return self._formatted_exception_buffer.getvalue()

  def RestoreExceptionFormatter(self):
    exception_formatter.PrintFormattedException = self._original_formatter
    if self._story_runner_logging_stub:
      self._story_runner_logging_stub.Restore()
      self._story_runner_logging_stub = None

  def assertFormattedExceptionIsEmpty(self):
    self.longMessage = False
    self.assertEquals(
        '', self.formatted_exception,
        msg='Expected empty formatted exception: actual=%s' % '\n   > '.join(
            self.formatted_exception.split('\n')))

  def assertFormattedExceptionOnlyHas(self, expected_exception_name):
    self.longMessage = True
    actual_exception_names = re.findall(r'^Traceback.*?^(\w+)',
                                        self.formatted_exception,
                                        re.DOTALL | re.MULTILINE)
    self.assertEquals([expected_exception_name], actual_exception_names,
                      msg='Full formatted exception: %s' % '\n   > '.join(
                          self.formatted_exception.split('\n')))

  def testBrowserRestartsAfterEachPage(self):
    self.CaptureFormattedException()
    story_set = story.StorySet()
    story_set.AddStory(page_module.Page(
        'file://blank.html', story_set, base_dir=util.GetUnittestDataDir(),
        name='foo'))
    story_set.AddStory(page_module.Page(
        'file://blank.html', story_set, base_dir=util.GetUnittestDataDir(),
        name='bar'))

    class Test(legacy_page_test.LegacyPageTest):

      def __init__(self, *args, **kwargs):
        super(Test, self).__init__(*args, **kwargs)
        self.browser_starts = 0
        self.platform_name = None

      def DidStartBrowser(self, browser):
        super(Test, self).DidStartBrowser(browser)
        self.browser_starts += 1
        self.platform_name = browser.platform.GetOSName()

      def ValidateAndMeasurePage(self, page, tab, results):
        pass

    options = options_for_unittests.GetCopy()
    options.output_formats = ['none']
    options.suppress_gtest_report = True

    with tempfile_ext.NamedTemporaryDirectory('page_E2E_tests') as tempdir:
      options.output_dir = tempdir
      test = Test()
      SetUpStoryRunnerArguments(options)
      results = results_options.CreateResults(EmptyMetadataForTest(), options)
      story_runner.Run(test, story_set, options, results)
      self.assertEquals(len(story_set), len(GetSuccessfulPageRuns(results)))
      # Browser is started once per story run, except in ChromeOS where a single
      # instance is reused for all stories.
      if test.platform_name == 'chromeos':
        self.assertEquals(1, test.browser_starts)
      else:
        self.assertEquals(len(story_set), test.browser_starts)
      self.assertFormattedExceptionIsEmpty()

  @decorators.Disabled('chromeos')  # crbug.com/483212
  def testUserAgent(self):
    story_set = story.StorySet()
    page = page_module.Page(
        'file://blank.html', story_set, base_dir=util.GetUnittestDataDir(),
        shared_page_state_class=shared_page_state.SharedTabletPageState,
        name='blank.html')
    story_set.AddStory(page)

    class TestUserAgent(legacy_page_test.LegacyPageTest):
      def ValidateAndMeasurePage(self, page, tab, results):
        del page, results  # unused
        actual_user_agent = tab.EvaluateJavaScript(
            'window.navigator.userAgent')
        expected_user_agent = user_agent.UA_TYPE_MAPPING['tablet']
        assert actual_user_agent.strip() == expected_user_agent

        # This is so we can check later that the test actually made it into this
        # function. Previously it was timing out before even getting here, which
        # should fail, but since it skipped all the asserts, it slipped by.
        self.hasRun = True  # pylint: disable=attribute-defined-outside-init

    test = TestUserAgent()
    options = options_for_unittests.GetCopy()
    options.output_formats = ['none']
    options.suppress_gtest_report = True

    with tempfile_ext.NamedTemporaryDirectory('page_E2E_tests') as tempdir:
      options.output_dir = tempdir
      SetUpStoryRunnerArguments(options)
      results = results_options.CreateResults(EmptyMetadataForTest(), options)
      story_runner.Run(test, story_set, options, results)

      self.assertTrue(hasattr(test, 'hasRun') and test.hasRun)

  # Ensure that story_runner forces exactly 1 tab before running a page.
  @decorators.Enabled('has tabs')
  def testOneTab(self):
    story_set = story.StorySet()
    page = page_module.Page(
        'file://blank.html', story_set, base_dir=util.GetUnittestDataDir(),
        name='blank.html')
    story_set.AddStory(page)

    class TestOneTab(legacy_page_test.LegacyPageTest):

      def DidStartBrowser(self, browser):
        browser.tabs.New()

      def ValidateAndMeasurePage(self, page, tab, results):
        del page, results  # unused
        assert len(tab.browser.tabs) == 1

    test = TestOneTab()
    options = options_for_unittests.GetCopy()
    options.output_formats = ['none']
    options.suppress_gtest_report = True

    with tempfile_ext.NamedTemporaryDirectory('page_E2E_tests') as tempdir:
      options.output_dir = tempdir
      SetUpStoryRunnerArguments(options)
      results = results_options.CreateResults(EmptyMetadataForTest(), options)
      story_runner.Run(test, story_set, options, results)

  @decorators.Disabled('chromeos')  # crbug.com/652385
  def testTrafficSettings(self):
    story_set = story.StorySet()
    slow_page = page_module.Page(
        'file://green_rect.html', story_set, base_dir=util.GetUnittestDataDir(),
        name='slow',
        traffic_setting=traffic_setting_module.GOOD_3G)
    fast_page = page_module.Page(
        'file://green_rect.html', story_set, base_dir=util.GetUnittestDataDir(),
        name='fast',
        traffic_setting=traffic_setting_module.WIFI)
    story_set.AddStory(slow_page)
    story_set.AddStory(fast_page)

    latencies_by_page_in_ms = {}

    class MeasureLatency(legacy_page_test.LegacyPageTest):
      def __init__(self):
        super(MeasureLatency, self).__init__()
        self._will_navigate_time = None

      def WillNavigateToPage(self, page, tab):
        del page, tab # unused
        self._will_navigate_time = time.time() * 1000

      def ValidateAndMeasurePage(self, page, tab, results):
        del results  # unused
        latencies_by_page_in_ms[page.name] = (
            time.time() * 1000 - self._will_navigate_time)

    test = MeasureLatency()
    options = options_for_unittests.GetCopy()
    options.output_formats = ['none']
    options.suppress_gtest_report = True

    with tempfile_ext.NamedTemporaryDirectory('page_E2E_tests') as tempdir:
      options.output_dir = tempdir
      SetUpStoryRunnerArguments(options)
      results = results_options.CreateResults(EmptyMetadataForTest(), options)
      story_runner.Run(test, story_set, options, results)
      failure_messages = []
      for r in results.all_page_runs:
        if r.failure_str:
          failure_messages.append(
              'Failure message of story %s:\n%s' % (r.story, r.failure_str))
      self.assertFalse(results.had_failures, msg=''.join(failure_messages))
      self.assertIn('slow', latencies_by_page_in_ms)
      self.assertIn('fast', latencies_by_page_in_ms)
      # Slow page should be slower than fast page by at least 40 ms (roundtrip
      # time of good 3G) - 2 ms (roundtrip time of Wifi)
      self.assertGreater(latencies_by_page_in_ms['slow'],
                         latencies_by_page_in_ms['fast'] + 40 - 2)

  # Ensure that story_runner allows the test to customize the browser
  # before it launches.
  def testBrowserBeforeLaunch(self):
    story_set = story.StorySet()
    page = page_module.Page(
        'file://blank.html', story_set, base_dir=util.GetUnittestDataDir(),
        name='blank.html')
    story_set.AddStory(page)

    class TestBeforeLaunch(legacy_page_test.LegacyPageTest):

      def __init__(self):
        super(TestBeforeLaunch, self).__init__()
        self._did_call_will_start = False
        self._did_call_did_start = False

      def WillStartBrowser(self, platform):
        self._did_call_will_start = True
        # TODO(simonjam): Test that the profile is available.

      def DidStartBrowser(self, browser):
        assert self._did_call_will_start
        self._did_call_did_start = True

      def ValidateAndMeasurePage(self, *_):
        assert self._did_call_did_start

    test = TestBeforeLaunch()
    options = options_for_unittests.GetCopy()
    options.output_formats = ['none']
    options.suppress_gtest_report = True

    with tempfile_ext.NamedTemporaryDirectory('page_E2E_tests') as tempdir:
      options.output_dir = tempdir
      SetUpStoryRunnerArguments(options)
      results = results_options.CreateResults(EmptyMetadataForTest(), options)
      story_runner.Run(test, story_set, options, results)

  def testRunPageWithStartupUrl(self):
    num_times_browser_closed = [0]

    class TestSharedState(shared_page_state.SharedPageState):

      def _StopBrowser(self):
        if self._browser:
          num_times_browser_closed[0] += 1
        super(TestSharedState, self)._StopBrowser()

    story_set = story.StorySet()
    page = page_module.Page(
        'file://blank.html', story_set, base_dir=util.GetUnittestDataDir(),
        startup_url='about:blank', shared_page_state_class=TestSharedState,
        name='blank.html')
    story_set.AddStory(page)

    class Measurement(legacy_page_test.LegacyPageTest):

      def __init__(self):
        super(Measurement, self).__init__()

      def ValidateAndMeasurePage(self, page, tab, results):
        del page, tab, results  # not used

    options = options_for_unittests.GetCopy()
    options.pageset_repeat = 2
    options.output_formats = ['none']
    options.suppress_gtest_report = True
    if not browser_finder.FindBrowser(options):
      return

    with tempfile_ext.NamedTemporaryDirectory('page_E2E_tests') as tempdir:
      options.output_dir = tempdir
      test = Measurement()
      SetUpStoryRunnerArguments(options)
      results = results_options.CreateResults(EmptyMetadataForTest(), options)
      story_runner.Run(test, story_set, options, results)
      self.assertEquals('about:blank', options.browser_options.startup_url)
      # _StopBrowser should be called 2 times:
      # 1. browser restarts after page 1 run
      # 2. in the TearDownState after all the pages have run.
      self.assertEquals(num_times_browser_closed[0], 2)

  # Ensure that story_runner calls cleanUp when a page run fails.
  def testCleanUpPage(self):
    story_set = story.StorySet()
    page = page_module.Page(
        'file://blank.html', story_set, base_dir=util.GetUnittestDataDir(),
        name='blank.html')
    story_set.AddStory(page)

    class Test(legacy_page_test.LegacyPageTest):

      def __init__(self):
        super(Test, self).__init__()
        self.did_call_clean_up = False

      def ValidateAndMeasurePage(self, *_):
        raise legacy_page_test.Failure

      def DidRunPage(self, platform):
        del platform  # unused
        self.did_call_clean_up = True

    test = Test()
    options = options_for_unittests.GetCopy()
    options.output_formats = ['none']
    options.suppress_gtest_report = True

    with tempfile_ext.NamedTemporaryDirectory('page_E2E_tests') as tempdir:
      options.output_dir = tempdir
      SetUpStoryRunnerArguments(options)
      results = results_options.CreateResults(EmptyMetadataForTest(), options)
      story_runner.Run(test, story_set, options, results)
      assert test.did_call_clean_up

  # Ensure skipping the test if shared state cannot be run on the browser.
  def testSharedPageStateCannotRunOnBrowser(self):
    story_set = story.StorySet()

    class UnrunnableSharedState(shared_page_state.SharedPageState):
      def CanRunOnBrowser(self, browser_info, page):
        del browser_info, page  # unused
        return False

      def ValidateAndMeasurePage(self, _):
        pass

    story_set.AddStory(page_module.Page(
        url='file://blank.html', page_set=story_set,
        base_dir=util.GetUnittestDataDir(),
        shared_page_state_class=UnrunnableSharedState,
        name='blank.html'))

    class Test(legacy_page_test.LegacyPageTest):

      def __init__(self, *args, **kwargs):
        super(Test, self).__init__(*args, **kwargs)
        self.will_navigate_to_page_called = False

      def ValidateAndMeasurePage(self, *args):
        del args  # unused
        raise Exception('Exception should not be thrown')

      def WillNavigateToPage(self, page, tab):
        del page, tab  # unused
        self.will_navigate_to_page_called = True

    test = Test()
    options = options_for_unittests.GetCopy()
    options.output_formats = ['none']
    options.suppress_gtest_report = True
    SetUpStoryRunnerArguments(options)
    results = results_options.CreateResults(EmptyMetadataForTest(), options)
    story_runner.Run(test, story_set, options, results)
    self.assertFalse(test.will_navigate_to_page_called)
    self.assertEquals(1, len(GetSuccessfulPageRuns(results)))
    self.assertEquals(1, len(results.skipped_values))
    self.assertFalse(results.had_failures)

  def _RunPageTestThatRaisesAppCrashException(self, test, max_failures):
    class TestPage(page_module.Page):

      def RunNavigateSteps(self, _):
        raise exceptions.AppCrashException

    story_set = story.StorySet()
    for i in range(5):
      story_set.AddStory(
          TestPage('file://blank.html', story_set,
                   base_dir=util.GetUnittestDataDir(), name='foo%d' % i))
    options = options_for_unittests.GetCopy()
    options.output_formats = ['none']
    options.suppress_gtest_report = True
    SetUpStoryRunnerArguments(options)
    results = results_options.CreateResults(EmptyMetadataForTest(), options)
    story_runner.Run(test, story_set, options, results,
                     max_failures=max_failures)
    return results

  def testSingleTabMeansCrashWillCauseFailure(self):
    self.CaptureFormattedException()

    class SingleTabTest(legacy_page_test.LegacyPageTest):

      def ValidateAndMeasurePage(self, *_):
        pass

    test = SingleTabTest()
    results = self._RunPageTestThatRaisesAppCrashException(
        test, max_failures=1)
    self.assertEquals([], GetSuccessfulPageRuns(results))
    self.assertEquals(2, results.num_failed)  # max_failures + 1
    self.assertFormattedExceptionIsEmpty()

  def testWebPageReplay(self):
    story_set = example_domain.ExampleDomainPageSet()
    body = []

    class TestWpr(legacy_page_test.LegacyPageTest):
      def ValidateAndMeasurePage(self, page, tab, results):
        del page, results  # unused
        body.append(tab.EvaluateJavaScript('document.body.innerText'))

      def DidRunPage(self, platform):
        # Force the replay server to restart between pages; this verifies that
        # the restart mechanism works.
        platform.network_controller.StopReplay()

    test = TestWpr()
    options = options_for_unittests.GetCopy()
    options.output_formats = ['none']
    options.suppress_gtest_report = True
    SetUpStoryRunnerArguments(options)
    results = results_options.CreateResults(EmptyMetadataForTest(), options)

    story_runner.Run(test, story_set, options, results)

    self.longMessage = True
    self.assertIn('Example Domain', body[0],
                  msg='URL: %s' % story_set.stories[0].url)
    self.assertIn('Example Domain', body[1],
                  msg='URL: %s' % story_set.stories[1].url)

    self.assertEquals(2, len(GetSuccessfulPageRuns(results)))
    self.assertFalse(results.had_failures)

  def testScreenShotTakenForFailedPage(self):
    self.CaptureFormattedException()
    platform_screenshot_supported = [False]
    tab_screenshot_supported = [False]
    chrome_version_screen_shot = [None]

    class FailingTestPage(page_module.Page):

      def RunNavigateSteps(self, action_runner):
        action_runner.Navigate(self._url)
        platform_screenshot_supported[0] = (
            action_runner.tab.browser.platform.CanTakeScreenshot)
        tab_screenshot_supported[0] = action_runner.tab.screenshot_supported
        if not platform_screenshot_supported[0] and tab_screenshot_supported[0]:
          chrome_version_screen_shot[0] = action_runner.tab.Screenshot()
        raise exceptions.AppCrashException

    story_set = story.StorySet()
    story_set.AddStory(page_module.Page('file://blank.html', story_set,
                                        name='blank.html'))
    failing_page = FailingTestPage('chrome://version', story_set,
                                   name='failing')
    story_set.AddStory(failing_page)
    with tempfile_ext.NamedTemporaryDirectory() as tempdir:
      options = options_for_unittests.GetCopy()
      options.output_dir = tempdir
      options.output_formats = ['none']
      options.browser_options.take_screenshot_for_failed_page = True
      options.suppress_gtest_report = True
      SetUpStoryRunnerArguments(options)
      results = results_options.CreateResults(EmptyMetadataForTest(), options)
      story_runner.Run(DummyTest(), story_set, options, results,
                       max_failures=2)
      self.assertTrue(results.had_failures)
      if not platform_screenshot_supported[0] and tab_screenshot_supported[0]:
        artifacts = results._artifact_results.GetTestArtifacts(
            failing_page.name)
        self.assertIsNotNone(artifacts)
        self.assertIn('screenshot', artifacts)

        screenshot_file_path = os.path.join(tempdir, artifacts['screenshot'][0])

        actual_screenshot = image_util.FromPngFile(screenshot_file_path)
        self.assertEquals(image_util.Pixels(chrome_version_screen_shot[0]),
                          image_util.Pixels(actual_screenshot))


class FakePageRunEndToEndTests(unittest.TestCase):

  def setUp(self):
    self.options = fakes.CreateBrowserFinderOptions()
    self.options.output_formats = ['none']
    self.options.suppress_gtest_report = True
    SetUpStoryRunnerArguments(self.options)

  def testNoScreenShotTakenForFailedPageDueToNoSupport(self):
    self.options.browser_options.take_screenshot_for_failed_page = True

    class FailingTestPage(page_module.Page):

      def RunNavigateSteps(self, action_runner):
        raise exceptions.AppCrashException

    story_set = story.StorySet()
    story_set.AddStory(page_module.Page('file://blank.html', story_set,
                                        name='blank.html'))
    failing_page = FailingTestPage('chrome://version', story_set,
                                   name='failing')
    story_set.AddStory(failing_page)
    results = results_options.CreateResults(
        EmptyMetadataForTest(), self.options)
    story_runner.Run(DummyTest(), story_set, self.options, results,
                     max_failures=2)
    self.assertTrue(results.had_failures)

  def testScreenShotTakenForFailedPageOnSupportedPlatform(self):
    fake_platform = self.options.fake_possible_browser.returned_browser.platform
    expected_png_base64 = """
 iVBORw0KGgoAAAANSUhEUgAAAAIAAAACCAIAAAD91
 JpzAAAAFklEQVR4Xg3EAQ0AAABAMP1LY3YI7l8l6A
 T8tgwbJAAAAABJRU5ErkJggg==
"""
    fake_platform.screenshot_png_data = expected_png_base64
    self.options.browser_options.take_screenshot_for_failed_page = True

    class FailingTestPage(page_module.Page):

      def RunNavigateSteps(self, action_runner):
        raise exceptions.AppCrashException
    story_set = story.StorySet()
    story_set.AddStory(page_module.Page('file://blank.html', story_set,
                                        name='blank.html'))
    failing_page = FailingTestPage('chrome://version', story_set,
                                   name='failing')
    story_set.AddStory(failing_page)

    self.options.output_formats = ['json-test-results']
    with tempfile_ext.NamedTemporaryDirectory() as tempdir:
      self.options.output_dir = tempdir
      results = results_options.CreateResults(
          EmptyMetadataForTest(), self.options)

      # This ensures the output stream used by the json test results object is
      # closed. On windows, we can't delete the temp directory if a file in that
      # directory is still in use.
      with results:
        story_runner.Run(DummyTest(), story_set, self.options, results,
                         max_failures=2)
        self.assertTrue(results.had_failures)
        artifacts = results._artifact_results.GetTestArtifacts(
            failing_page.name)
        self.assertIsNotNone(artifacts)
        self.assertIn('screenshot', artifacts)

        screenshot_file_path = os.path.join(tempdir, artifacts['screenshot'][0])

        actual_screenshot_img = image_util.FromPngFile(screenshot_file_path)
        self.assertTrue(
            image_util.AreEqual(
                image_util.FromBase64Png(expected_png_base64),
                actual_screenshot_img))
