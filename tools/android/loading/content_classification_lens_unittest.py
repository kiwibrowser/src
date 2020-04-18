# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import copy
import unittest

from content_classification_lens import (ContentClassificationLens,
                                         _RulesMatcher)
from request_track import Request
import test_utils


class ContentClassificationLensTestCase(unittest.TestCase):
  _DOCUMENT_URL = 'http://bla.com'
  _MAIN_FRAME_ID = '123.1'
  _REQUEST = Request.FromJsonDict({'url': _DOCUMENT_URL,
                                   'document_url': _DOCUMENT_URL,
                                   'request_id': '1234.1',
                                   'frame_id': _MAIN_FRAME_ID,
                                   'initiator': {'type': 'other'},
                                   'timestamp': 2,
                                   'status': 200,
                                   'timing': {},
                                   'resource_type': 'Document'})
  _PAGE_EVENTS = [{'method': 'Page.frameStartedLoading',
                   'frame_id': _MAIN_FRAME_ID},
                  {'method': 'Page.frameAttached',
                   'frame_id': '123.13', 'parent_frame_id': _MAIN_FRAME_ID}]
  _RULES = ['bla.com']

  def testGetDocumentUrl(self):
    trace = test_utils.LoadingTraceFromEvents(
        [self._REQUEST], self._PAGE_EVENTS)
    lens = ContentClassificationLens(trace, [], [])
    self.assertEquals(self._DOCUMENT_URL, lens._GetDocumentUrl())
    # Don't be fooled by redirects.
    request = copy.deepcopy(self._REQUEST)
    request.status = 302
    request.document_url = 'http://www.bla.com'
    trace = test_utils.LoadingTraceFromEvents(
        [request, self._REQUEST], self._PAGE_EVENTS)
    lens = ContentClassificationLens(trace, [], [])
    self.assertEquals(self._DOCUMENT_URL, lens._GetDocumentUrl())

  def testGetDocumentUrlSeveralChanges(self):
    request = copy.deepcopy(self._REQUEST)
    request.status = 200
    request.document_url = 'http://www.blabla.com'
    request2 = copy.deepcopy(request)
    request2.document_url = 'http://www.blablabla.com'
    trace = test_utils.LoadingTraceFromEvents(
        [self._REQUEST, request, request2], self._PAGE_EVENTS)
    lens = ContentClassificationLens(trace, [], [])
    self.assertEquals(request2.document_url, lens._GetDocumentUrl())

  def testNoRules(self):
    trace = test_utils.LoadingTraceFromEvents(
        [self._REQUEST], self._PAGE_EVENTS)
    lens = ContentClassificationLens(trace, [], [])
    self.assertFalse(lens.IsAdRequest(self._REQUEST))
    self.assertFalse(lens.IsTrackingRequest(self._REQUEST))

  def testAdRequest(self):
    trace = test_utils.LoadingTraceFromEvents(
        [self._REQUEST], self._PAGE_EVENTS)
    lens = ContentClassificationLens(trace, self._RULES, [])
    self.assertTrue(lens.IsAdRequest(self._REQUEST))
    self.assertFalse(lens.IsTrackingRequest(self._REQUEST))

  def testTrackingRequest(self):
    trace = test_utils.LoadingTraceFromEvents(
        [self._REQUEST], self._PAGE_EVENTS)
    lens = ContentClassificationLens(trace, [], self._RULES)
    self.assertFalse(lens.IsAdRequest(self._REQUEST))
    self.assertTrue(lens.IsTrackingRequest(self._REQUEST))

  def testMainFrameIsNotAnAdFrame(self):
    trace = test_utils.LoadingTraceFromEvents(
        [self._REQUEST], self._PAGE_EVENTS)
    lens = ContentClassificationLens(trace, self._RULES, [])
    self.assertFalse(lens.IsAdOrTrackingFrame(self._MAIN_FRAME_ID))

  def testAdFrame(self):
    request = copy.deepcopy(self._REQUEST)
    request.request_id = '1234.2'
    request.frame_id = '123.123'
    trace = test_utils.LoadingTraceFromEvents(
        [self._REQUEST, request], self._PAGE_EVENTS)
    lens = ContentClassificationLens(trace, self._RULES, [])
    self.assertTrue(lens.IsAdOrTrackingFrame(request.frame_id))

  def testAdAndTrackingRequests(self):
    ad_request = copy.deepcopy(self._REQUEST)
    ad_request.request_id = '1234.2'
    ad_request.frame_id = '123.123'
    non_ad_request_non_ad_frame = copy.deepcopy(self._REQUEST)
    non_ad_request_non_ad_frame.request_id = '1234.3'
    non_ad_request_non_ad_frame.url = 'http://www.example.com'
    non_ad_request_non_ad_frame.frame_id = '123.456'
    non_ad_request_ad_frame = copy.deepcopy(self._REQUEST)
    non_ad_request_ad_frame.request_id = '1234.4'
    non_ad_request_ad_frame.url = 'http://www.example.com'
    non_ad_request_ad_frame.frame_id = ad_request.frame_id

    trace = test_utils.LoadingTraceFromEvents(
        [self._REQUEST, ad_request, non_ad_request_non_ad_frame,
         non_ad_request_ad_frame], self._PAGE_EVENTS)
    lens = ContentClassificationLens(trace, self._RULES, [])
    self.assertSetEqual(
        set([self._REQUEST, ad_request, non_ad_request_ad_frame]),
        set(lens.AdAndTrackingRequests()))


