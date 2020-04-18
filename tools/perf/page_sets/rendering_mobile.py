# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import shared_page_state
from telemetry import story

from page_sets import key_mobile_sites_smooth
from page_sets import top_25_smooth


class _SharedPageState(shared_page_state.SharedMobilePageState):
  pass


class RenderingMobilePageSet(story.StorySet):
  """ Page set for measuring rendering performance on desktop. """

  def __init__(self, scroll_forever=False):
    super(RenderingMobilePageSet, self).__init__(
        archive_data_file='data/rendering_mobile.json',
        cloud_storage_bucket=story.PARTNER_BUCKET)

    self.scroll_forever = scroll_forever

    top_25_smooth.AddPagesToPageSet(
        self,
        shared_page_state_class=_SharedPageState,
        name_func=lambda name: name + '_desktop')
    top_25_smooth.AddPagesToPageSet(
        self,
        shared_page_state_class=_SharedPageState,
        name_func=lambda name: name + '_desktop_gpu_raster',
        extra_browser_args=['--force-gpu-rasterization'])

    key_mobile_sites_smooth.AddPagesToPageSet(
        self,
        shared_page_state_class=_SharedPageState)
    key_mobile_sites_smooth.AddPagesToPageSet(
        self,
        shared_page_state_class=_SharedPageState,
        name_func=lambda name: name + '_sync_scroll',
        extra_browser_args=['--disable-threaded-scrolling'])
