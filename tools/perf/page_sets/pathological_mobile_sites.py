# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry.page import shared_page_state
from telemetry import story


class PathologicalMobileSitesPage(page_module.Page):

  def __init__(self, url, page_set):
    super(PathologicalMobileSitesPage, self).__init__(
        url=url, page_set=page_set,
        shared_page_state_class=shared_page_state.SharedMobilePageState,
        name=url)

  def RunPageInteractions(self, action_runner):
    with action_runner.CreateGestureInteraction('ScrollAction'):
      action_runner.ScrollPage()


class PathologicalMobileSitesPageSet(story.StorySet):

  """Pathologically bad and janky sites on mobile."""

  def __init__(self):
    super(PathologicalMobileSitesPageSet, self).__init__(
        archive_data_file='data/pathological_mobile_sites.json',
        cloud_storage_bucket=story.PARTNER_BUCKET)

    sites = ['http://edition.cnn.com',
             'http://m.espn.go.com/nhl/rankings',
             'http://recode.net',
             'http://sports.yahoo.com/',
             'http://www.latimes.com',
             ('http://www.pbs.org/newshour/bb/'
              'much-really-cost-live-city-like-seattle/#the-rundown'),
             ('http://www.theguardian.com/politics/2015/mar/09/'
              'ed-balls-tory-spending-plans-nhs-charging'),
             'http://www.zdnet.com',
             'http://www.wowwiki.com/World_of_Warcraft:_Mists_of_Pandaria',
             'https://www.linkedin.com/in/linustorvalds']

    for site in sites:
      self.AddStory(PathologicalMobileSitesPage(site, self))
