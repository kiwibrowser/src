# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from common.chrome_proxy_shared_page_state import ChromeProxySharedPageState
from telemetry.page import page as page_module
from telemetry import story


class SyntheticPage(page_module.Page):

  def __init__(self, url, page_set):
    super(SyntheticPage, self).__init__(url=url, page_set=page_set,
        shared_page_state_class=ChromeProxySharedPageState)


class SyntheticStorySet(story.StorySet):

  """ Chrome proxy synthetic test pages. """

  def __init__(self):
    super(SyntheticStorySet, self).__init__()

    urls_list = [
      'http://check.googlezip.net/test.html',
      'http://check.googlezip.net/static/'
    ]

    for url in urls_list:
      self.AddStory(SyntheticPage(url, self))
