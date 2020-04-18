# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from common.chrome_proxy_shared_page_state import ChromeProxySharedPageState
from telemetry.page import page as page_module
from telemetry import story


class BlockOncePage(page_module.Page):

  def __init__(self, url, page_set):
    super(BlockOncePage, self).__init__(url=url,page_set=page_set,
        shared_page_state_class=ChromeProxySharedPageState)

  def RunNavigateSteps(self, action_runner):
    super(BlockOncePage, self).RunNavigateSteps(action_runner)
    # Test block-once on a POST request.
    # Ensure that a subsequent request uses the data reduction proxy.
    action_runner.ExecuteJavaScript('''
      (function() {
        window.post_request_completed = false;
        var request = new XMLHttpRequest();
        request.open("POST",
            "http://chromeproxy-test.appspot.com/default?" +
            "respBody=T0s=&respHeader=eyJBY2Nlc3MtQ29udHJvbC1BbGxvdy1Pcml" +
            "naW4iOlsiKiJdfQ==&respStatus=200&flywheelAction=block-once");
        request.onload = function() {
          window.post_request_completed = true;
          var viaProxyRequest = new XMLHttpRequest();
          viaProxyRequest.open("GET",
              "http://check.googlezip.net/image.png");
          viaProxyRequest.send();
        };
        request.send();
      })();
    ''')
    action_runner.WaitForJavaScriptCondition(
        "window.post_request_completed == true", timeout=30)

class BlockOnceStorySet(story.StorySet):

  """ Chrome proxy test sites """

  def __init__(self):
    super(BlockOnceStorySet, self).__init__()

    # Test block-once for a GET request.
    urls_list = [
      'http://check.googlezip.net/blocksingle/',
    ]

    for url in urls_list:
      self.AddStory(BlockOncePage(url, self))
