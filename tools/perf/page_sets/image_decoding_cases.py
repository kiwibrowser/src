# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry import story

class ImageDecodingCasesPage(page_module.Page):

  def __init__(self, url, page_set, name):
    super(ImageDecodingCasesPage, self).__init__(
        url=url, page_set=page_set, name=name)

  def RunPageInteractions(self, action_runner):
    with action_runner.CreateInteraction('DecodeImage'):
      action_runner.Wait(5)

class ImageDecodingCasesPageSet(story.StorySet):

  """ A directed benchmark of accelerated jpeg image decoding performance """

  def __init__(self):
    super(ImageDecodingCasesPageSet, self).__init__()

    urls_list = [
      ('file://image_decoding_cases/yuv_decoding.html', 'yuv_decoding.html')
    ]

    for url, name in urls_list:
      self.AddStory(ImageDecodingCasesPage(url, self, name))
