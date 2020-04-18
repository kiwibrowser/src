# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import utils

from telemetry import page
from telemetry import story
from benchmarks.pagesets import media_router_page
from telemetry.core import exceptions
from telemetry.page import shared_page_state
from telemetry.util import js_template


SESSION_TIME = 300  # 5 minutes


class SharedState(shared_page_state.SharedPageState):
  """Shared state that restarts the browser for every single story."""

  def __init__(self, test, finder_options, story_set):
    super(SharedState, self).__init__(
        test, finder_options, story_set)

  def DidRunStory(self, results):
    super(SharedState, self).DidRunStory(results)
    self._StopBrowser()


class CastDialogPage(media_router_page.CastPage):
  """Cast page to open a cast-enabled page and open media router dialog."""

  def __init__(self, page_set, url='file://basic_test.html',
               shared_page_state_class=shared_page_state.SharedPageState,
               name='basic_test.html'):
    super(CastDialogPage, self).__init__(
        url=url, page_set=page_set,
        shared_page_state_class=shared_page_state_class, name=name)

  def RunPageInteractions(self, action_runner):
    # Wait for 5s after Chrome is opened in order to get consistent results.
    action_runner.Wait(5)
    with action_runner.CreateInteraction('LaunchDialog'):
      # Open dialog
      action_runner.TapElement(selector='#start_session_button')
      action_runner.Wait(5)
      # Close media router dialog
      for tab in action_runner.tab.browser.tabs:
        if tab.url == 'chrome://media-router/':
          self.CloseDialog(tab)


class CastIdlePage(CastDialogPage):
  """Cast page to open a cast-enabled page and do nothing."""

  def __init__(self, page_set):
    super(CastIdlePage, self).__init__(
        page_set=page_set,
        url='file://basic_test.html',
        shared_page_state_class=SharedState,
        name='basic_test.html')

  def RunPageInteractions(self, action_runner):
    # Wait for 5s after Chrome is opened in order to get consistent results.
    action_runner.Wait(5)
    with action_runner.CreateInteraction('Idle'):
      action_runner.ExecuteJavaScript('collectPerfData();')
      action_runner.Wait(SESSION_TIME)


class CastFlingingPage(media_router_page.CastPage):
  """Cast page to fling a video to Chromecast device."""

  def __init__(self, page_set):
    super(CastFlingingPage, self).__init__(
        page_set=page_set,
        url='file://basic_test.html#flinging',
        shared_page_state_class=SharedState,
        name='basic_test.html#flinging')

  def RunPageInteractions(self, action_runner):
    sink_name = self._GetDeviceName()
    # Wait for 5s after Chrome is opened in order to get consistent results.
    action_runner.Wait(5)
    with action_runner.CreateInteraction('flinging'):

      self._WaitForResult(
          action_runner,
          lambda: action_runner.EvaluateJavaScript('initialized'),
          'Failed to initialize',
          timeout=30)
      self.CloseExistingRoute(action_runner, sink_name)

      # Start session
      action_runner.TapElement(selector='#start_session_button')
      self._WaitForResult(
          action_runner,
          lambda: len(action_runner.tab.browser.tabs) >= 2,
          'MR dialog never showed up.')

      for tab in action_runner.tab.browser.tabs:
        # Choose sink
        if tab.url == 'chrome://media-router/':
          self.WaitUntilDialogLoaded(action_runner, tab)
          self.ChooseSink(tab, sink_name)

      self._WaitForResult(
        action_runner,
        lambda: action_runner.EvaluateJavaScript('currentSession'),
         'Failed to start session',
         timeout=10)

      # Load Media
      self.ExecuteAsyncJavaScript(
          action_runner,
          js_template.Render(
              'loadMedia({{ url }});', url=utils.GetInternalVideoURL()),
          lambda: action_runner.EvaluateJavaScript('currentMedia'),
          'Failed to load media',
          timeout=120)

      action_runner.Wait(5)
      action_runner.ExecuteJavaScript('collectPerfData();')
      action_runner.Wait(SESSION_TIME)
      # Stop session
      self.ExecuteAsyncJavaScript(
          action_runner,
          'stopSession();',
          lambda: not action_runner.EvaluateJavaScript('currentSession'),
          'Failed to stop session',
          timeout=60, retry=3)


class CastMirroringPage(media_router_page.CastPage):
  """Cast page to mirror a tab to Chromecast device."""

  def __init__(self, page_set):
    super(CastMirroringPage, self).__init__(
        page_set=page_set,
        url='file://mirroring.html',
        shared_page_state_class=SharedState,
        name='mirroring.html')

  def RunPageInteractions(self, action_runner):
    sink_name = self._GetDeviceName()
    # Wait for 5s after Chrome is opened in order to get consistent results.
    action_runner.Wait(5)
    with action_runner.CreateInteraction('mirroring'):
      self.CloseExistingRoute(action_runner, sink_name)

      # Start session
      action_runner.TapElement(selector='#start_session_button')
      self._WaitForResult(
          action_runner,
          lambda: len(action_runner.tab.browser.tabs) >= 2,
          'MR dialog never showed up.')

      for tab in action_runner.tab.browser.tabs:
        # Choose sink
        if tab.url == 'chrome://media-router/':
          self.WaitUntilDialogLoaded(action_runner, tab)
          self.ChooseSink(tab, sink_name)

      # Wait for 5s to make sure the route is created.
      action_runner.Wait(5)
      action_runner.TapElement(selector='#start_session_button')
      self._WaitForResult(
          action_runner,
          lambda: len(action_runner.tab.browser.tabs) >= 2,
          'MR dialog never showed up.')

      for tab in action_runner.tab.browser.tabs:
        if tab.url == 'chrome://media-router/':
          self.WaitUntilDialogLoaded(action_runner, tab)
          if not self.CheckIfExistingRoute(tab, sink_name):
            raise RuntimeError('Failed to start mirroring session.')
      action_runner.ExecuteJavaScript('collectPerfData();')
      action_runner.Wait(SESSION_TIME)
      self.CloseExistingRoute(action_runner, sink_name)


class MediaRouterDialogPageSet(story.StorySet):
  """Pageset for media router dialog latency tests."""

  def __init__(self):
    super(MediaRouterDialogPageSet, self).__init__(
        cloud_storage_bucket=story.PARTNER_BUCKET)
    self.AddStory(CastDialogPage(self))


class MediaRouterCPUMemoryPageSet(story.StorySet):
  """Pageset for media router CPU/memory usage tests."""

  def __init__(self):
    super(MediaRouterCPUMemoryPageSet, self).__init__(
        cloud_storage_bucket=story.PARTNER_BUCKET)
    self.AddStory(CastIdlePage(self))
    self.AddStory(CastFlingingPage(self))
    self.AddStory(CastMirroringPage(self))


class CPUMemoryPageSet(story.StorySet):
  """Pageset to get baseline CPU and memory usage."""

  def __init__(self):
    super(CPUMemoryPageSet, self).__init__(
        cloud_storage_bucket=story.PARTNER_BUCKET)
    self.AddStory(CastIdlePage(self))
