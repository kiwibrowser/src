# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry import story

from page_sets import webgl_supported_shared_state


class Tanks(page_module.Page):

  def __init__(self, page_set):
    url = 'http://webassembly.org/demo/Tanks/'
    super(Tanks, self).__init__(
        url=url,
        page_set=page_set,
        shared_page_state_class=(
            webgl_supported_shared_state.WebGLSupportedSharedState),
        name='WasmTanks')

  @property
  def skipped_gpus(self):
    # Unity WebGL is not supported on mobile
    return ['arm', 'qualcomm']

  def RunPageInteractions(self, action_runner):
    action_runner.WaitForJavaScriptCondition(
        """document.getElementsByClassName('progress Dark').length != 0""")
    action_runner.WaitForJavaScriptCondition(
        """document.getElementsByClassName('progress Dark')[0].style['display']
          == 'none'""")

class SpaceBuggy(page_module.Page):

  def __init__(self, page_set):
    url = 'https://playcanv.as/p/3RerJIcy/'
    super(SpaceBuggy, self).__init__(
        url=url,
        page_set=page_set,
        shared_page_state_class=(
            webgl_supported_shared_state.WebGLSupportedSharedState),
        name='WasmSpaceBuggy')

  @property
  def skipped_gpus(self):
    return []

  def RunPageInteractions(self, action_runner):
    action_runner.WaitForJavaScriptCondition("document.getElementById('frame')")
    action_runner.WaitForJavaScriptCondition("""document.getElementById('frame')
        .contentDocument.getElementsByClassName('btn btn-primary btn-play')
        .length != 0""")
    action_runner.ClickElement(element_function="""(function() {return document
        .getElementById("frame").contentDocument.getElementsByClassName(
        "btn btn-primary btn-play")[0]})()""")
    action_runner.WaitForJavaScriptCondition("""document.getElementById('frame')
        .contentDocument.getElementsByClassName('panel level-select')[0]
        .style.bottom != '-100px'""")
    action_runner.ClickElement(element_function="""(function() {return document
        .getElementById("frame").contentDocument.getElementsByClassName(
        "btn btn-primary btn-play")[1]})()""")
    action_runner.WaitForJavaScriptCondition("""document.getElementById('frame')
        .contentDocument.getElementsByClassName('panel level-select')[0]
        .style.bottom == '-100px'""")

class EpicPageSet(page_module.Page):

  def __init__(self, page_set, url, name):
    super(EpicPageSet, self).__init__(
        url=url,
        page_set=page_set,
        shared_page_state_class=(
            webgl_supported_shared_state.WebGLSupportedSharedState),
        name=name)

  @property
  def skipped_gpus(self):
    return ['arm', 'qualcomm']

  def RunPageInteractions(self, action_runner):
    # We wait for the fullscreen button to become visible
    action_runner.WaitForJavaScriptCondition("""document
        .getElementById('fullscreen_request').style.display ===
        'inline-block'""")

class EpicZenGarden(EpicPageSet):

  def __init__(self, page_set):
    url = 'https://s3.amazonaws.com/mozilla-games/ZenGarden/EpicZenGarden.html'
    super(EpicZenGarden, self).__init__(
        page_set=page_set, url=url, name='WasmZenGarden')

class EpicSunTemple(EpicPageSet):

  def __init__(self, page_set):
    url = ("https://s3.amazonaws.com/mozilla-games/tmp/2017-02-21-SunTemple/"
           "SunTemple.html")
    super(EpicSunTemple, self).__init__(
        page_set=page_set, url=url, name='WasmSunTemple')

class EpicStylizedRenderer(EpicPageSet):

  def __init__(self, page_set):
    url = ("https://s3.amazonaws.com/mozilla-games/tmp/2017-02-21-StylizedRen"
           "dering/StylizedRendering.html")
    super(EpicStylizedRenderer, self).__init__(
        page_set=page_set, url=url, name='WasmStylizedRenderer')

class EpicZenGardenAsm(page_module.Page):

  def __init__(self, page_set):
    url = ("https://s3.amazonaws.com/unrealengine/HTML5/TestBuilds/Release-4."
           "17.1-CL-3637171/Zen-HTML5-Shipping.html")
    super(EpicZenGardenAsm, self).__init__(
        url=url,
        page_set=page_set,
        shared_page_state_class=(
            webgl_supported_shared_state.WebGLSupportedSharedState),
        name='AsmJsZenGarden')

  @property
  def skipped_gpus(self):
    # Unity WebGL is not supported on mobile
    return ['arm', 'qualcomm']

  def RunPageInteractions(self, action_runner):
    action_runner.WaitForJavaScriptCondition(
        """document.getElementsByClassName('emscripten').length != 0""")
    action_runner.WaitForJavaScriptCondition(
        """document.getElementsByClassName('emscripten')[0].style['display']
          != 'none'""")

class WasmRealWorldPagesStorySet(story.StorySet):
  """Top apps, used to monitor web assembly apps."""

  def __init__(self):
    super(WasmRealWorldPagesStorySet, self).__init__(
        archive_data_file='data/wasm_realworld_pages.json',
        cloud_storage_bucket=story.INTERNAL_BUCKET)

    self.AddStory(Tanks(self))
    self.AddStory(SpaceBuggy(self))
    self.AddStory(EpicZenGarden(self))
    self.AddStory(EpicSunTemple(self))
    self.AddStory(EpicStylizedRenderer(self))
    self.AddStory(EpicZenGardenAsm(self))
