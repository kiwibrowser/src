# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Labels requests according to the type of content they represent."""

import collections
import logging
import operator
import os
import urlparse

import loading_trace
import request_track


class ContentClassificationLens(object):
  """Associates requests and frames with the type of content they represent."""
  def __init__(self, trace, ad_rules, tracking_rules):
    """Initializes an instance of ContentClassificationLens.

    Args:
      trace: (LoadingTrace) loading trace.
      ad_rules: ([str]) List of Adblock+ compatible rules used to classify ads.
      tracking_rules: ([str]) List of Adblock+ compatible rules used to
                      classify tracking and analytics.
    """
    self._trace = trace
    self._requests = trace.request_track.GetEvents()
    self._requests_by_id = {r.request_id: r for r in self._requests}
    self._main_frame_id = trace.page_track.GetEvents()[0]['frame_id']
    self._frame_to_requests = collections.defaultdict(list)
    self._ad_requests = set()
    self._tracking_requests = set()
    self._ad_matcher = _RulesMatcher(ad_rules, True)
    self._tracking_matcher = _RulesMatcher(tracking_rules, True)
    self._document_url = self._GetDocumentUrl()
    self._GroupRequestsByFrameId()
    self._LabelRequests()

  def IsAdRequest(self, request):
    """Returns True iff the request matches one of the ad_rules."""
    return request.request_id in self._ad_requests

  def IsTrackingRequest(self, request):
    """Returns True iff the request matches one of the tracking_rules."""
    return request.request_id in self._tracking_requests

  def IsAdOrTrackingFrame(self, frame_id):
    """A Frame is an Ad frame if it's not the main frame and its main resource
    is ad or tracking-related.
    """
    if (frame_id not in self._frame_to_requests
        or frame_id == self._main_frame_id):
      return False
    frame_requests = [self._requests_by_id[request_id]
                      for request_id in self._frame_to_requests[frame_id]]
    sorted_frame_resources = sorted(
        frame_requests, key=operator.attrgetter('start_msec'))
    frame_main_resource = sorted_frame_resources[0]
    return (frame_main_resource.request_id in self._ad_requests
            or frame_main_resource.request_id in self._tracking_requests)

  def AdAndTrackingRequests(self):
    """Returns a list of requests linked to ads and tracking.

    Returns the union of:
    - Requests tagged as ad or tracking.
    - Requests originating from an ad frame.
    """
    frame_ids = {r.frame_id for r in self._requests}
    ad_frame_ids = filter(self.IsAdOrTrackingFrame, frame_ids)
    return filter(lambda r: self.IsAdRequest(r) or self.IsTrackingRequest(r)
                  or r.frame_id in ad_frame_ids, self._requests)

  @classmethod
  def WithRulesFiles(cls, trace, ad_rules_filename, tracking_rules_filename):
    """Returns an instance of ContentClassificationLens with the rules read
    from files.
    """
    ad_rules = []
    tracking_rules = []
    if os.path.exists(ad_rules_filename):
      ad_rules = open(ad_rules_filename, 'r').readlines()
    if os.path.exists(tracking_rules_filename):
      tracking_rules = open(tracking_rules_filename, 'r').readlines()
    return ContentClassificationLens(trace, ad_rules, tracking_rules)

  def _GroupRequestsByFrameId(self):
    for request in self._requests:
      frame_id = request.frame_id
      self._frame_to_requests[frame_id].append(request.request_id)

  def _LabelRequests(self):
    for request in self._requests:
      request_id = request.request_id
      if self._ad_matcher.Matches(request, self._document_url):
        self._ad_requests.add(request_id)
      if self._tracking_matcher.Matches(request, self._document_url):
        self._tracking_requests.add(request_id)

  def _GetDocumentUrl(self):
    main_frame_id = self._trace.page_track.GetMainFrameId()
    # Take the last one as JS redirects can change the document URL.
    document_url = None
    for r in self._requests:
      # 304: not modified.
      if r.frame_id == main_frame_id and r.status in (200, 304):
        document_url = r.document_url
    return document_url


class _RulesMatcher(object):
  """Matches requests with rules in Adblock+ format."""
  _WHITELIST_PREFIX = '@@'
  _RESOURCE_TYPE_TO_OPTIONS_KEY = {
      'Script': 'script', 'Stylesheet': 'stylesheet', 'Image': 'image',
      'XHR': 'xmlhttprequest'}
  def __init__(self, rules, no_whitelist):
    """Initializes an instance of _RulesMatcher.

    Args:
      rules: ([str]) list of rules.
      no_whitelist: (bool) Whether the whitelisting rules should be ignored.
    """
    self._rules = self._FilterRules(rules, no_whitelist)
    if self._rules:
      try:
        import adblockparser
        self._matcher = adblockparser.AdblockRules(self._rules)
      except ImportError:
        logging.critical('Likely you need to install adblockparser. Try:\n'
                         ' pip install --user adblockparser\n'
                         'For 10-100x better performance, also try:\n'
                         " pip install --user 're2 >= 0.2.21'")
        raise
    else:
      self._matcher = None

  def Matches(self, request, document_url):
    """Returns whether a request matches one of the rules."""
    if self._matcher is None:
      return False
    url = request.url
    return self._matcher.should_block(
        url, self._GetOptions(request, document_url))

  @classmethod
  def _GetOptions(cls, request, document_url):
    options = {}
    resource_type = request.resource_type
    option = cls._RESOURCE_TYPE_TO_OPTIONS_KEY.get(resource_type)
    if option:
      options[option] = True
    if cls._IsThirdParty(request.url, document_url):
      options['third-party'] = True
    return options

  @classmethod
  def _FilterRules(cls, rules, no_whitelist):
    if not no_whitelist:
      return rules
    else:
      return [rule for rule in rules
              if not rule.startswith(cls._WHITELIST_PREFIX)]

  @classmethod
  def _IsThirdParty(cls, url, document_url):
    # Common definition of "third-party" is "not from the same TLD+1".
    # Unfortunately, knowing what is a TLD is not trivial. To do it without a
    # database, we use the following simple (and incorrect) rules:
    # - co.{in,uk,jp,hk} is a TLD
    # - com.{au,hk} is a TLD
    # Otherwise, this is the part after the last dot.
    return cls._GetTldPlusOne(url) != cls._GetTldPlusOne(document_url)

  @classmethod
  def _GetTldPlusOne(cls, url):
    hostname = urlparse.urlparse(url).hostname
    if not hostname:
      return hostname
    parts = hostname.split('.')
    if len(parts) <= 2:
      return hostname
    tld_parts_count = 1
    may_be_tld = parts[-2:]
    if may_be_tld[0] == 'co' and may_be_tld[1] in ('in', 'uk', 'jp'):
      tld_parts_count = 2
    elif may_be_tld[0] == 'com' and may_be_tld[1] in ('au', 'hk'):
      tld_parts_count = 2
    tld_plus_one = '.'.join(parts[-(tld_parts_count + 1):])
    return tld_plus_one
