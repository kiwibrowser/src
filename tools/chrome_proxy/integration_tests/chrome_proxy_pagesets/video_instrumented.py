# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from common.chrome_proxy_shared_page_state import ChromeProxySharedPageState
from telemetry.page import page as page_module
from telemetry import story

class VideoFrameStorySet(story.StorySet):
  """Chrome proxy video tests: verify frames of transcoded videos"""
  def __init__(self):
    super(VideoFrameStorySet, self).__init__()
    for url in [
        'http://check.googlezip.net/cacheable/video/buck_bunny_640x360_24fps_video.html',
        'http://check.googlezip.net/cacheable/video/buck_bunny_60fps_video.html',
        ]:
      self.AddStory(page_module.Page(url, self,
          shared_page_state_class=ChromeProxySharedPageState))

class VideoAudioStorySet(story.StorySet):
  """Chrome proxy video tests: verify audio of transcoded videos"""
  def __init__(self):
    super(VideoAudioStorySet, self).__init__()
    for url in [
        'http://check.googlezip.net/cacheable/video/buck_bunny_640x360_24fps_audio.html',
        ]:
      self.AddStory(page_module.Page(url, self,
          shared_page_state_class=ChromeProxySharedPageState))
