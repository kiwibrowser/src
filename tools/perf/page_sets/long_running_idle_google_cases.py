# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry import story

from page_sets import google_pages

IDLE_TIME_IN_SECONDS = 100
SAMPLING_INTERVAL_IN_SECONDS = 1
STEPS = IDLE_TIME_IN_SECONDS / SAMPLING_INTERVAL_IN_SECONDS

def _IdleAction(action_runner):
  action_runner.tab.browser.DumpMemory()
  for _ in xrange(STEPS):
    action_runner.Wait(SAMPLING_INTERVAL_IN_SECONDS)
    action_runner.tab.browser.DumpMemory()

def _CreateIdlePageClass(base_page_cls):
  class DerivedIdlePage(base_page_cls):  # pylint: disable=no-init
    def RunPageInteractions(self, action_runner):
      _IdleAction(action_runner)
  return DerivedIdlePage


def _CreateIdleBackgroundPageClass(base_page_cls):
  class DerivedIdlePage(base_page_cls):  # pylint: disable=no-init
    def RunPageInteractions(self, action_runner):
      action_runner.tab.browser.tabs.New()
      _IdleAction(action_runner)
  return DerivedIdlePage


class LongRunningIdleGmailPageSet(story.StorySet):
  def __init__(self):
    super(LongRunningIdleGmailPageSet, self).__init__(
        archive_data_file='data/long_running_idle_gmail_page.json',
        cloud_storage_bucket=story.PARTNER_BUCKET)
    self.AddStory(
        _CreateIdlePageClass(google_pages.GmailPage)(self))


class LongRunningIdleGmailBackgroundPageSet(story.StorySet):
  def __init__(self):
    # Reuse the wpr of foreground gmail.
    super(LongRunningIdleGmailBackgroundPageSet, self).__init__(
        archive_data_file='data/long_running_idle_gmail_page.json',
        cloud_storage_bucket=story.PARTNER_BUCKET)
    self.AddStory(
        _CreateIdleBackgroundPageClass(google_pages.GmailPage)(self))
