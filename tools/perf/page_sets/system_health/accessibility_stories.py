# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from page_sets.login_helpers import google_login

from page_sets.system_health import platforms
from page_sets.system_health import story_tags
from page_sets.system_health import system_health_story


LONG_TEXT = """Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla
suscipit enim ut nunc vestibulum, vitae porta dui eleifend. Donec
condimentum ante malesuada mi sodales maximus."""


class _AccessibilityStory(system_health_story.SystemHealthStory):
  """Abstract base class for accessibility System Health user stories."""
  ABSTRACT_STORY = True
  SUPPORTED_PLATFORMS = platforms.DESKTOP_ONLY

  def __init__(self, story_set, take_memory_measurement):
    super(_AccessibilityStory, self).__init__(
        story_set, take_memory_measurement,
        extra_browser_args=['--force-renderer-accessibility'])


class AccessibilityScrollingCodeSearchStory(_AccessibilityStory):
  """Tests scrolling an element within a page."""
  NAME = 'browse_accessibility:tech:codesearch'
  URL = 'https://cs.chromium.org/chromium/src/ui/accessibility/platform/ax_platform_node_mac.mm'
  TAGS = [story_tags.ACCESSIBILITY, story_tags.SCROLL]

  def RunNavigateSteps(self, action_runner):
    super(AccessibilityScrollingCodeSearchStory, self).RunNavigateSteps(
        action_runner)
    action_runner.WaitForElement(text='// namespace ui')
    action_runner.ScrollElement(selector='#file_scroller', distance=1000)


class AccessibilityWikipediaStory(_AccessibilityStory):
  """Wikipedia page on Accessibility. Long, but very simple, clean layout."""
  NAME = 'load_accessibility:media:wikipedia'
  URL = 'https://en.wikipedia.org/wiki/Accessibility'
  TAGS = [story_tags.ACCESSIBILITY]


class AccessibilityAmazonStory(_AccessibilityStory):
  """Amazon results page. Good example of a site with a data table."""
  NAME = 'load_accessibility:shopping:amazon'
  URL = 'https://www.amazon.com/gp/offer-listing/B01IENFJ14'
  TAGS = [story_tags.ACCESSIBILITY]


class AccessibilityGmailComposeStory(_AccessibilityStory):
  """Tests typing a lot of text into a Gmail compose window."""
  NAME = 'browse_accessibility:tools:gmail_compose'
  URL = 'https://mail.google.com/mail/#inbox?compose=new'
  TAGS = [story_tags.ACCESSIBILITY, story_tags.KEYBOARD_INPUT]

  def RunNavigateSteps(self, action_runner):
    google_login.LoginGoogleAccount(action_runner, 'googletest')

    # Navigating to https://mail.google.com immediately leads to an infinite
    # redirection loop due to a bug in WPR (see
    # https://github.com/chromium/web-page-replay/issues/70). We therefore first
    # navigate to a sub-URL to set up the session and hit the resulting
    # redirection loop. Afterwards, we can safely navigate to
    # https://mail.google.com.
    action_runner.tab.WaitForDocumentReadyStateToBeComplete()
    action_runner.Navigate(
        'https://mail.google.com/mail/mu/mp/872/trigger_redirection_loop')
    action_runner.tab.WaitForDocumentReadyStateToBeComplete()

    super(AccessibilityGmailComposeStory, self).RunNavigateSteps(
        action_runner)

    action_runner.WaitForJavaScriptCondition(
        'document.getElementById("loading").style.display === "none"')

    # Tab from the To field to the message body.
    action_runner.WaitForElement(selector='#\\:gr')
    action_runner.PressKey('Tab')
    action_runner.PressKey('Tab')

    # EnterText doesn't handle newlines for some reason.
    long_text = LONG_TEXT.replace('\n', ' ')

    # Enter some text
    action_runner.EnterText(long_text, character_delay_ms=1)

    # Move up a couple of lines and then enter it again, this causes
    # a huge amount of wrapping and re-layout
    action_runner.PressKey('Home')
    action_runner.PressKey('ArrowUp')
    action_runner.PressKey('ArrowUp')
    action_runner.EnterText(long_text, character_delay_ms=1)
