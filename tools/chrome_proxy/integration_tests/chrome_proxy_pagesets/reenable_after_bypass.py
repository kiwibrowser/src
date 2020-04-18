# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from common.chrome_proxy_shared_page_state import ChromeProxySharedPageState
from telemetry.page import page as page_module
from telemetry import story


class ReenableAfterBypassPage(page_module.Page):
  """A test page for the re-enable after bypass tests.

  Attributes:
      bypass_seconds_min: The minimum number of seconds that the bypass
          triggered by loading this page should last.
      bypass_seconds_max: The maximum number of seconds that the bypass
          triggered by loading this page should last.
  """

  def __init__(self,
               url,
               page_set,
               bypass_seconds_min,
               bypass_seconds_max):
    super(ReenableAfterBypassPage, self).__init__(url=url, page_set=page_set,
        shared_page_state_class=ChromeProxySharedPageState)
    self.bypass_seconds_min = bypass_seconds_min
    self.bypass_seconds_max = bypass_seconds_max


class ReenableAfterBypassStorySet(story.StorySet):
  """ Chrome proxy test sites """

  def __init__(self):
    super(ReenableAfterBypassStorySet, self).__init__()

    # Test page for "Chrome-Proxy: block=0". Loading this page should cause all
    # data reduction proxies to be bypassed for one to five minutes.
    self.AddStory(ReenableAfterBypassPage(
        url="http://check.googlezip.net/block/",
        page_set=self,
        bypass_seconds_min=60,
        bypass_seconds_max=300))
