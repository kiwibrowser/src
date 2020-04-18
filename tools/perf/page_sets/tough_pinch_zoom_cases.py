# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import page as page_module
from telemetry.page import shared_page_state
from telemetry import story


class ToughPinchZoomCasesPage(page_module.Page):

  def __init__(self, url, page_set, name=''):
    if name == '':
      name = url
    super(ToughPinchZoomCasesPage, self).__init__(
        url=url, page_set=page_set, name=name,
        shared_page_state_class=shared_page_state.SharedDesktopPageState)
    self.target_scale_factor = page_set.target_scale_factor

  def RunPinchGesture(self, action_runner, left_anchor_ratio=0.5,
                      top_anchor_ratio=0.5, scale_factor=None,
                      speed_in_pixels_per_second=800):
      with action_runner.CreateGestureInteraction('PinchAction',
                                                  repeatable=True):
        action_runner.PinchPage(
            left_anchor_ratio=left_anchor_ratio,
            top_anchor_ratio=top_anchor_ratio,
            scale_factor=scale_factor,
            speed_in_pixels_per_second=speed_in_pixels_per_second)

  def RunPageInteractions(self, action_runner):
    action_runner.tab.WaitForDocumentReadyStateToBeInteractiveOrBetter()
    for _ in xrange(0, 3):
      current_scale_factor = self.target_scale_factor
      self.RunPinchGesture(action_runner, scale_factor=current_scale_factor)
      while current_scale_factor > 1.0:
        current_scale_factor *= 1/2.0
        self.RunPinchGesture(action_runner, scale_factor=1/2.0)

class GoogleSearchPage(ToughPinchZoomCasesPage):

  """ Why: top google property; a google tab is often open. """

  def __init__(self, page_set):
    super(GoogleSearchPage, self).__init__(
      url='https://www.google.com/#hl=en&q=barack+obama',
      page_set=page_set)

  def RunNavigateSteps(self, action_runner):
    super(GoogleSearchPage, self).RunNavigateSteps(action_runner)
    action_runner.WaitForElement(text='Next')


class GmailPage(ToughPinchZoomCasesPage):

  """ Why: productivity, top google properties """

  def __init__(self, page_set):
    super(GmailPage, self).__init__(
      url='https://mail.google.com/mail/',
      page_set=page_set)

  def RunNavigateSteps(self, action_runner):
    super(GmailPage, self).RunNavigateSteps(action_runner)
    action_runner.WaitForJavaScriptCondition(
        'window.gmonkey !== undefined &&'
        'document.getElementById("gb") !== null')


class GoogleCalendarPage(ToughPinchZoomCasesPage):

  """ Why: productivity, top google properties """

  def __init__(self, page_set):
    super(GoogleCalendarPage, self).__init__(
      url='https://www.google.com/calendar/',
      page_set=page_set)

  def RunNavigateSteps(self, action_runner):
    super(GoogleCalendarPage, self).RunNavigateSteps(action_runner)
    action_runner.Wait(2)

class GoogleImageSearchPage(ToughPinchZoomCasesPage):

  """ Why: tough image case; top google properties """

  def __init__(self, page_set):
    super(GoogleImageSearchPage, self).__init__(
      url='https://www.google.com/search?q=cats&tbm=isch',
      page_set=page_set)


class YoutubePage(ToughPinchZoomCasesPage):

  """ Why: #3 (Alexa global) """

  def __init__(self, page_set):
    super(YoutubePage, self).__init__(
      url='http://www.youtube.com',
      page_set=page_set)

  def RunNavigateSteps(self, action_runner):
    super(YoutubePage, self).RunNavigateSteps(action_runner)
    action_runner.Wait(2)

class BlogSpotPage(ToughPinchZoomCasesPage):

  """
  Why: #11 (Alexa global), google property; some blogger layouts have infinite
  scroll but more interesting
  """

  def __init__(self, page_set):
    super(BlogSpotPage, self).__init__(
      url='http://googlewebmastercentral.blogspot.com/',
      page_set=page_set, name='Blogger')

  def RunNavigateSteps(self, action_runner):
    super(BlogSpotPage, self).RunNavigateSteps(action_runner)
    action_runner.WaitForElement(text='accessibility')


class FacebookPage(ToughPinchZoomCasesPage):

  """ Why: top social,Public profile """

  def __init__(self, page_set):
    super(FacebookPage, self).__init__(
      url='http://www.facebook.com/barackobama',
      page_set=page_set, name='Facebook')

  def RunNavigateSteps(self, action_runner):
    super(FacebookPage, self).RunNavigateSteps(action_runner)
    action_runner.WaitForElement(text='About')


