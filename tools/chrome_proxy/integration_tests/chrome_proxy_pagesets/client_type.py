# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from common.chrome_proxy_shared_page_state import ChromeProxySharedPageState
from telemetry.page import page as page_module
from telemetry import story


class ClientTypePage(page_module.Page):
  """A test page for the chrome proxy client type tests.

  Attributes:
      bypass_for_client_type: The client type Chrome-Proxy header directive that
          would get a bypass when this page is fetched through the data
          reduction proxy. For example, a value of "android" means that this
          page would cause a bypass when fetched from a client that sets
          "Chrome-Proxy: c=android".
  """

  def __init__(self, url, page_set, bypass_for_client_type):
    super(ClientTypePage, self).__init__(url=url, page_set=page_set,
        shared_page_state_class=ChromeProxySharedPageState)
    self.bypass_for_client_type = bypass_for_client_type


class ClientTypeStorySet(story.StorySet):
  """Chrome proxy test sites"""

  def __init__(self):
    super(ClientTypeStorySet, self).__init__()

    # Page that should not bypass for any client types. This page is here in
    # order to determine the Chrome-Proxy client type value before running any
    # of the following pages, since there's no way to get the client type value
    # from a request that was bypassed.
    self.AddStory(ClientTypePage(
        url='http://check.googlezip.net/test.html',
        page_set=self,
        bypass_for_client_type='none'))

    # Page that should cause a bypass for android chrome clients.
    self.AddStory(ClientTypePage(
        url='http://check.googlezip.net/chrome-proxy-header/c_android/',
        page_set=self,
        bypass_for_client_type='android'))

    # Page that should cause a bypass for android webview clients.
    self.AddStory(ClientTypePage(
        url='http://check.googlezip.net/chrome-proxy-header/c_webview/',
        page_set=self,
        bypass_for_client_type='webview'))

    # Page that should cause a bypass for iOS clients.
    self.AddStory(ClientTypePage(
        url='http://check.googlezip.net/chrome-proxy-header/c_ios/',
        page_set=self,
        bypass_for_client_type='ios'))

    # Page that should cause a bypass for Linux clients.
    self.AddStory(ClientTypePage(
        url='http://check.googlezip.net/chrome-proxy-header/c_linux/',
        page_set=self,
        bypass_for_client_type='linux'))

    # Page that should cause a bypass for Windows clients.
    self.AddStory(ClientTypePage(
        url='http://check.googlezip.net/chrome-proxy-header/c_win/',
        page_set=self,
        bypass_for_client_type='win'))

    # Page that should cause a bypass for ChromeOS clients.
    self.AddStory(ClientTypePage(
        url='http://check.googlezip.net/chrome-proxy-header/c_chromeos/',
        page_set=self,
        bypass_for_client_type='chromeos'))
