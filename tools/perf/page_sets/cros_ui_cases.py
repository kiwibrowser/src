# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry.page import shared_page_state
from telemetry import story

class _SharedPageState(shared_page_state.SharedDesktopPageState):

  def CanRunOnBrowser(self, browser_info, page):
    if not hasattr(page, 'CanRunOnBrowser'):
      return True
    return page.CanRunOnBrowser(browser_info.browser)


class DesktopUIPage(page_module.Page):

  def __init__(self, url, page_set, name):
    super(DesktopUIPage, self).__init__(
        url=url,
        page_set=page_set,
        name=name,
        shared_page_state_class=_SharedPageState,
        extra_browser_args=['--always-request-presentation-time'])


class OverviewMode(DesktopUIPage):

  def CanRunOnBrowser(self, browser):
    return browser.supports_overview_mode

  def RunPageInteractions(self, action_runner):
    action_runner.Wait(1)
    # TODO(chiniforooshan): CreateInteraction creates an async event in the
    # renderer, which works fine; it is nicer if we create UI interaction
    # records in the browser process.
    with action_runner.CreateInteraction('ui_EnterOverviewAction'):
      action_runner.EnterOverviewMode()
      # TODO(chiniforooshan): The follwoing wait, and the one after
      # ExitOverviewMode(), is a workaround for crbug.com/788454. Remove when
      # the bug is fixed.
      action_runner.Wait(1)
    action_runner.Wait(0.5)
    with action_runner.CreateInteraction('ui_ExitOverviewAction'):
      action_runner.ExitOverviewMode()
      action_runner.Wait(1)


class CrosUiCasesPageSet(story.StorySet):
  """Pages that test desktop UI performance."""

  def __init__(self):
    super(CrosUiCasesPageSet, self).__init__(
      archive_data_file='data/cros_ui_cases.json',
      cloud_storage_bucket=story.PARTNER_BUCKET)

    self.AddStory(OverviewMode(
        'http://news.yahoo.com', self, 'overview:yahoo_news'))
    self.AddStory(OverviewMode(
        'http://jsbin.com/giqafofe/1/quiet?JS_POSTER_CIRCLE', self,
        'overview:js_poster_circle'))
