# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page
from telemetry.page import shared_page_state
from telemetry import story

from page_sets import top_pages


class Top25PageSet(story.StorySet):

  """ Page set consists of top 25 pages with only navigation actions. """

  def __init__(self):
    super(Top25PageSet, self).__init__(
        archive_data_file='data/top_25.json',
        cloud_storage_bucket=story.PARTNER_BUCKET)

    shared_desktop_state = shared_page_state.SharedDesktopPageState
    self.AddStory(top_pages.GoogleWebSearchPage(self, shared_desktop_state))
    self.AddStory(top_pages.GmailPage(self, shared_desktop_state))
    self.AddStory(top_pages.GoogleCalendarPage(self, shared_desktop_state))
    self.AddStory(
        top_pages.GoogleImageSearchPage(self, shared_desktop_state))
    self.AddStory(top_pages.GoogleDocPage(self, shared_desktop_state))
    self.AddStory(top_pages.GooglePlusPage(self, shared_desktop_state))
    self.AddStory(top_pages.YoutubePage(self, shared_desktop_state))
    self.AddStory(top_pages.BlogspotPage(self, shared_desktop_state))
    self.AddStory(top_pages.WordpressPage(self, shared_desktop_state))
    self.AddStory(top_pages.FacebookPage(self, shared_desktop_state))
    self.AddStory(top_pages.LinkedinPage(self, shared_desktop_state))
    self.AddStory(top_pages.WikipediaPage(self, shared_desktop_state))
    self.AddStory(top_pages.TwitterPage(self, shared_desktop_state))
    self.AddStory(top_pages.PinterestPage(self, shared_desktop_state))
    self.AddStory(top_pages.ESPNPage(self, shared_desktop_state))
    self.AddStory(top_pages.WeatherPage(self, shared_desktop_state))
    self.AddStory(top_pages.YahooGamesPage(self, shared_desktop_state))

    other_urls = [
        # Why: #1 news worldwide (Alexa global)
        'http://news.yahoo.com',
        # Why: #2 news worldwide
        'http://www.cnn.com',
        # Why: #1 world commerce website by visits; #3 commerce in the US by
        # time spent
        'http://www.amazon.com',
        # Why: #1 commerce website by time spent by users in US
        'http://www.ebay.com',
        # Why: #1 Alexa recreation
        'http://booking.com',
        # Why: #1 Alexa reference
        'http://answers.yahoo.com',
        # Why: #1 Alexa sports
        'http://sports.yahoo.com/',
        # Why: top tech blog
        'http://techcrunch.com'
    ]

    for url in other_urls:
      self.AddStory(
          page.Page(url, self, shared_page_state_class=shared_desktop_state,
                    name=url))