class LinkedinPage(ToughPinchZoomCasesPage):

  """ Why: #12 (Alexa global),Public profile """

  def __init__(self, page_set):
    super(LinkedinPage, self).__init__(
      url='http://www.linkedin.com/in/linustorvalds',
      page_set=page_set, name='LinkedIn')


class TwitterPage(ToughPinchZoomCasesPage):

  """ Why: #8 (Alexa global),Picked an interesting page """

  def __init__(self, page_set):
    super(TwitterPage, self).__init__(
      url='https://twitter.com/katyperry',
      page_set=page_set, name='Twitter')

  def RunNavigateSteps(self, action_runner):
    super(TwitterPage, self).RunNavigateSteps(action_runner)
    action_runner.Wait(2)

class ESPNPage(ToughPinchZoomCasesPage):

  """ Why: #1 sports """

  def __init__(self, page_set):
    super(ESPNPage, self).__init__(
      url='http://espn.go.com/nba',
      page_set=page_set, name='ESPN')


class WeatherDotComPage(ToughPinchZoomCasesPage):

  """ Why: #7 (Alexa news); #27 total time spent,Picked interesting page """

  def __init__(self, page_set):
    super(WeatherDotComPage, self).__init__(
      # pylint: disable=line-too-long
      url='http://www.weather.com/weather/right-now/Mountain+View+CA+94043',
      page_set=page_set, name='Weather.com')


class YahooGamePage(ToughPinchZoomCasesPage):

  """ Why: #1 games according to Alexa (with actual games in it) """

  def __init__(self, page_set):
    super(YahooGamePage, self).__init__(
      url='http://games.yahoo.com',
      page_set=page_set)

  def RunNavigateSteps(self, action_runner):
    super(YahooGamePage, self).RunNavigateSteps(action_runner)
    action_runner.Wait(2)


class ToughPinchZoomCasesPageSet(story.StorySet):

  """ Set of pages that are tricky to pinch-zoom """

  def __init__(self, target_scale_factor):
    super(ToughPinchZoomCasesPageSet, self).__init__(
      archive_data_file='data/tough_pinch_zoom_cases.json',
      cloud_storage_bucket=story.PARTNER_BUCKET)

    self.target_scale_factor = target_scale_factor

    self.AddStory(GoogleSearchPage(self))
    self.AddStory(GmailPage(self))
    self.AddStory(GoogleCalendarPage(self))
    self.AddStory(GoogleImageSearchPage(self))
    self.AddStory(YoutubePage(self))
    self.AddStory(BlogSpotPage(self))
    self.AddStory(FacebookPage(self))
    self.AddStory(LinkedinPage(self))
    self.AddStory(TwitterPage(self))
    self.AddStory(ESPNPage(self))

    # Why: #1 news worldwide (Alexa global)
    self.AddStory(ToughPinchZoomCasesPage('http://news.yahoo.com', self))

    # Why: #2 news worldwide
    self.AddStory(ToughPinchZoomCasesPage('http://www.cnn.com', self))

    self.AddStory(WeatherDotComPage(self))

    # Why: #1 world commerce website by visits; #3 commerce in the US by time
    # spent
    self.AddStory(ToughPinchZoomCasesPage('http://www.amazon.com', self))

    # Why: #1 commerce website by time spent by users in US
    self.AddStory(ToughPinchZoomCasesPage('http://www.ebay.com', self))

    self.AddStory(YahooGamePage(self))

    # Why: #1 Alexa recreation
    self.AddStory(ToughPinchZoomCasesPage('http://booking.com', self))

    # Why: #1 Alexa sports
    self.AddStory(ToughPinchZoomCasesPage('http://sports.yahoo.com/', self))


class AndroidToughPinchZoomCasesPageSet(ToughPinchZoomCasesPageSet):

  """
  ToughPinchZoomCasesPageSet using the maximum Android zoom level. This is
  chosen as 7x, which may seem to exceed the 5x value specified in
  WebPreferences::default_maximum_page_scale_factor. However, as desktop sites
  on Android start at less than 1x scale (up to 0.25x), a value of 7x does not
  exceed the 5x limit.
  """

  def __init__(self):
    super(AndroidToughPinchZoomCasesPageSet, self).__init__(7.0)


class DesktopToughPinchZoomCasesPageSet(ToughPinchZoomCasesPageSet):

  """ ToughPinchZoomCasesPageSet using the maximum desktop zoom level """

  def __init__(self):
    super(DesktopToughPinchZoomCasesPageSet, self).__init__(4.0)
