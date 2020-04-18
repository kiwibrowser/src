# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry.page import shared_page_state
from telemetry import story


class NoOpPage(page_module.Page):

  def __init__(self, url, page_set):
    super(NoOpPage, self).__init__(
        url=url,
        page_set=page_set,
        shared_page_state_class=shared_page_state.SharedMobilePageState,
        name=url.split('/')[-1])

  def RunNavigateSteps(self, action_runner):
    super(NoOpPage, self).RunNavigateSteps(action_runner)
    # Let load activity settle.
    action_runner.Wait(2)

  def RunPageInteractions(self, action_runner):
    # The default page interaction is simply waiting in an idle state.
    with action_runner.CreateInteraction('IdleWaiting'):
      action_runner.Wait(5)


class NoOpTouchScrollPage(NoOpPage):
  def __init__(self, url, page_set):
    super(NoOpTouchScrollPage, self).__init__(url=url, page_set=page_set)

  def RunPageInteractions(self, action_runner):
    # The noop touch motion should last ~5 seconds.
    with action_runner.CreateGestureInteraction('ScrollAction'):
      action_runner.ScrollPage(direction='down', use_touch=True,
                               speed_in_pixels_per_second=300, distance=1500)


class KeyNoOpCasesPageSet(story.StorySet):

  """ Key no-op cases """

  def __init__(self):
    super(KeyNoOpCasesPageSet, self).__init__()

    # Why: An infinite rAF loop which does not modify the page should incur
    # minimal activity.
    self.AddStory(NoOpPage('file://key_noop_cases/no_op_raf.html', self))

    # Why: An infinite setTimeout loop which does not modify the page should
    # incur minimal activity.
    self.AddStory(NoOpPage('file://key_noop_cases/no_op_settimeout.html', self))

    # Why: Scrolling an empty, unscrollable page should have no expensive side
    # effects, as overscroll is suppressed in such cases.
    self.AddStory(NoOpTouchScrollPage(
        'file://key_noop_cases/no_op_scroll.html', self))

    # Why: Feeding a stream of touch events to a no-op handler should be cheap.
    self.AddStory(NoOpTouchScrollPage(
        'file://key_noop_cases/no_op_touch_handler.html', self))
