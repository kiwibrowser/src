# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from common.chrome_proxy_shared_page_state import ChromeProxySharedPageState
from telemetry.page import page as page_module
from telemetry import story


class YouTubePage(page_module.Page):

  def __init__(self, url, page_set):
    super(YouTubePage, self).__init__(url=url, page_set=page_set,
        shared_page_state_class=ChromeProxySharedPageState)


class YouTubeStorySet(story.StorySet):

  """ Chrome proxy test site to verify YouTube functionality. """

  def __init__(self):
    super(YouTubeStorySet, self).__init__()

    urls_list = [
      'http://data-saver-test.appspot.com/youtube',
    ]

    for url in urls_list:
      self.AddStory(YouTubePage(url, self))
