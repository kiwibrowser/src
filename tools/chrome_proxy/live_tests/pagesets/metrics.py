# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry.page import page as page_module
from telemetry import story


class MetricsPage(page_module.Page):

  def __init__(self, url, page_set):
    super(MetricsPage, self).__init__(url=url, page_set=page_set)


class MetricsStorySet(story.StorySet):

  """ Chrome proxy test sites for measuring data savings """

  def __init__(self):
    super(MetricsStorySet, self).__init__()

    urls_list = [
      'http://check.googlezip.net/metrics/',
    ]

    for url in urls_list:
      self.AddStory(MetricsPage(url, self))
