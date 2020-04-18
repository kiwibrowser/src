# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from common import network_metrics
from telemetry.page import legacy_page_test
from telemetry.value import scalar


CHROME_PROXY_VIA_HEADER = 'Chrome-Compression-Proxy'


class ChromeProxyMetricException(legacy_page_test.MeasurementFailure):
  pass


class ChromeProxyResponse(network_metrics.HTTPResponse):
  """ Represents an HTTP response from a timeline event."""
  def __init__(self, event):
    super(ChromeProxyResponse, self).__init__(event)

  def ShouldHaveChromeProxyViaHeader(self):
    resp = self.response
    # Ignore https and data url
    if resp.url.startswith('https') or resp.url.startswith('data:'):
      return False
    # Ignore 304 Not Modified and cache hit.
    if resp.status == 304 or resp.served_from_cache:
      return False
    # Ignore invalid responses that don't have any header. Log a warning.
    if not resp.headers:
      logging.warning('response for %s does not any have header '
                      '(refer=%s, status=%s)',
                      resp.url, resp.GetHeader('Referer'), resp.status)
      return False
    return True

  def HasResponseHeader(self, key, value):
    response_header = self.response.GetHeader(key)
    if not response_header:
      return False
    values = [v.strip() for v in response_header.split(',')]
    return any(v == value for v in values)

  def HasRequestHeader(self, key, value):
    if key not in self.response.request_headers:
      return False
    request_header = self.response.request_headers[key]
    values = [v.strip() for v in request_header.split(',')]
    return any(v == value for v in values)

  def HasChromeProxyViaHeader(self):
    via_header = self.response.GetHeader('Via')
    if not via_header:
      return False
    vias = [v.strip(' ') for v in via_header.split(',')]
    # The Via header is valid if it has a 4-character version prefix followed by
    # the proxy name, for example, "1.1 Chrome-Compression-Proxy".
    return any(v[4:] == CHROME_PROXY_VIA_HEADER for v in vias)

  def HasExtraViaHeader(self, extra_header):
    return self.HasResponseHeader('Via', extra_header)

  def IsValidByViaHeader(self):
    return (not self.ShouldHaveChromeProxyViaHeader() or
            self.HasChromeProxyViaHeader())

  def GetChromeProxyRequestHeaderValue(self, key):
    """Get a specific Chrome-Proxy request header value.

    Returns:
        The value for a specific Chrome-Proxy request header value for a
        given key. Returns None if no such key is present.
    """
    if 'Chrome-Proxy' not in self.response.request_headers:
      return None

    chrome_proxy_request_header = self.response.request_headers['Chrome-Proxy']
    values = [v.strip() for v in chrome_proxy_request_header.split(',')]
    for value in values:
      kvp = value.split('=', 1)
      if len(kvp) == 2 and kvp[0].strip() == key:
        return kvp[1].strip()
    return None

  def GetChromeProxyClientType(self):
    """Get the client type directive from the Chrome-Proxy request header.

    Returns:
        The client type directive from the Chrome-Proxy request header for the
        request that lead to this response. For example, if the request header
        "Chrome-Proxy: c=android" is present, then this method would return
        "android". Returns None if no client type directive is present.
    """
    return self.GetChromeProxyRequestHeaderValue('c')

  def HasChromeProxyLoFiRequest(self):
    return self.HasRequestHeader('Chrome-Proxy-Accept-Transform', "empty-image")

  def HasChromeProxyLoFiResponse(self):
    return self.HasResponseHeader('Chrome-Proxy-Content-Transform',
                                  "empty-image")

  def HasChromeProxyLitePageRequest(self):
    return self.HasRequestHeader('Chrome-Proxy-Accept-Transform', "lite-page")

  def HasChromeProxyLitePageExpRequest(self):
    return self.HasRequestHeader('Chrome-Proxy', "exp=ignore_preview_blacklist")

  def HasChromeProxyLitePageResponse(self):
    return self.HasResponseHeader('Chrome-Proxy-Content-Transform', "lite-page")

  def HasChromeProxyPassThroughRequest(self):
    return self.HasRequestHeader('Chrome-Proxy-Accept-Transform', "identity")
