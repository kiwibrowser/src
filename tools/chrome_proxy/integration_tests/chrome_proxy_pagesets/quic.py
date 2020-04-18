# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from common.chrome_proxy_shared_page_state import ChromeProxySharedPageState
from telemetry.page import page as page_module
from telemetry import story


class QuicPage(page_module.Page):

  def __init__(self, url, page_set):
    super(QuicPage, self).__init__(url=url, page_set=page_set,
        shared_page_state_class=ChromeProxySharedPageState)


class QuicStorySet(story.StorySet):

  """ Chrome proxy test page to verify the QUIC proxy usage """

  def __init__(self):
    super(QuicStorySet, self).__init__()

    urls_list = [
      'http://check.googlezip.net/test.html',
    ]

    for url in urls_list:
      self.AddStory(QuicPage(url, self))
