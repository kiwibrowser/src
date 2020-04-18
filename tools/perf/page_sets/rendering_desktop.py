# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import shared_page_state
from telemetry import story

from page_sets import top_25_smooth


class _SharedPageState(shared_page_state.SharedDesktopPageState):
  pass


class RenderingDesktopPageSet(story.StorySet):
  """ Page set for measuring rendering performance on desktop. """

  def __init__(self, scroll_forever=False):
    super(RenderingDesktopPageSet, self).__init__(
        archive_data_file='data/rendering_desktop.json',
        cloud_storage_bucket=story.PARTNER_BUCKET)

    self.scroll_forever = scroll_forever

    top_25_smooth.AddPagesToPageSet(
        page_set=self,
        shared_page_state_class=_SharedPageState)
