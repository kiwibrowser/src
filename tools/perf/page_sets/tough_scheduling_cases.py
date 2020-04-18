# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry.page import shared_page_state
from telemetry import story


class ToughSchedulingCasesPage(page_module.Page):

  def __init__(self, url, page_set):
    super(ToughSchedulingCasesPage, self).__init__(
        url=url, page_set=page_set,
        shared_page_state_class=shared_page_state.SharedMobilePageState,
        name=url.split('/')[-1])

  def RunPageInteractions(self, action_runner):
    with action_runner.CreateGestureInteraction('ScrollAction'):
      action_runner.ScrollPage()


class TouchDraggingPage(ToughSchedulingCasesPage):

  """Why: Simple JS touch dragging."""

  def __init__(self, page_set):
    super(TouchDraggingPage, self).__init__(
        url='file://tough_scheduling_cases/simple_touch_drag.html',
        page_set=page_set)

  def RunPageInteractions(self, action_runner):
    with action_runner.CreateGestureInteraction('ScrollAction'):
      action_runner.ScrollElement(
          selector='#card',
          use_touch=True,
          direction='up',
          speed_in_pixels_per_second=150,
          distance=400)


class SynchronizedScrollOffsetPage(ToughSchedulingCasesPage):

  """Why: For measuring the latency of scroll-synchronized effects."""

  def __init__(self, page_set):
    super(SynchronizedScrollOffsetPage, self).__init__(
        url='file://tough_scheduling_cases/sync_scroll_offset.html',
        page_set=page_set)

  def RunPageInteractions(self, action_runner):
    with action_runner.CreateGestureInteraction('ScrollBounceAction'):
      action_runner.ScrollBouncePage()


class SecondBatchJsPage(ToughSchedulingCasesPage):

  """Why: For testing dynamically loading a large batch of Javascript and
          running a part of it in response to user input.
  """

  def __init__(self, page_set, variant='medium'):
    super(SecondBatchJsPage, self).__init__(
        url='file://tough_scheduling_cases/second_batch_js.html?%s' % variant,
        page_set=page_set)

  def RunPageInteractions(self, action_runner):
    # Do a dummy tap to warm up the synthetic tap code path.
    action_runner.TapElement(selector='div[id="spinner"]')
    # Begin the action immediately because we want the page to update smoothly
    # even while resources are being loaded.
    action_runner.WaitForJavaScriptCondition('window.__ready !== undefined')

    with action_runner.CreateGestureInteraction('LoadAction'):
      action_runner.ExecuteJavaScript('kickOffLoading()')
      action_runner.WaitForJavaScriptCondition('window.__ready')
      # Click one second after the resources have finished loading.
      action_runner.Wait(1)
      action_runner.TapElement(selector='input[id="run"]')
      # Wait for the test to complete.
      action_runner.WaitForJavaScriptCondition('window.__finished')


class ToughSchedulingCasesPageSet(story.StorySet):

  """Tough scheduler latency test cases."""

  def __init__(self):
    super(ToughSchedulingCasesPageSet, self).__init__(
        cloud_storage_bucket=story.INTERNAL_BUCKET)

    # Why: Simple scrolling baseline
    self.AddStory(ToughSchedulingCasesPage(
        'file://tough_scheduling_cases/simple_text_page.html',
        self))
    # Why: Touch handler scrolling baseline
    self.AddStory(ToughSchedulingCasesPage(
        'file://tough_scheduling_cases/touch_handler_scrolling.html',
        self))
    # Why: requestAnimationFrame scrolling baseline
    self.AddStory(ToughSchedulingCasesPage(
        'file://tough_scheduling_cases/raf.html',
        self))
    # Why: Test canvas blocking behavior
    self.AddStory(ToughSchedulingCasesPage(
        'file://tough_scheduling_cases/raf_canvas.html',
        self))
    # Why: Test a requestAnimationFrame handler with concurrent CSS animation
    self.AddStory(ToughSchedulingCasesPage(
        'file://tough_scheduling_cases/raf_animation.html',
        self))
    # Why: Stress test for the scheduler
    self.AddStory(ToughSchedulingCasesPage(
        'file://tough_scheduling_cases/raf_touch_animation.html',
        self))
    self.AddStory(TouchDraggingPage(self))
    # Why: For measuring the latency of scroll-synchronized effects.
    self.AddStory(SynchronizedScrollOffsetPage(page_set=self))
    # Why: Test loading a large amount of Javascript.
    self.AddStory(SecondBatchJsPage(page_set=self, variant='light'))
    self.AddStory(SecondBatchJsPage(page_set=self, variant='medium'))
    self.AddStory(SecondBatchJsPage(page_set=self, variant='heavy'))
