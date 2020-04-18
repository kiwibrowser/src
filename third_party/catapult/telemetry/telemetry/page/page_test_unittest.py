# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry.page import page as page_module
from telemetry.page import legacy_page_test
from telemetry.testing import options_for_unittests
from telemetry.testing import page_test_test_case
from telemetry.util import wpr_modes


class PageTestThatFails(legacy_page_test.LegacyPageTest):

  def ValidateAndMeasurePage(self, page, tab, results):
    raise legacy_page_test.Failure


class PageTestForBlank(legacy_page_test.LegacyPageTest):

  def ValidateAndMeasurePage(self, page, tab, results):
    contents = tab.EvaluateJavaScript('document.body.textContent')
    if contents.strip() != 'Hello world':
      raise legacy_page_test.MeasurementFailure(
          'Page contents were: ' + contents)


class PageTestForReplay(legacy_page_test.LegacyPageTest):

  def ValidateAndMeasurePage(self, page, tab, results):
    # Web Page Replay returns '404 Not found' if a page is not in the archive.
    contents = tab.EvaluateJavaScript('document.body.textContent')
    if '404 Not Found' in contents.strip():
      raise legacy_page_test.MeasurementFailure('Page not in archive.')


class PageTestQueryParams(legacy_page_test.LegacyPageTest):

  def ValidateAndMeasurePage(self, page, tab, results):
    query = tab.EvaluateJavaScript('window.location.search')
    expected = '?foo=1'
    if query.strip() != expected:
      raise legacy_page_test.MeasurementFailure(
          'query was %s, not %s.' % (query, expected))


class PageTestWithAction(legacy_page_test.LegacyPageTest):

  def __init__(self):
    super(PageTestWithAction, self).__init__()

  def ValidateAndMeasurePage(self, page, tab, results):
    pass


class PageWithAction(page_module.Page):

  def __init__(self, url, story_set):
    super(PageWithAction, self).__init__(url, story_set, story_set.base_dir,
                                         name=url)
    self.run_test_action_called = False

  def RunPageInteractions(self, _):
    self.run_test_action_called = True


class PageTestUnitTest(page_test_test_case.PageTestTestCase):

  def setUp(self):
    self._options = options_for_unittests.GetCopy()
    self._options.browser_options.wpr_mode = wpr_modes.WPR_OFF

  def testGotToBlank(self):
    story_set = self.CreateStorySetFromFileInUnittestDataDir('blank.html')
    measurement = PageTestForBlank()
    all_results = self.RunMeasurement(
        measurement, story_set, options=self._options)
    self.assertFalse(all_results.had_failures)

  def testGotQueryParams(self):
    story_set = self.CreateStorySetFromFileInUnittestDataDir(
        'blank.html?foo=1')
    measurement = PageTestQueryParams()
    all_results = self.RunMeasurement(
        measurement, story_set, options=self._options)
    self.assertFalse(all_results.had_failures)

  def testFailure(self):
    story_set = self.CreateStorySetFromFileInUnittestDataDir('blank.html')
    measurement = PageTestThatFails()
    all_results = self.RunMeasurement(
        measurement, story_set, options=self._options)
    self.assertTrue(all_results.had_failures)

  def testRunActions(self):
    story_set = self.CreateEmptyPageSet()
    page = PageWithAction('file://blank.html', story_set)
    story_set.AddStory(page)
    measurement = PageTestWithAction()
    self.RunMeasurement(measurement, story_set, options=self._options)
    self.assertTrue(page.run_test_action_called)
