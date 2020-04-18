# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from telemetry.page import page as page_module
from telemetry import story

from page_sets import webgl_supported_shared_state


_MAPS_PERF_TEST_DIR = os.path.join(os.path.dirname(__file__), 'maps_perf_test')

class MapsPage(page_module.Page):
  """Google Maps benchmarks and pixel tests.

  The Maps team gave us a build of their test. The static files are stored in
  //src/tools/perf/page_sets/maps_perf_test/.

  Note: the file maps_perf_test/load_dataset is a large binary file (~3Mb),
  hence we upload it to cloud storage & only check in the SHA1 hash.

  The command to upload it to cloud_storage is:
  <path to depot_tools>/upload_to_google_storage.py \
      maps_perf_test/load_dataset --bucket=chromium-telemetry
"""

  def __init__(self, page_set):
    super(MapsPage, self).__init__(
      url='file://performance.html',
      base_dir=_MAPS_PERF_TEST_DIR,
      page_set=page_set,
      shared_page_state_class=(
          webgl_supported_shared_state.WebGLSupportedSharedState),
      name='maps_perf_test')

  @property
  def skipped_gpus(self):
    # Skip this intensive test on low-end devices. crbug.com/464731
    return ['arm']

  def RunPageInteractions(self, action_runner):
    action_runner.WaitForJavaScriptCondition('window.startTest !== undefined')
    action_runner.EvaluateJavaScript('startTest()')
    with action_runner.CreateInteraction('MapAnimation'):
      action_runner.WaitForJavaScriptCondition('window.testDone', timeout=120)


class MapsPageSet(story.StorySet):

  """ Google Maps examples """

  def __init__(self):
    super(MapsPageSet,self).__init__(cloud_storage_bucket=story.PUBLIC_BUCKET)

    self.AddStory(MapsPage(self))
