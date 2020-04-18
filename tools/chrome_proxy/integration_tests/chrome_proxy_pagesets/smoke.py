# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from common.chrome_proxy_shared_page_state import ChromeProxySharedPageState
from telemetry.page import page as page_module
from telemetry import story


class SmokePage(page_module.Page):

  def __init__(self, url, page_set, name=''):
    super(SmokePage, self).__init__(url=url, page_set=page_set, name=name,
        shared_page_state_class=ChromeProxySharedPageState)


class Page1(SmokePage):

  """
  Why: Check chrome proxy response headers.
  """

  def __init__(self, page_set):
    super(Page1, self).__init__(
      url='http://check.googlezip.net/test.html',
      page_set=page_set,
      name='header validation')


class Page2(SmokePage):

  """
  Why: Check data compression
  """

  def __init__(self, page_set):
    super(Page2, self).__init__(
      url='http://check.googlezip.net/static/',
      page_set=page_set,
      name='compression: image')


class Page3(SmokePage):

  """
  Why: Check bypass
  """

  def __init__(self, page_set):
    super(Page3, self).__init__(
      url='http://check.googlezip.net/block/',
      page_set=page_set,
      name='bypass')


class Page4(SmokePage):

  """
  Why: Check data compression
  """

  def __init__(self, page_set):
    super(Page4, self).__init__(
      url='http://check.googlezip.net/static/',
      page_set=page_set,
      name='compression: javascript')


class Page5(SmokePage):

  """
  Why: Check data compression
  """

  def __init__(self, page_set):
    super(Page5, self).__init__(
      url='http://check.googlezip.net/static/',
      page_set=page_set,
      name='compression: css')



class SmokeStorySet(story.StorySet):

  """ Chrome proxy test sites """

  def __init__(self):
    super(SmokeStorySet, self).__init__()

    self.AddStory(Page1(self))
    self.AddStory(Page2(self))
    self.AddStory(Page3(self))
    self.AddStory(Page4(self))
    self.AddStory(Page5(self))
