# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from common.chrome_proxy_shared_page_state import ChromeProxySharedPageState
from telemetry.page import page as page_module
from telemetry import story


class HTTPSBypassPage(page_module.Page):

  def __init__(self, url, page_set):
    super(HTTPSBypassPage, self).__init__(url=url, page_set=page_set,
        shared_page_state_class=ChromeProxySharedPageState)


class HTTPSBypassStorySet(story.StorySet):

  """ Chrome proxy test page for traffic over https. """

  def __init__(self):
    super(HTTPSBypassStorySet, self).__init__()

    urls_list = [
      'https://check.googlezip.net/test.html',
    ]

    for url in urls_list:
      self.AddStory(HTTPSBypassPage(url, self))
