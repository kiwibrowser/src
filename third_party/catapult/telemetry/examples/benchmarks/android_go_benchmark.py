# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import logging

from telemetry.core import android_platform
from telemetry.internal.browser import browser_finder
from telemetry.timeline import chrome_trace_category_filter
from telemetry.util import wpr_modes
from telemetry.web_perf import timeline_based_measurement
from telemetry import benchmark
from telemetry import story as story_module

from devil.android.sdk import intent


class SharedAndroidStoryState(story_module.SharedState):

  def __init__(self, test, finder_options, story_set):
    """
    Args:
      test: (unused)
      finder_options: A finder_options object
      story_set: (unused)
    """
    super(SharedAndroidStoryState, self).__init__(
        test, finder_options, story_set)
    self._finder_options = finder_options
    self._possible_browser = browser_finder.FindBrowser(self._finder_options)
    self._browser = None
    self._current_story = None

    # This is an Android-only shared state.
    assert isinstance(self.platform, android_platform.AndroidPlatform)
    self._finder_options.browser_options.browser_user_agent_type = 'mobile'

    # TODO: This will always use live sites. Should use options to configure
    # network_controller properly. See e.g.: https://goo.gl/nAsyFr
    self.platform.network_controller.Open(wpr_modes.WPR_OFF)
    self.platform.Initialize()

  @property
  def platform(self):
    return self._possible_browser.platform

  def TearDownState(self):
    self.platform.network_controller.Close()

  def LaunchBrowser(self, url):
    # TODO: Android Go stories could, e.g., use the customtabs helper app to
    # start Chrome as a custom tab.
    self.platform.StartActivity(
        intent.Intent(package=self._possible_browser.settings.package,
                      activity=self._possible_browser.settings.activity,
                      action=None, data=url),
        blocking=True)

  def FindBrowser(self):
    assert self._browser is None, 'Browser was already found'
    self._browser = self._possible_browser.FindExistingBrowser()
    return self._browser

  def WillRunStory(self, story):
    # TODO: Should start replay to use WPR recordings.
    # See e.g.: https://goo.gl/UJuu8a
    self._possible_browser.SetUpEnvironment(
        self._finder_options.browser_options)
    self._current_story = story

  def RunStory(self, _):
    self._current_story.Run(self)

  def DidRunStory(self, _):
    self._current_story = None
    try:
      self._browser.Close()
    finally:
      self._browser = None
      self._possible_browser.CleanUpEnvironment()

  def DumpStateUponFailure(self, story, results):
    del story
    del results
    if self._browser is not None:
      self._browser.DumpStateUponFailure()
    else:
      logging.warning('Cannot dump browser state: No browser.')
    # TODO: Should also capture screenshot, etc., like shared_page_state does.
    # Ideally common code should be factored out. See: https://goo.gl/RUwgGW

  def CanRunStory(self, _):
    return True


class AndroidGoFooStory(story_module.Story):
  def __init__(self):
    super(AndroidGoFooStory, self).__init__(
        SharedAndroidStoryState, name='go:story:foo')

  def Run(self, state):
    state.LaunchBrowser('https://en.wikipedia.org/wiki/Main_Page')
    browser = state.FindBrowser()
    action_runner = browser.foreground_tab.action_runner
    action_runner.tab.WaitForDocumentReadyStateToBeComplete()
    action_runner.RepeatableBrowserDrivenScroll(repeat_count=2)


class AndroidGoBarStory(story_module.Story):
  def __init__(self):
    super(AndroidGoBarStory, self).__init__(
        SharedAndroidStoryState, name='go:story:bar')

  def Run(self, state):
    state.LaunchBrowser('http://www.bbc.co.uk/news')
    browser = state.FindBrowser()
    action_runner = browser.foreground_tab.action_runner
    action_runner.tab.WaitForDocumentReadyStateToBeComplete()
    action_runner.RepeatableBrowserDrivenScroll(repeat_count=2)


class AndroidGoStories(story_module.StorySet):
  def __init__(self):
    super(AndroidGoStories, self).__init__()
    self.AddStory(AndroidGoFooStory())
    self.AddStory(AndroidGoBarStory())


class AndroidGoBenchmark(benchmark.Benchmark):
  def CreateCoreTimelineBasedMeasurementOptions(self):
    cat_filter = chrome_trace_category_filter.ChromeTraceCategoryFilter(
        filter_string='rail,toplevel')

    options = timeline_based_measurement.Options(cat_filter)
    options.config.enable_chrome_trace = True
    options.SetTimelineBasedMetrics([
        'clockSyncLatencyMetric',
        'tracingMetric',
    ])
    return options

  def CreateStorySet(self, options):
    return AndroidGoStories()

  @classmethod
  def Name(cls):
    return 'android_go.example'
