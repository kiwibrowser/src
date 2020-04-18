# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from common.chrome_proxy_shared_page_state import ChromeProxySharedPageState
from telemetry.page import page as page_module
from telemetry import story


class LitePagePage(page_module.Page):
  """
  A test page for the chrome proxy lite page tests.
  Checks that a lite page is served.
  """

  def __init__(self, url, page_set):
    super(LitePagePage, self).__init__(url=url, page_set=page_set,
        shared_page_state_class=ChromeProxySharedPageState)


class LitePageStorySet(story.StorySet):
  """ Chrome proxy test sites """

  def __init__(self):
    super(LitePageStorySet, self).__init__()

    urls_list = [
      'http://check.googlezip.net/test.html',
    ]

    for url in urls_list:
      self.AddStory(LitePagePage(url, self))
