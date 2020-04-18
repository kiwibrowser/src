# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry.page import shared_page_state
from telemetry import story

from page_sets.login_helpers import google_login


class KeyDesktopMoveCasesPage(page_module.Page):

  def __init__(self, url, page_set, name=''):
    if name == '':
      name = url
    super(KeyDesktopMoveCasesPage, self).__init__(
        url=url, page_set=page_set, name=name,
        shared_page_state_class=shared_page_state.SharedDesktopPageState)


class GmailMouseScrollPage(KeyDesktopMoveCasesPage):

  """ Why: productivity, top google properties """

  def __init__(self, page_set):
    super(GmailMouseScrollPage, self).__init__(
      url='https://mail.google.com/mail/',
      page_set=page_set)

    self.scrollable_element_function = '''
      function(callback) {
        gmonkey.load('2.0', function(api) {
          callback(api.getScrollableElement());
        });
      }'''

  def RunNavigateSteps(self, action_runner):
    google_login.LoginGoogleAccount(action_runner, 'googletest')
    super(GmailMouseScrollPage, self).RunNavigateSteps(action_runner)
    action_runner.WaitForJavaScriptCondition(
        'window.gmonkey !== undefined &&'
        'document.getElementById("gb") !== null')
    # This check is needed for gmonkey to load completely.
    action_runner.WaitForJavaScriptCondition(
        'document.readyState == "complete"')

  def RunPageInteractions(self, action_runner):
    action_runner.ExecuteJavaScript('''
        gmonkey.load('2.0', function(api) {
          window.__scrollableElementForTelemetry = api.getScrollableElement();
        });''')
    action_runner.WaitForJavaScriptCondition(
        'window.__scrollableElementForTelemetry != null')
    scrollbar_x, start_y, end_y = self._CalculateScrollBarRatios(action_runner)

    with action_runner.CreateGestureInteraction('DragAction'):
      action_runner.DragPage(left_start_ratio=scrollbar_x,
          top_start_ratio=start_y, left_end_ratio=scrollbar_x,
          top_end_ratio=end_y, speed_in_pixels_per_second=100,
          element_function='window.__scrollableElementForTelemetry')

  def _CalculateScrollBarRatios(self, action_runner):
    viewport_height = float(action_runner.EvaluateJavaScript(
        'window.__scrollableElementForTelemetry.clientHeight'))
    content_height = float(action_runner.EvaluateJavaScript(
        'window.__scrollableElementForTelemetry.scrollHeight'))
    viewport_width = float(action_runner.EvaluateJavaScript(
        'window.__scrollableElementForTelemetry.offsetWidth'))
    scrollbar_width = float(action_runner.EvaluateJavaScript('''
        window.__scrollableElementForTelemetry.offsetWidth -
        window.__scrollableElementForTelemetry.scrollWidth'''))

    # This calculation is correct only when the element doesn't have border or
    # padding or scroll buttons (eg: gmail mail element).
    # Calculating the mid point of start of scrollbar.
    scrollbar_height_ratio = viewport_height / content_height
    scrollbar_start_mid_y = scrollbar_height_ratio / 2
    scrollbar_width_ratio = scrollbar_width / viewport_width
    scrollbar_mid_x_right_offset = scrollbar_width_ratio / 2
    scrollbar_mid_x = 1 - scrollbar_mid_x_right_offset

    # The End point of scrollbar (x remains same).
    scrollbar_end_mid_y = 1 - scrollbar_start_mid_y
    return scrollbar_mid_x, scrollbar_start_mid_y, scrollbar_end_mid_y


class GoogleMapsPage(KeyDesktopMoveCasesPage):

  """ Why: productivity, top google properties; Supports drag gestures """

  def __init__(self, page_set):
    super(GoogleMapsPage, self).__init__(
        url='https://www.google.co.uk/maps/@51.5043968,-0.1526806',
        page_set=page_set,
        name='Maps')

  def RunNavigateSteps(self, action_runner):
    super(GoogleMapsPage, self).RunNavigateSteps(action_runner)
    action_runner.WaitForElement(selector='.widget-scene-canvas')
    action_runner.WaitForElement(selector='.widget-zoom-in')
    action_runner.WaitForElement(selector='.widget-zoom-out')

  def RunPageInteractions(self, action_runner):
    for _ in range(3):
      action_runner.Wait(2)
      with action_runner.CreateGestureInteraction(
          'DragAction', repeatable=True):
        action_runner.DragPage(left_start_ratio=0.5, top_start_ratio=0.75,
                               left_end_ratio=0.75, top_end_ratio=0.5)
    # TODO(ssid): Add zoom gestures after fixing bug crbug.com/462214.


class KeyDesktopMoveCasesPageSet(story.StorySet):

  """ Special cases for move gesture """

  def __init__(self):
    super(KeyDesktopMoveCasesPageSet, self).__init__(
      archive_data_file='data/key_desktop_move_cases.json',
      cloud_storage_bucket=story.PARTNER_BUCKET)

    self.AddStory(GmailMouseScrollPage(self))
    self.AddStory(GoogleMapsPage(self))
