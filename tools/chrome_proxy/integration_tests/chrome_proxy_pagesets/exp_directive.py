# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from common.chrome_proxy_shared_page_state import ChromeProxySharedPageState
from telemetry.page import page as page_module
from telemetry import story


class ExpDirectivePage(page_module.Page):
  """A test page for the experiment Chrome-Proxy directive tests."""

  def __init__(self, url, page_set):
    super(ExpDirectivePage, self).__init__(url=url, page_set=page_set,
        shared_page_state_class=ChromeProxySharedPageState)


class ExpDirectiveStorySet(story.StorySet):
  """ Chrome proxy test sites """

  def __init__(self):
    super(ExpDirectiveStorySet, self).__init__()

    urls_list = [
      'http://check.googlezip.net/exp/',
    ]

    for url in urls_list:
      self.AddStory(ExpDirectivePage(url, self))
