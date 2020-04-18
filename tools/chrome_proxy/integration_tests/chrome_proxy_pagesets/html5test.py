# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from common.chrome_proxy_shared_page_state import ChromeProxySharedPageState
from telemetry.page import page as page_module
from telemetry import story


class HTML5TestPage(page_module.Page):

  def __init__(self, url, page_set):
    super(HTML5TestPage, self).__init__(url=url, page_set=page_set,
        shared_page_state_class=ChromeProxySharedPageState)


class HTML5TestStorySet(story.StorySet):

  """ Chrome proxy test page for traffic over https. """

  def __init__(self):
    super(HTML5TestStorySet, self).__init__()

    urls_list = [
      'http://html5test.com/',
    ]

    for url in urls_list:
      self.AddStory(HTML5TestPage(url, self))
