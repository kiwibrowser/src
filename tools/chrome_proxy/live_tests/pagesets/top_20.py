# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry.page import page as page_module
from telemetry import story


class Top20Page(page_module.Page):

  def __init__(self, url, page_set, name=''):
    super(Top20Page, self).__init__(url=url, page_set=page_set, name=name)
    self.archive_data_file = '../data/chrome_proxy_top_20.json'

class Top20StorySet(story.StorySet):

  """ Pages hand-picked for Chrome Proxy tests. """

  def __init__(self):
    super(Top20StorySet, self).__init__(
      archive_data_file='../data/chrome_proxy_top_20.json')

    # Why: top google property; a google tab is often open
    self.AddStory(Top20Page('https://www.google.com/#hl=en&q=barack+obama',
                                self))

    # Why: #3 (Alexa global)
    self.AddStory(Top20Page('http://www.youtube.com', self))

    # Why: #18 (Alexa global), Picked an interesting post
    self.AddStory(Top20Page(
      # pylint: disable=C0301
      'http://en.blog.wordpress.com/2012/09/04/freshly-pressed-editors-picks-for-august-2012/',
      self, 'Wordpress'))

    # Why: top social,Public profile
    self.AddStory(Top20Page('http://www.facebook.com/barackobama', self,
                                'Facebook'))

    # Why: #12 (Alexa global),Public profile
    self.AddStory(Top20Page('http://www.linkedin.com/in/linustorvalds',
                                self, 'LinkedIn'))

    # Why: #6 (Alexa) most visited worldwide,Picked an interesting page
    self.AddStory(Top20Page('http://en.wikipedia.org/wiki/Wikipedia', self,
                                'Wikipedia_(1_tab)'))

    # Why: #8 (Alexa global),Picked an interesting page
    self.AddStory(Top20Page('https://twitter.com/katyperry', self,
                                'Twitter'))

    # Why: #37 (Alexa global)
    self.AddStory(Top20Page('http://pinterest.com', self, 'Pinterest'))

    # Why: #1 sports
    self.AddStory(Top20Page('http://espn.go.com', self, 'ESPN'))

    # Why: #1 news worldwide (Alexa global)
    self.AddStory(Top20Page('http://news.yahoo.com', self))

    # Why: #2 news worldwide
    self.AddStory(Top20Page('http://www.cnn.com', self))

    # Why: #7 (Alexa news); #27 total time spent,Picked interesting page
    self.AddStory(Top20Page(
      'http://www.weather.com/weather/right-now/Mountain+View+CA+94043',
      self, 'Weather.com'))

    # Why: #1 world commerce website by visits; #3 commerce in the US by time
    # spent
    self.AddStory(Top20Page('http://www.amazon.com', self))

    # Why: #1 commerce website by time spent by users in US
    self.AddStory(Top20Page('http://www.ebay.com', self))

    # Why: #1 games according to Alexa (with actual games in it)
    self.AddStory(Top20Page('http://games.yahoo.com', self))

    # Why: #1 Alexa recreation
    self.AddStory(Top20Page('http://booking.com', self))

    # Why: #1 Alexa reference
    self.AddStory(Top20Page('http://answers.yahoo.com', self))

    # Why: #1 Alexa sports
    self.AddStory(Top20Page('http://sports.yahoo.com/', self))

    # Why: top tech blog
    self.AddStory(Top20Page('http://techcrunch.com', self))

    self.AddStory(Top20Page('http://www.nytimes.com', self))
