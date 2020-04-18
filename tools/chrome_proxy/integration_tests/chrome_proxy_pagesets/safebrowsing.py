# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import logging

from common.chrome_proxy_shared_page_state import ChromeProxySharedPageState
from telemetry.core import exceptions
from telemetry.page import page as page_module
from telemetry import story


class SafebrowsingPage(page_module.Page):

  """
  Why: Expect 'malware ahead' page. Use a short navigation timeout because no
  response will be received.
  """

  def __init__(self, url, page_set, expect_timeout):
    super(SafebrowsingPage, self).__init__(url=url, page_set=page_set,
        shared_page_state_class=ChromeProxySharedPageState)
    self._expect_timeout = expect_timeout

  def RunNavigateSteps(self, action_runner):
    try:
      action_runner.Navigate(self.url, timeout_in_seconds=5)
    except exceptions.TimeoutException as e:
      if self._expect_timeout:
        logging.warning('Navigation timeout on page %s', self.url)
      else:
        raise e


class SafebrowsingStorySet(story.StorySet):

  """ Chrome proxy test sites """

  def __init__(self, expect_timeout=False):
    super(SafebrowsingStorySet, self).__init__()

    self.AddStory(
        SafebrowsingPage('http://www.ianfette.org/', self, expect_timeout))