class _MatcherTestCase(unittest.TestCase):
  _RULES_WITH_WHITELIST = ['/thisisanad.', '@@myadvertisingdomain.com/*',
                           '@@||www.mydomain.com/ads/$elemhide']
  _SCRIPT_RULE = 'domainwithscripts.com/*$script'
  _THIRD_PARTY_RULE = 'domainwithscripts.com/*$third-party'
  _SCRIPT_REQUEST = Request.FromJsonDict(
      {'url': 'http://domainwithscripts.com/bla.js',
       'resource_type': 'Script',
       'request_id': '1234.1',
       'frame_id': '123.1',
       'initiator': {'type': 'other'},
       'timestamp': 2,
       'timing': {}})

  def testRemovesWhitelistRules(self):
    matcher = _RulesMatcher(self._RULES_WITH_WHITELIST, False)
    self.assertEquals(3, len(matcher._rules))
    matcher = _RulesMatcher(self._RULES_WITH_WHITELIST, True)
    self.assertEquals(1, len(matcher._rules))

  def testScriptRule(self):
    matcher = _RulesMatcher([self._SCRIPT_RULE], False)
    request = copy.deepcopy(self._SCRIPT_REQUEST)
    request.resource_type = 'Stylesheet'
    self.assertFalse(matcher.Matches(
        request, ContentClassificationLensTestCase._DOCUMENT_URL))
    self.assertTrue(matcher.Matches(
        self._SCRIPT_REQUEST, ContentClassificationLensTestCase._DOCUMENT_URL))

  def testGetTldPlusOne(self):
    self.assertEquals(
        'easy.com',
        _RulesMatcher._GetTldPlusOne('http://www.easy.com/hello/you'))
    self.assertEquals(
        'not-so-easy.co.uk',
        _RulesMatcher._GetTldPlusOne('http://www.not-so-easy.co.uk/hello/you'))
    self.assertEquals(
        'hard.co.uk',
        _RulesMatcher._GetTldPlusOne('http://hard.co.uk/'))

  def testThirdPartyRule(self):
    matcher = _RulesMatcher([self._THIRD_PARTY_RULE], False)
    request = copy.deepcopy(self._SCRIPT_REQUEST)
    document_url = 'http://www.domainwithscripts.com/good-morning'
    self.assertFalse(matcher.Matches(request, document_url))
    document_url = 'http://anotherdomain.com/good-morning'
    self.assertTrue(matcher.Matches(request, document_url))


if __name__ == '__main__':
  unittest.main()
