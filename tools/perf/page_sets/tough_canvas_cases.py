# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry import story


class ToughCanvasCasesPage(page_module.Page):

  def __init__(self, url, page_set):
    name = url
    if not url.startswith('http'):
      name = url[7:]
    super(ToughCanvasCasesPage, self).__init__(url=url, page_set=page_set,
                                               name=name)

  def RunNavigateSteps(self, action_runner):
    super(ToughCanvasCasesPage, self).RunNavigateSteps(action_runner)
    action_runner.WaitForJavaScriptCondition(
        "document.readyState == 'complete'")

  def RunPageInteractions(self, action_runner):
    with action_runner.CreateInteraction('CanvasAnimation'):
      action_runner.Wait(5)


class MicrosofFirefliesPage(ToughCanvasCasesPage):

  def __init__(self, page_set):
    super(MicrosofFirefliesPage, self).__init__(
      # pylint: disable=line-too-long
      url='http://ie.microsoft.com/testdrive/Performance/Fireflies/Default.html',
      page_set=page_set)


class ToughCanvasCasesPageSet(story.StorySet):

  """
  Description: Self-driven Canvas2D animation examples
  """

  def __init__(self):
    super(ToughCanvasCasesPageSet, self).__init__(
      archive_data_file='data/tough_canvas_cases.json',
      cloud_storage_bucket=story.PARTNER_BUCKET)

    # Crashes on Galaxy Nexus. crbug.com/314131
    # TODO(rnephew): Rerecord this story.
    # self.AddStory(MicrosofFirefliesPage(self))

    urls_list = [
      'http://geoapis.appspot.com/agdnZW9hcGlzchMLEgtFeGFtcGxlQ29kZRjh1wIM',
      'http://runway.countlessprojects.com/prototype/performance_test.html',
      # pylint: disable=line-too-long
      'http://ie.microsoft.com/testdrive/Performance/FishIETank/Default.html',
      'http://ie.microsoft.com/testdrive/Performance/SpeedReading/Default.html',
      'http://www.kevs3d.co.uk/dev/canvask3d/k3d_test.html',
      'http://www.megidish.net/awjs/',
      'http://themaninblue.com/experiment/AnimationBenchmark/canvas/',
      'http://mix10k.visitmix.com/Entry/Details/169',
      'http://www.craftymind.com/factory/guimark2/HTML5ChartingTest.html',
      'http://www.chiptune.com/starfield/starfield.html',
      'http://jarrodoverson.com/static/demos/particleSystem/',
      'http://www.effectgames.com/demos/canvascycle/',
      'http://spielzeugz.de/html5/liquid-particles.html',
      'http://hakim.se/experiments/html5/magnetic/02/',
      'http://ie.microsoft.com/testdrive/Performance/LetItSnow/',
      'http://ie.microsoft.com/testdrive/Graphics/WorkerFountains/Default.html',
      'http://ie.microsoft.com/testdrive/Graphics/TweetMap/Default.html',
      'http://ie.microsoft.com/testdrive/Graphics/VideoCity/Default.html',
      'http://ie.microsoft.com/testdrive/Performance/AsteroidBelt/Default.html',
      'http://www.smashcat.org/av/canvas_test/',
      # pylint: disable=line-too-long
      'file://tough_canvas_cases/canvas2d_balls_common/bouncing_balls.html?ball=image_with_shadow&back=image',
      # pylint: disable=line-too-long
      'file://tough_canvas_cases/canvas2d_balls_common/bouncing_balls.html?ball=text&back=white&ball_count=15',
      'file://tough_canvas_cases/canvas-font-cycler.html',
      'file://tough_canvas_cases/canvas-animation-no-clear.html',
      'file://tough_canvas_cases/canvas_toBlob.html',
      'file://../../../chrome/test/data/perf/canvas_bench/many_images.html',
      'file://tough_canvas_cases/rendering_throughput/canvas_arcs.html',
      'file://tough_canvas_cases/rendering_throughput/canvas_lines.html',
      'file://tough_canvas_cases/rendering_throughput/put_get_image_data.html',
      'file://tough_canvas_cases/rendering_throughput/fill_shapes.html',
      'file://tough_canvas_cases/rendering_throughput/stroke_shapes.html',
      'file://tough_canvas_cases/rendering_throughput/bouncing_clipped_rectangles.html',
      'file://tough_canvas_cases/rendering_throughput/bouncing_gradient_circles.html',
      'file://tough_canvas_cases/rendering_throughput/bouncing_svg_images.html',
      'file://tough_canvas_cases/rendering_throughput/bouncing_png_images.html'
    ]

    for url in urls_list:
      self.AddStory(ToughCanvasCasesPage(url, self))
