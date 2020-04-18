# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from common.chrome_proxy_shared_page_state import ChromeProxySharedPageState
from telemetry.page import page as page_module
from telemetry import story

class PassThroughPage(page_module.Page):
  """
  A test page for the chrome proxy pass-through tests.
  """

  def __init__(self, url, page_set):
    super(PassThroughPage, self).__init__(url=url, page_set=page_set,
        shared_page_state_class=ChromeProxySharedPageState)

  def RunNavigateSteps(self, action_runner):
    super(PassThroughPage, self).RunNavigateSteps(action_runner)
    action_runner.ExecuteJavaScript('''
        (function() {
          var request = new XMLHttpRequest();
          request.open("GET", {{ url }});
          request.setRequestHeader("Chrome-Proxy-Accept-Transform", "identity");
          request.send(null);
        })();''', url=self.url)
    action_runner.Wait(1)


class PassThroughStorySet(story.StorySet):
  """ Chrome proxy test sites """

  def __init__(self):
    super(PassThroughStorySet, self).__init__()

    urls_list = [
      'http://check.googlezip.net/image.png',
    ]

    for url in urls_list:
      self.AddStory(PassThroughPage(url, self))
