# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from common.chrome_proxy_shared_page_state import ChromeProxySharedPageState
from telemetry.page import page as page_module
from telemetry import story


class CorsBypassPage(page_module.Page):

  def __init__(self, url, page_set):
    super(CorsBypassPage, self).__init__(url=url, page_set=page_set,
        shared_page_state_class=ChromeProxySharedPageState)


class CorsBypassStorySet(story.StorySet):

  """ Chrome proxy test sites """

  def __init__(self):
    super(CorsBypassStorySet, self).__init__()

    urls_list = [
      'http://www.gstatic.com/chrome/googlezip/cors/',
    ]

    for url in urls_list:
      self.AddStory(CorsBypassPage(url, self))
