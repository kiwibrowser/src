# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from common.chrome_proxy_shared_page_state import ChromeProxySharedPageState
from telemetry.page import page as page_module
from telemetry.page import shared_page_state
from telemetry import story


class ControllableProxySharedState(ChromeProxySharedPageState):

  def WillRunStory(self, page):
    if page.use_chrome_proxy:
      self._finder_options.AppendExtraBrowserArgs('--enable-spdy-proxy-auth')
    super(ControllableProxySharedState, self).WillRunStory(page)


class VideoPage(page_module.Page):
  """A test page containing a video.

  Attributes:
      use_chrome_proxy: If true, fetches use the data reduction proxy.
          Otherwise, fetches are sent directly to the origin.
  """

  def __init__(self, url, page_set, use_chrome_proxy):
    super(VideoPage, self).__init__(
      url=url, page_set=page_set,
      shared_page_state_class=ControllableProxySharedState)
    self.use_chrome_proxy = use_chrome_proxy


class VideoStorySet(story.StorySet):
  """Base class for Chrome proxy video tests."""

  def __init__(self, mode):
    super(VideoStorySet, self).__init__()
    urls_list = [
        'http://check.googlezip.net/cacheable/video/buck_bunny_tiny.html',
    ]
    for url in urls_list:
      self._AddStoryForURL(url)

  def _AddStoryForURL(self, url):
    raise NotImplementedError


class VideoDirectStorySet(VideoStorySet):
  """Chrome proxy video tests: direct fetch."""
  def __init__(self):
    super(VideoDirectStorySet, self).__init__('direct')

  def _AddStoryForURL(self, url):
      self.AddStory(VideoPage(url, self, False))


class VideoProxiedStorySet(VideoStorySet):
  """Chrome proxy video tests: proxied fetch."""
  def __init__(self):
    super(VideoProxiedStorySet, self).__init__('proxied')

  def _AddStoryForURL(self, url):
      self.AddStory(VideoPage(url, self, True))


class VideoCompareStorySet(VideoStorySet):
  """Chrome proxy video tests: compare direct and proxied fetches."""
  def __init__(self):
    super(VideoCompareStorySet, self).__init__('compare')

  def _AddStoryForURL(self, url):
      self.AddStory(VideoPage(url, self, False))
      self.AddStory(VideoPage(url, self, True))
