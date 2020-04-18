# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import contextlib

from telemetry import decorators
from telemetry import page as page_module
from telemetry import story
from telemetry.internal.testing.test_page_sets import example_domain
from telemetry.page import cache_temperature
from telemetry.testing import browser_test_case
from telemetry.timeline import tracing_config
from tracing.trace_data import trace_data


_TEST_URL = example_domain.HTTP_EXAMPLE


class CacheTemperatureTests(browser_test_case.BrowserTestCase):
  def __init__(self, *args, **kwargs):
    super(CacheTemperatureTests, self).__init__(*args, **kwargs)
    self._full_trace = None

  def setUp(self):
    super(CacheTemperatureTests, self).setUp()
    self._browser.platform.network_controller.StartReplay(
        example_domain.FetchExampleDomainArchive())

  def tearDown(self):
    super(CacheTemperatureTests, self).tearDown()
    self._browser.platform.network_controller.StopReplay()

  @contextlib.contextmanager
  def CaptureTrace(self):
    tracing_controller = self._browser.platform.tracing_controller
    options = tracing_config.TracingConfig()
    options.enable_chrome_trace = True
    tracing_controller.StartTracing(options)
    try:
      yield
    finally:
      self._full_trace = tracing_controller.StopTracing()[0]

  def CollectTraceMarkers(self):
    if not self._full_trace:
      return set()

    chrome_trace = self._full_trace.GetTraceFor(trace_data.CHROME_TRACE_PART)
    return set(
        event['name']
        for event in chrome_trace['traceEvents']
        if event['cat'] == 'blink.console')

  @decorators.Enabled('has tabs')
  @decorators.Disabled('chromeos')  # crbug.com/840033
  def testEnsureAny(self):
    with self.CaptureTrace():
      story_set = story.StorySet()
      page = page_module.Page(
          _TEST_URL, page_set=story_set,
          cache_temperature=cache_temperature.ANY, name=_TEST_URL)
      cache_temperature.EnsurePageCacheTemperature(page, self._browser)

    markers = self.CollectTraceMarkers()
    self.assertNotIn('telemetry.internal.ensure_diskcache.start', markers)
    self.assertNotIn('telemetry.internal.warm_cache.warm.start', markers)
    self.assertNotIn('telemetry.internal.warm_cache.warm.end', markers)
    self.assertNotIn('telemetry.internal.warm_cache.hot.start', markers)
    self.assertNotIn('telemetry.internal.warm_cache.hot.end', markers)

  @decorators.Enabled('has tabs')
  @decorators.Disabled('chromeos')  # crbug.com/840033
  def testEnsureCold(self):
    with self.CaptureTrace():
      story_set = story.StorySet()
      page = page_module.Page(
          _TEST_URL, page_set=story_set,
          cache_temperature=cache_temperature.COLD, name=_TEST_URL)
      cache_temperature.EnsurePageCacheTemperature(page, self._browser)

    markers = self.CollectTraceMarkers()
    self.assertIn('telemetry.internal.ensure_diskcache.start', markers)
    self.assertIn('telemetry.internal.ensure_diskcache.end', markers)

  @decorators.Disabled('reference')
  @decorators.Enabled('has tabs')
  @decorators.Disabled('chromeos')  # crbug.com/840033
  def testEnsureWarmAfterColdRun(self):
    with self.CaptureTrace():
      story_set = story.StorySet()
      page = page_module.Page(
          _TEST_URL, page_set=story_set,
          cache_temperature=cache_temperature.COLD, name=_TEST_URL)
      cache_temperature.EnsurePageCacheTemperature(page, self._browser)

      previous_page = page
      page = page_module.Page(
          _TEST_URL, page_set=story_set,
          cache_temperature=cache_temperature.WARM, name=_TEST_URL)
      cache_temperature.EnsurePageCacheTemperature(
          page, self._browser, previous_page)

    markers = self.CollectTraceMarkers()
    self.assertNotIn('telemetry.internal.warm_cache.warm.start', markers)
    self.assertNotIn('telemetry.internal.warm_cache.warm.end', markers)
    self.assertNotIn('telemetry.internal.warm_cache.hot.start', markers)
    self.assertNotIn('telemetry.internal.warm_cache.hot.end', markers)

  @decorators.Disabled('reference')
  @decorators.Enabled('has tabs')
  @decorators.Disabled('chromeos')  # crbug.com/840033
  def testEnsureWarmFromScratch(self):
    with self.CaptureTrace():
      story_set = story.StorySet()
      page = page_module.Page(
          _TEST_URL, page_set=story_set,
          cache_temperature=cache_temperature.WARM, name=_TEST_URL)
      cache_temperature.EnsurePageCacheTemperature(page, self._browser)

    markers = self.CollectTraceMarkers()
    self.assertIn('telemetry.internal.warm_cache.warm.start', markers)
    self.assertIn('telemetry.internal.warm_cache.warm.end', markers)

  @decorators.Disabled('reference')
  @decorators.Enabled('has tabs')
  @decorators.Disabled('chromeos')  # crbug.com/840033
  def testEnsureHotAfterColdAndWarmRun(self):
    with self.CaptureTrace():
      story_set = story.StorySet()
      page = page_module.Page(
          _TEST_URL, page_set=story_set,
          cache_temperature=cache_temperature.COLD, name=_TEST_URL)
      cache_temperature.EnsurePageCacheTemperature(page, self._browser)

      previous_page = page
      page = page_module.Page(
          _TEST_URL, page_set=story_set,
          cache_temperature=cache_temperature.WARM, name=_TEST_URL)
      cache_temperature.EnsurePageCacheTemperature(
          page, self._browser, previous_page)

      previous_page = page
      page = page_module.Page(
          _TEST_URL, page_set=story_set,
          cache_temperature=cache_temperature.HOT, name=_TEST_URL)
      cache_temperature.EnsurePageCacheTemperature(
          page, self._browser, previous_page)

    markers = self.CollectTraceMarkers()
    self.assertNotIn('telemetry.internal.warm_cache.warm.start', markers)
    self.assertNotIn('telemetry.internal.warm_cache.warm.end', markers)
    self.assertNotIn('telemetry.internal.warm_cache.hot.start', markers)
    self.assertNotIn('telemetry.internal.warm_cache.hot.end', markers)

  @decorators.Disabled('reference')
  @decorators.Disabled('chromeos')  # crbug.com/840033
  def testEnsureHotAfterColdRun(self):
    with self.CaptureTrace():
      story_set = story.StorySet()
      page = page_module.Page(
          _TEST_URL, page_set=story_set,
          cache_temperature=cache_temperature.COLD, name=_TEST_URL)
      cache_temperature.EnsurePageCacheTemperature(page, self._browser)

      previous_page = page
      page = page_module.Page(
          _TEST_URL, page_set=story_set,
          cache_temperature=cache_temperature.HOT, name=_TEST_URL)
      cache_temperature.EnsurePageCacheTemperature(
          page, self._browser, previous_page)

    markers = self.CollectTraceMarkers()
    # After navigation for another origin url, traces in previous origin page
    # does not appear in |markers|, so we can not check this:
    # self.assertIn('telemetry.internal.warm_cache.hot.start', markers)
    # TODO: Ensure all traces are in |markers|
    self.assertIn('telemetry.internal.warm_cache.hot.end', markers)

  @decorators.Disabled('reference')
  @decorators.Enabled('has tabs')
  @decorators.Disabled('chromeos')  # crbug.com/840033
  def testEnsureHotFromScratch(self):
    with self.CaptureTrace():
      story_set = story.StorySet()
      page = page_module.Page(
          _TEST_URL, page_set=story_set,
          cache_temperature=cache_temperature.HOT, name=_TEST_URL)
      cache_temperature.EnsurePageCacheTemperature(page, self._browser)

    markers = self.CollectTraceMarkers()
    self.assertIn('telemetry.internal.warm_cache.warm.start', markers)
    self.assertIn('telemetry.internal.warm_cache.warm.end', markers)
    self.assertIn('telemetry.internal.warm_cache.hot.start', markers)
    self.assertIn('telemetry.internal.warm_cache.hot.end', markers)

  @decorators.Disabled('reference')
  @decorators.Enabled('has tabs')
  @decorators.Disabled('chromeos')  # crbug.com/840033
  def testEnsureWarmBrowser(self):
    with self.CaptureTrace():
      story_set = story.StorySet()
      page = page_module.Page(
          _TEST_URL, page_set=story_set,
          cache_temperature=cache_temperature.WARM_BROWSER,
          name=_TEST_URL)
      cache_temperature.EnsurePageCacheTemperature(
          page, self._browser)

    markers = self.CollectTraceMarkers()
    # Browser cache warming happens in a different tab so markers shouldn't
    # appear.
    self.assertNotIn('telemetry.internal.warm_cache.warm.start', markers)
    self.assertNotIn('telemetry.internal.warm_cache.warm.end', markers)
    self.assertNotIn('telemetry.internal.warm_cache.hot.start', markers)
    self.assertNotIn('telemetry.internal.warm_cache.hot.end', markers)

  @decorators.Disabled('reference')
  @decorators.Enabled('has tabs')
  @decorators.Disabled('chromeos')  # crbug.com/840033
  def testEnsureHotBrowser(self):
    with self.CaptureTrace():
      story_set = story.StorySet()
      page = page_module.Page(
          _TEST_URL, page_set=story_set,
          cache_temperature=cache_temperature.HOT_BROWSER,
          name=_TEST_URL)
      cache_temperature.EnsurePageCacheTemperature(
          page, self._browser)

    markers = self.CollectTraceMarkers()
    # Browser cache warming happens in a different tab so markers shouldn't
    # appear.
    self.assertNotIn('telemetry.internal.warm_cache.warm.start', markers)
    self.assertNotIn('telemetry.internal.warm_cache.warm.end', markers)
    self.assertNotIn('telemetry.internal.warm_cache.hot.start', markers)
    self.assertNotIn('telemetry.internal.warm_cache.hot.end', markers)
