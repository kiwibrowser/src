# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import py_utils
from telemetry import story as story_module
from telemetry.page import page as page_module
from telemetry.page import shared_page_state

class LeakDetectionSharedState(shared_page_state.SharedDesktopPageState):
  def ShouldStopBrowserAfterStoryRun(self, story):
    del story # unused
    return False  # Keep the same browser instance open across stories.


class LeakDetectionPage(page_module.Page):
  def __init__(self, url, page_set, name=''):
    super(LeakDetectionPage, self).__init__(
      url=url, page_set=page_set, name=name,
      shared_page_state_class=LeakDetectionSharedState)

  def RunNavigateSteps(self, action_runner):
    tabs = action_runner.tab.browser.tabs
    new_tab = tabs.New()
    new_tab.action_runner.Navigate('about:blank')
    new_tab.action_runner.PrepareForLeakDetection()
    new_tab.action_runner.MeasureMemory()
    new_tab.action_runner.Navigate(self.url)
    self._WaitForPageLoadToComplete(new_tab.action_runner)
    new_tab.action_runner.Navigate('about:blank')
    new_tab.action_runner.PrepareForLeakDetection()
    new_tab.action_runner.MeasureMemory()
    new_tab.Close()

  def _WaitForPageLoadToComplete(self, action_runner):
    py_utils.WaitFor(action_runner.tab.HasReachedQuiescence, timeout=30)


# Some websites have a script that loads resources continuously, in which cases
# HasReachedQuiescence would not be reached. This class waits for document ready
# state to be complete to avoid timeout for those pages.
class ResourceLoadingLeakDetectionPage(LeakDetectionPage):
  def _WaitForPageLoadToComplete(self, action_runner):
    action_runner.tab.WaitForDocumentReadyStateToBeComplete()


class LeakDetectionStorySet(story_module.StorySet):
  def __init__(self):
    super(LeakDetectionStorySet, self).__init__(
      archive_data_file='data/leak_detection.json',
      cloud_storage_bucket=story_module.PARTNER_BUCKET)
    urls_list = [
      # Alexa top websites
      'https://www.google.com',
      'https://www.youtube.com',
      'https://www.facebook.com',
      'https://www.baidu.com',
      'https://www.wikipedia.org',
      'http://www.qq.com',
      'http://www.amazon.com',
      'http://www.twitter.com',
      # websites which were found to be leaking in the past
      'https://www.prezi.com',
      'http://www.time.com',
      'http://www.cheapoair.com',
      'http://www.onlinedown.net',
      'http://www.dailypost.ng',
      'http://www.aljazeera.net',
    ]
    resource_loading_urls_list = [
      'https://www.yahoo.com',
      'http://www.quora.com',
      'https://www.macys.com',
      'http://infomoney.com.br',
      'http://www.listindiario.com',
    ]
    for url in urls_list:
      self.AddStory(LeakDetectionPage(url, self, url))
    for url in resource_loading_urls_list:
      self.AddStory(ResourceLoadingLeakDetectionPage(url, self, url))
