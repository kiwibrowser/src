# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry.page.shared_page_state import SharedPageState

class ChromeProxySharedPageState(SharedPageState):
  """Overides SharePageState to disable replay service/forwarder."""

  def __init__(self, test, finder_options, story_set):
    super(ChromeProxySharedPageState, self).__init__(
        test, finder_options, story_set)
    network_controller = self.platform.network_controller
    network_controller.StopReplay()

    #TODO(bustamante): Implement/use a non-private way to stop the forwarder.
    network_controller._network_controller_backend._StopForwarder()

