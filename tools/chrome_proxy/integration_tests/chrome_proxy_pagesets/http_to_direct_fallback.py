# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from common.chrome_proxy_shared_page_state import ChromeProxySharedPageState
from telemetry.page import page as page_module
from telemetry import story


class HTTPToDirectFallbackPage(page_module.Page):
  """Page that tests falling back from the HTTP proxy to a direct connection."""
  def __init__(self, url, page_set):
    super(HTTPToDirectFallbackPage, self).__init__(url=url, page_set=page_set,
        shared_page_state_class=ChromeProxySharedPageState)


class HTTPToDirectFallbackStorySet(story.StorySet):
  """Chrome proxy test sites"""
  def __init__(self):
    super(HTTPToDirectFallbackStorySet, self).__init__()

    urls_list = [
      'http://check.googlezip.net/fallback/',
      'http://check.googlezip.net/block/',
    ]

    for url in urls_list:
      self.AddStory(HTTPToDirectFallbackPage(url, self))
