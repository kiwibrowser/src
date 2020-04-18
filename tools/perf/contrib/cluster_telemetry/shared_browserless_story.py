# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


from telemetry import story as story_module
from telemetry.core import platform as p_module


class SharedBrowserlessStory(story_module.SharedState):
  """
  This class subclasses story_module.SharedState and removes all logic required
  to bring up a browser that is found in shared_page_state.SharedPageState.
  """

  def __init__(self, test, finder_options, story_set):
    super(SharedBrowserlessStory, self).__init__(
        test, finder_options, story_set)

  @property
  def platform(self):
    p_module.GetHostPlatform()

  def WillRunStory(self, unused_page):
    return

  def DidRunStory(self, results):
    return

  def CanRunStory(self, unused_page):
    return True

  def RunStory(self, results):
    return

  def TearDownState(self):
    pass

  def DumpStateUponFailure(self, story, results):
    pass
