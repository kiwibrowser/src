# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import urlparse

from common import inspector_network
from common import network_metrics
from telemetry import decorators
from telemetry.testing import tab_test_case


class InspectorNetworkTabTest(tab_test_case.TabTestCase):
  class TestCase(object):
    def __init__(self, responses_count=0,
                 subresources=None):
      # Number of responses expected for this case.
      self.responses_count = responses_count
      # List of subresource links for this case.
      self.subresources = subresources

  def __init__(self, *args):
    super(InspectorNetworkTabTest, self).__init__(*args)

  def _FilterFaviconEvents(self, events):
    for event in events:
      if 'favicon.ico' in event.args['response']['url']:
        events.remove(event)
    return events

  def _NavigateAndGetHTTPResponseEvents(self, page):
    network = inspector_network.InspectorNetwork(
        self._tab._inspector_backend._websocket)
    network.timeline_recorder.Start()
    self.Navigate(page)
    timeline_model = network.timeline_recorder.Stop()
    self.assertTrue(timeline_model)
    return timeline_model.GetAllEventsOfName('HTTPResponse')

  def testHTTPResponseTimelineRecorder(self):
    tests = {
        'blank.html': InspectorNetworkTabTest.TestCase(responses_count=1),
        'green_rect.html': InspectorNetworkTabTest.TestCase(responses_count=1),
        'image_decoding.html': InspectorNetworkTabTest.TestCase(
            responses_count=2, subresources=['image.png']),
        }
    for page, test in tests.iteritems():
      events = self._FilterFaviconEvents(
          self._NavigateAndGetHTTPResponseEvents(page))
      self.assertEqual(test.responses_count, len(events))

      # Verify required event fields
      for event in events:
        self.assertEqual('HTTPResponse', event.name)
        resp = inspector_network.InspectorNetworkResponseData.FromTimelineEvent(
            event)
        self.assertLess(0.0, resp.timestamp)

        self.assertTrue(resp.headers)
        self.assertTrue(resp.headers['Content-Length'])
        body, base64_encoded = resp.GetBody()

        link = resp.url[resp.url.rfind('/') + 1 :]
        self.assertTrue(link == page or link in test.subresources)
        if link == page:
          self.assertEqual(resp.GetHeader('Content-Type'), 'text/html')
          self.assertTrue('<!DOCTYPE HTML>' in body)
          self.assertFalse(base64_encoded)
          self.assertEqual('other', resp.initiator['type'])
        else:
          # We know this is the only subresource type in our setup.
          self.assertEqual(resp.GetHeader('Content-Type'), 'image/png')
          self.assertFalse('<!DOCTYPE HTML>' in body)
          self.assertTrue(base64_encoded)
          self.assertEqual('parser', resp.initiator['type'])

  def testNetworkTiming(self):
    test = (
        'image_decoding.html', InspectorNetworkTabTest.TestCase(
            responses_count=2, subresources=['image.png'])
        )

    network = inspector_network.InspectorNetwork(
        self._tab._inspector_backend._websocket)
    network.StartMonitoringNetwork()
    self.Navigate(test[0])
    network.StopMonitoringNetwork()
    response_data = network.GetResponseData()
    path_to_response = {urlparse.urlparse(r.url).path: r
                        for r in response_data}
    self.assertTrue('/image_decoding.html' in path_to_response)
    self.assertTrue(
        path_to_response['/image_decoding.html'].timing['requestTime'] > 0)
    self.assertTrue(
        path_to_response['/image_decoding.html'].timing['loadingFinished'] > 0)
    self.assertTrue('/image.png' in path_to_response)
    self.assertTrue(
        path_to_response['/image.png'].timing['requestTime'] > 0)
    self.assertTrue(
        path_to_response['/image.png'].timing['loadingFinished'] > 0)

  # Flaky on many platforms (at least Win, Linux, and Mac).
  # http://crbug.com/424706
  @decorators.Disabled('all')
  def testCacheableHTTPResponse(self):
    # We know this page has one PNG image and its cacheable.
    events = self._NavigateAndGetHTTPResponseEvents('image_decoding.html')
    images_first = []
    for event in events:
      resp = inspector_network.InspectorNetworkResponseData.FromTimelineEvent(
          event)
      if resp.GetHeader('Content-Type') == 'image/png':
        images_first.append(resp)

    self.assertEqual(1, len(images_first))
    self.assertFalse(images_first[0].served_from_cache)

    events = self._NavigateAndGetHTTPResponseEvents('image_decoding.html')
    images_second = []
    for event in events:
      resp = inspector_network.InspectorNetworkResponseData.FromTimelineEvent(
          event)
      if resp.GetHeader('Content-Type') == 'image/png':
        images_second.append(resp)
    self.assertEqual(1, len(images_second))
    # On the second fetch, the image is served from cache.
    self.assertTrue(images_second[0].served_from_cache)
