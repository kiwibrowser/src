# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from common.chrome_proxy_shared_page_state import ChromeProxySharedPageState
from telemetry.page import page as page_module
from telemetry import story


class BypassOnTimeoutPage(page_module.Page):

  def __init__(self, url, page_set):
    super(BypassOnTimeoutPage, self).__init__(url=url, page_set=page_set,
        shared_page_state_class=ChromeProxySharedPageState)


class BypassOnTimeoutStorySet(story.StorySet):
  """This site takes 60s to respond when being accessed by proxy."""

  def __init__(self):
    super(BypassOnTimeoutStorySet, self).__init__()

    urls_list = [
      'http://chromeproxy-test.appspot.com/blackhole'
    ]

    for url in urls_list:
      self.AddStory(BypassOnTimeoutPage(url, self))
