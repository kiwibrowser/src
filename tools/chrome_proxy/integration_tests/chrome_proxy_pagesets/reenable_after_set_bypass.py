# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from common.chrome_proxy_shared_page_state import ChromeProxySharedPageState
from telemetry.page import page as page_module
from telemetry import story


class ReenableAfterSetBypassPage(page_module.Page):
  """A test page for the re-enable after bypass tests with set duration."""

  BYPASS_SECONDS = 20

  def __init__(self, url, page_set):
    super(ReenableAfterSetBypassPage, self).__init__(
        url=url, page_set=page_set,
        shared_page_state_class=ChromeProxySharedPageState)


class ReenableAfterSetBypassStorySet(story.StorySet):
  """ Chrome proxy test sites """

  def __init__(self):
    super(ReenableAfterSetBypassStorySet, self).__init__()

    # Test page for "Chrome-Proxy: block=20". Loading this page should cause all
    # data reduction proxies to be bypassed for ten seconds.
    self.AddStory(ReenableAfterSetBypassPage(
        url="http://check.googlezip.net/block20/",
        page_set=self))
