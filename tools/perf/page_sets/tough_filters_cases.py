# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry import story


class ToughFiltersCasesPage(page_module.Page):

  def __init__(self, url, page_set, name):
    super(ToughFiltersCasesPage, self).__init__(
        url=url, page_set=page_set, name=name)

  def RunPageInteractions(self, action_runner):
    with action_runner.CreateInteraction('Filter'):
      action_runner.Wait(10)


class PirateMarkPage(page_module.Page):

  def RunPageInteractions(self, action_runner):
    action_runner.WaitForNetworkQuiescence()
    with action_runner.CreateInteraction('Filter'):
      action_runner.EvaluateJavaScript(
          'document.getElementById("benchmarkButtonText").click()')
      action_runner.Wait(10)

class ToughFiltersCasesPageSet(story.StorySet):

  """
  Description: Self-driven filters animation examples
  """

  def __init__(self):
    super(ToughFiltersCasesPageSet, self).__init__(
      archive_data_file='data/tough_filters_cases.json',
      cloud_storage_bucket=story.PARTNER_BUCKET)

    urls_list = [
      ('http://rawgit.com/WebKit/webkit/master/PerformanceTests/Animometer/developer.html?test-interval=20&display=minimal&controller=fixed&frame-rate=50&kalman-process-error=1&kalman-measurement-error=4&time-measurement=performance&suite-name=Animometer&test-name=Focus&complexity=100', # pylint: disable=line-too-long
       'MotionMark_Focus'),
      ('http://letmespellitoutforyou.com/samples/svg/filter_terrain.svg',
       'Filter_Terrain_SVG'),
      ('http://static.bobdo.net/Analog_Clock.svg',
       'Analog_Clock_SVG'),
    ]

    for url, name in urls_list:
      self.AddStory(ToughFiltersCasesPage(url, self, name))

    pirate_url = ('http://web.archive.org/web/20150502135732/'
                  'http://ie.microsoft.com/testdrive/Performance/'
                  'Pirates/Default.html')
    name = 'IE_PirateMark'
    self.AddStory(PirateMarkPage(pirate_url, self, name=name))
