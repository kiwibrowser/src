# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry.page import shared_page_state
from telemetry import story

class ToughImageDecodeCasesPage(page_module.Page):

  def __init__(self, url, name, page_set):
    super(ToughImageDecodeCasesPage, self).__init__(
      url=url,
      page_set=page_set,
      name=name,
      shared_page_state_class=shared_page_state.SharedMobilePageState)

  def RunPageInteractions(self, action_runner):
    action_runner.WaitForJavaScriptCondition(
      'document.readyState === "complete"')
    action_runner.ScrollPage(direction='down', speed_in_pixels_per_second=5000)
    with action_runner.CreateGestureInteraction('ScrollAction'):
      action_runner.ScrollPage(direction='up', speed_in_pixels_per_second=5000)

class ToughImageDecodeCasesPageSet(story.StorySet):

  """
  Description: A collection of difficult image decode tests
  """

  def __init__(self):
    super(ToughImageDecodeCasesPageSet, self).__init__(
      archive_data_file='data/tough_image_decode_cases.json',
      cloud_storage_bucket=story.PUBLIC_BUCKET)

    page_name_list = [
      'http://localhost:9000/cats-unscaled.html',
      'http://localhost:9000/cats-viewport-width.html'
    ]

    for name in page_name_list:
      self.AddStory(ToughImageDecodeCasesPage(
        name, name, self))
