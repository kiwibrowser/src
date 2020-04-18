# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry import story


class ToughPathRenderingCasesPage(page_module.Page):
  def RunPageInteractions(self, action_runner):
    with action_runner.CreateInteraction('ClickStart'):
      action_runner.Wait(10)


class ChalkboardPage(page_module.Page):

  def RunPageInteractions(self, action_runner):
    with action_runner.CreateInteraction('ClickStart'):
      action_runner.EvaluateJavaScript(
          'document.getElementById("StartButton").click()')
      action_runner.Wait(20)

class ToughPathRenderingCasesPageSet(story.StorySet):

  """
  Description: Self-driven path rendering examples
  """

  def __init__(self):
    super(ToughPathRenderingCasesPageSet, self).__init__(
      archive_data_file='data/tough_path_rendering_cases.json',
      cloud_storage_bucket=story.PARTNER_BUCKET)

    page_list = [
      ('GUIMark_Vector_Chart_Test',
      'http://www.craftymind.com/factory/guimark2/HTML5ChartingTest.html'),
      ('MotionMark_Canvas_Fill_Shapes',
      'http://rawgit.com/WebKit/webkit/master/PerformanceTests/MotionMark/developer.html?test-name=Fillshapes&test-interval=20&display=minimal&tiles=big&controller=fixed&frame-rate=50&kalman-process-error=1&kalman-measurement-error=4&time-measurement=performance&suite-name=Canvassuite&complexity=1000'), # pylint: disable=line-too-long
      ('MotionMark_Canvas_Stroke_Shapes',
      'http://rawgit.com/WebKit/webkit/master/PerformanceTests/MotionMark/developer.html?test-name=Strokeshapes&test-interval=20&display=minimal&tiles=big&controller=fixed&frame-rate=50&kalman-process-error=1&kalman-measurement-error=4&time-measurement=performance&suite-name=Canvassuite&complexity=1000'), # pylint: disable=line-too-long
    ]

    for name, url in page_list:
      self.AddStory(ToughPathRenderingCasesPage(name=name, url=url,
          page_set=self))

    # Chalkboard content linked from
    # http://ie.microsoft.com/testdrive/Performance/Chalkboard/.
    chalkboard_url = ('https://testdrive-archive.azurewebsites.net'
                      '/performance/chalkboard/')
    name = 'IE_Chalkboard'
    self.AddStory(ChalkboardPage(chalkboard_url, self, name=name))
