# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from page_sets import android_screen_restoration_shared_state

from telemetry.page import page as page_module
from telemetry.page import shared_page_state
from telemetry import story


class KeyIdlePowerPage(page_module.Page):

  def __init__(self, url, page_set, turn_screen_off, duration_seconds=20,
               shared_page_state_class=shared_page_state.SharedMobilePageState):
    super(KeyIdlePowerPage, self).__init__(
        url=url,
        page_set=page_set,
        shared_page_state_class=(android_screen_restoration_shared_state
            .AndroidScreenRestorationSharedState),
        name=url.split('/')[-1])
    self._turn_screen_off = turn_screen_off
    self._duration_seconds = duration_seconds

  def RunNavigateSteps(self, action_runner):
    super(KeyIdlePowerPage, self).RunNavigateSteps(action_runner)
    action_runner.Wait(2)
    if self._turn_screen_off:
      action_runner.tab.browser.platform.android_action_runner.TurnScreenOff()
      # We're not interested in tracking activity that occurs immediately after
      # the screen is turned off. Several seconds should be enough time for the
      # browser to "settle down" into an idle state.
      action_runner.Wait(2)

  def RunPageInteractions(self, action_runner):
    # The page interaction is simply waiting in an idle state.
    with action_runner.CreateInteraction('IdleWaiting'):
      action_runner.Wait(self._duration_seconds)


class KeyLongIdlePowerPage(KeyIdlePowerPage):

  def __init__(self, url, page_set, turn_screen_off,
               shared_page_state_class=shared_page_state.SharedMobilePageState):
    # 90 seconds ensures the capture of activity after the 60-second
    # PowerMonitor suspend signal.
    super(KeyLongIdlePowerPage, self).__init__(
        url=url,
        page_set=page_set,
        turn_screen_off=turn_screen_off,
        duration_seconds=90,
        shared_page_state_class=(android_screen_restoration_shared_state
            .AndroidScreenRestorationSharedState))
    self._name = self.name + " (Long Idle)"


class KeyIdlePowerCasesPageSet(story.StorySet):

  """ Key idle power cases """

  def __init__(self):
    super(KeyIdlePowerCasesPageSet, self).__init__()

    foreground_urls_list = [
      # Why: Ensure minimal activity for static, empty pages in the foreground.
      'file://key_idle_power_cases/blank.html',
    ]

    for url in foreground_urls_list:
      self.AddStory(KeyIdlePowerPage(url, self, False))

    background_urls_list = [
      # Why: Ensure animated GIFs aren't processed when Chrome is backgrounded.
      'file://key_idle_power_cases/animated-gif.html',
      # Why: Ensure CSS animations aren't processed when Chrome is backgrounded.
      'file://key_idle_power_cases/css-animation.html',
      # Why: Ensure rAF is suppressed when Chrome is backgrounded.
      'file://key_idle_power_cases/request-animation-frame.html',
      # Why: Ensure setTimeout is throttled when Chrome is backgrounded.
      'file://key_idle_power_cases/set-timeout.html',
    ]

    for url in background_urls_list:
      self.AddStory(KeyIdlePowerPage(url, self, True))

    # Why: Ensure that activity strictly diminishes the longer the idle time.
    self.AddStory(KeyLongIdlePowerPage(
        'file://key_idle_power_cases/set-timeout.html', self, True))
