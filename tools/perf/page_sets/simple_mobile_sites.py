# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry.page import shared_page_state
from telemetry import story


class SimplePage(page_module.Page):

  def __init__(self, url, page_set):
    super(SimplePage, self).__init__(
        url=url,
        page_set=page_set,
        shared_page_state_class=shared_page_state.Shared10InchTabletPageState,
        name=url)

  def RunNavigateSteps(self, action_runner):
    super(SimplePage, self).RunNavigateSteps(action_runner)
    # TODO(epenner): Remove this wait (http://crbug.com/366933)
    action_runner.Wait(5)

class SimpleScrollPage(SimplePage):

  def __init__(self, url, page_set):
    super(SimpleScrollPage, self).__init__(url=url, page_set=page_set)

  def RunPageInteractions(self, action_runner):
    # Make the scroll longer to reduce noise.
    with action_runner.CreateGestureInteraction('ScrollAction'):
      action_runner.ScrollPage(direction='down', speed_in_pixels_per_second=300)


class SimpleMobileSitesPageSet(story.StorySet):
  """ Simple mobile sites """

  def __init__(self):
    super(SimpleMobileSitesPageSet, self).__init__(
      archive_data_file='data/simple_mobile_sites.json',
      cloud_storage_bucket=story.PUBLIC_BUCKET)

    scroll_page_list = [
      # Why: Scrolls moderately complex pages (up to 60 layers)
      'http://www.ebay.co.uk/',
      'https://www.flickr.com/',
      'http://www.nyc.gov',
      'http://m.nytimes.com/'
    ]

    for url in scroll_page_list:
      self.AddStory(SimpleScrollPage(url, self))
