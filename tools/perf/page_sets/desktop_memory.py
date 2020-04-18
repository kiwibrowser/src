# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import logging
import py_utils

from telemetry.page import page as page_module
from telemetry.page import shared_page_state
from telemetry import story as story_module

_DUMP_WAIT_TIME = 3
_ITERATIONS = 10


class DesktopMemorySharedState(shared_page_state.SharedDesktopPageState):
  def ShouldStopBrowserAfterStoryRun(self, story):
    del story
    return False  # Keep the same browser instance open across stories.


class DesktopMemoryPage(page_module.Page):

  def __init__(self, url, page_set):
    super(DesktopMemoryPage, self).__init__(
        url=url, page_set=page_set,
        shared_page_state_class=DesktopMemorySharedState,
        name=url)

  def _DumpMemory(self, action_runner, phase):
    with action_runner.CreateInteraction(phase):
      action_runner.Wait(_DUMP_WAIT_TIME)
      action_runner.ForceGarbageCollection()
      action_runner.SimulateMemoryPressureNotification('critical')
      action_runner.Wait(_DUMP_WAIT_TIME)
      action_runner.tab.browser.DumpMemory()

  def RunPageInteractions(self, action_runner):
    self._DumpMemory(action_runner, 'pre')
    for _ in xrange(_ITERATIONS):
      action_runner.ReloadPage()

    tabs = action_runner.tab.browser.tabs
    for _ in xrange(_ITERATIONS):
      new_tab = tabs.New()
      new_tab.action_runner.Navigate(self._url)
      try:
        new_tab.action_runner.WaitForNetworkQuiescence()
      except py_utils.TimeoutException:
        logging.warning('WaitForNetworkQuiescence() timeout')
      new_tab.Close()

    self._DumpMemory(action_runner, 'post')


class DesktopMemoryPageSet(story_module.StorySet):

  """ Desktop sites with interesting memory characteristics """

  def __init__(self):
    super(DesktopMemoryPageSet, self).__init__()

    urls_list = [
      'http://www.google.com',
      "http://www.live.com",
      "http://www.youtube.com",
      "http://www.wikipedia.org",
      "http://www.flickr.com/",
      "http://www.cnn.com/",
      "http://www.adobe.com/",
      "http://www.aol.com/",
      "http://www.cnet.com/",
      "http://www.godaddy.com/",
      "http://www.walmart.com/",
      "http://www.skype.com/",
    ]

    for url in urls_list:
      self.AddStory(DesktopMemoryPage(url, self))
