# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry.page import shared_page_state
from telemetry import story


class StartedPage(page_module.Page):

  def __init__(self, url, page_set):
    super(StartedPage, self).__init__(
        url=url, page_set=page_set, startup_url=url,
        shared_page_state_class=shared_page_state.SharedPageState,
        name=url)

  def RunNavigateSteps(self, action_runner):
    # Do not call super.RunNavigateSteps() to avoid reloading the page that has
    # already been opened with startup_url.

    # TODO(gabadie): Get rid of this (crbug.com/555504)
    action_runner.Wait(10)

  def RunPageInteractions(self, action_runner):
    self.RunNavigateSteps(action_runner)


class StartupPagesPageSet(story.StorySet):
  """Pages for testing starting Chrome with a URL.

  Note that this file can't be used with record_wpr, since record_wpr requires
  a true navigate step, which we do not want for startup testing. Instead use
  record_wpr startup_pages_record to record data for this test."""

  def __init__(self):
    super(StartupPagesPageSet, self).__init__(
        archive_data_file='data/startup_pages.json',
        cloud_storage_bucket=story.PARTNER_BUCKET)

    # Typical page.
    self.AddStory(StartedPage('about:blank', self))
    # Typical page.
    self.AddStory(StartedPage('http://bbc.co.uk', self))


class ExperimentalStartupPagesPageSet(story.StorySet):
  """Pages for testing starting Chrome with a URL."""

  def __init__(self):
    super(ExperimentalStartupPagesPageSet, self).__init__(
        archive_data_file='data/startup_pages.json',
        cloud_storage_bucket=story.PARTNER_BUCKET)
    self.AddStory(StartedPage('http://bbc.co.uk', self))
