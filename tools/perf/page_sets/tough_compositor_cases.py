# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry.page import shared_page_state
from telemetry import story


class ToughCompositorPage(page_module.Page):

  def __init__(self, url, page_set):
    super(ToughCompositorPage, self).__init__(
        url=url, page_set=page_set,
        shared_page_state_class=shared_page_state.SharedMobilePageState,
        name=url)

  def RunNavigateSteps(self, action_runner):
    super(ToughCompositorPage, self).RunNavigateSteps(action_runner)
    # TODO(epenner): Remove this wait (http://crbug.com/366933)
    action_runner.Wait(5)

class ToughCompositorScrollPage(ToughCompositorPage):

  def __init__(self, url, page_set):
    super(ToughCompositorScrollPage, self).__init__(url=url, page_set=page_set)

  def RunPageInteractions(self, action_runner):
    # Make the scroll longer to reduce noise.
    with action_runner.CreateGestureInteraction('ScrollAction'):
      action_runner.ScrollPage(direction='down', speed_in_pixels_per_second=300)

class ToughCompositorWaitPage(ToughCompositorPage):

  def __init__(self, url, page_set):
    super(ToughCompositorWaitPage, self).__init__(url=url, page_set=page_set)

  def RunPageInteractions(self, action_runner):
    # We scroll back and forth a few times to reduce noise in the tests.
    with action_runner.CreateInteraction('Animation'):
      action_runner.Wait(8)


class ToughCompositorCasesPageSet(story.StorySet):

  """ Touch compositor sites """

  def __init__(self):
    super(ToughCompositorCasesPageSet, self).__init__(
      archive_data_file='data/tough_compositor_cases.json',
      cloud_storage_bucket=story.PUBLIC_BUCKET)

    scroll_urls_list = [
      # Why: Baseline CC scrolling page. A long page with only text. """
      'http://jsbin.com/pixavefe/1/quiet?CC_SCROLL_TEXT_ONLY',
      # Why: Baseline JS scrolling page. A long page with only text. """
      'http://jsbin.com/wixadinu/2/quiet?JS_SCROLL_TEXT_ONLY',
      # Why: Scroll by a large number of CC layers """
      'http://jsbin.com/yakagevo/1/quiet?CC_SCROLL_200_LAYER_GRID',
      # Why: Scroll by a large number of JS layers """
      'http://jsbin.com/jevibahi/4/quiet?JS_SCROLL_200_LAYER_GRID',
    ]

    wait_urls_list = [
      # Why: CC Poster circle animates many layers """
      'http://jsbin.com/falefice/1/quiet?CC_POSTER_CIRCLE',
      # Why: JS poster circle animates/commits many layers """
      'http://jsbin.com/giqafofe/1/quiet?JS_POSTER_CIRCLE',
      # Why: JS invalidation does lots of uploads """
      'http://jsbin.com/beqojupo/1/quiet?JS_FULL_SCREEN_INVALIDATION',
      # Why: Creates a large number of new tilings """
      'http://jsbin.com/covoqi/1/quiet?NEW_TILINGS',
    ]

    for url in scroll_urls_list:
      self.AddStory(ToughCompositorScrollPage(url, self))

    for url in wait_urls_list:
      self.AddStory(ToughCompositorWaitPage(url, self))
