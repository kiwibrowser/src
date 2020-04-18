# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Opens and modifies WPR archive.
"""

import collections
import os
import re
import sys
from urlparse import urlparse


_SRC_DIR = os.path.abspath(os.path.join(
    os.path.dirname(__file__), '..', '..', '..'))

_WEBPAGEREPLAY_DIR = os.path.join(_SRC_DIR, 'third_party', 'webpagereplay')
_WEBPAGEREPLAY_HTTPARCHIVE = os.path.join(_WEBPAGEREPLAY_DIR, 'httparchive.py')

sys.path.append(os.path.join(_SRC_DIR, 'third_party', 'webpagereplay'))
import httparchive

# Regex used to parse httparchive.py stdout's when listing all urls.
_PARSE_WPR_REQUEST_REGEX = re.compile(r'^\S+\s+(?P<url>\S+)')

# Regex used to extract WPR domain from WPR log.
_PARSE_WPR_DOMAIN_REGEX = re.compile(r'^\(WARNING\)\s.*\sHTTP server started on'
                                     r' (?P<netloc>\S+)\s*$')

# Regex used to extract URLs requests from WPR log.
_PARSE_WPR_URL_REGEX = re.compile(
    r'^\((?P<level>\S+)\)\s.*\shttpproxy\..*\s(?P<method>[A-Z]+)\s+'
    r'(?P<url>https?://[a-zA-Z0-9\-_:.]+/?\S*)\s.*$')


class WprUrlEntry(object):
  """Wpr url entry holding request and response infos. """

  def __init__(self, wpr_request, wpr_response):
    self._wpr_response = wpr_response
    self.url = self._ExtractUrl(str(wpr_request))

  def GetResponseHeadersDict(self):
    """Get a copied dictionary of available headers.

    Returns:
      dict(name -> value)
    """
    headers = collections.defaultdict(list)
    for (key, value) in self._wpr_response.original_headers:
      headers[key.lower()].append(value)
    return {k: ','.join(v) for (k, v) in headers.items()}

  def SetResponseHeader(self, name, value):
    """Set a header value.

    In the case where the <name> response header is present more than once
    in the response header list, then the given value is set only to the first
    occurrence of that given headers, and the next ones are removed.

    Args:
      name: The name of the response header to set.
      value: The value of the response header to set.
    """
    assert name.islower()
    new_headers = []
    new_header_set = False
    for header in self._wpr_response.original_headers:
      if header[0].lower() != name:
        new_headers.append(header)
      elif not new_header_set:
        new_header_set = True
        new_headers.append((header[0], value))
    if new_header_set:
      self._wpr_response.original_headers = new_headers
    else:
      self._wpr_response.original_headers.append((name, value))

  def DeleteResponseHeader(self, name):
    """Delete a header.

    In the case where the <name> response header is present more than once
    in the response header list, this method takes care of removing absolutely
    all them.

    Args:
      name: The name of the response header field to delete.
    """
    assert name.islower()
    self._wpr_response.original_headers = \
        [x for x in self._wpr_response.original_headers if x[0].lower() != name]

  def RemoveResponseHeaderDirectives(self, name, directives_blacklist):
    """Removed a set of directives from response headers.

    Also removes the cache header in case no more directives are left.
    It is useful, for example, to remove 'no-cache' from 'pragma: no-cache'.

    Args:
      name: The name of the response header field to modify.
      directives_blacklist: Set of lowered directives to remove from list.
    """
    response_headers = self.GetResponseHeadersDict()
    if name not in response_headers:
      return
    new_value = []
    for header_name in response_headers[name].split(','):
      if header_name.strip().lower() not in directives_blacklist:
        new_value.append(header_name)
    if new_value:
      self.SetResponseHeader(name, ','.join(new_value))
    else:
      self.DeleteResponseHeader(name)

  @classmethod
  def _ExtractUrl(cls, request_string):
    match = _PARSE_WPR_REQUEST_REGEX.match(request_string)
    assert match, 'Looks like there is an issue with: {}'.format(request_string)
    return match.group('url')


class WprArchiveBackend(object):
  """WPR archive back-end able to read and modify. """

  def __init__(self, wpr_archive_path):
    """Constructor:

    Args:
      wpr_archive_path: The path of the WPR archive to read/modify.
    """
    self._wpr_archive_path = wpr_archive_path
    self._http_archive = httparchive.HttpArchive.Load(wpr_archive_path)

  def ListUrlEntries(self):
    """Iterates over all url entries

    Returns:
      A list of WprUrlEntry.
    """
    return [WprUrlEntry(request, self._http_archive[request])
            for request in self._http_archive.get_requests()]

  def Persist(self):
    """Persists the archive to disk. """
    for request in self._http_archive.get_requests():
      response = self._http_archive[request]
      response.headers = response._TrimHeaders(response.original_headers)
    self._http_archive.Persist(self._wpr_archive_path)


# WPR request seen by the WPR's HTTP proxy.
#   is_served: Boolean whether WPR has found a matching resource in the archive.
#   method: HTTP method of the request ['GET', 'POST' and so on...].
#   url: The requested URL.
#   is_wpr_host: Whether the requested url have WPR has an host such as:
#     http://127.0.0.1:<WPR's HTTP listening port>/web-page-replay-command-exit
WprRequest = collections.namedtuple('WprRequest',
    ['is_served', 'method', 'url', 'is_wpr_host'])


def ExtractRequestsFromLog(log_path):
  """Extract list of requested handled by the WPR's HTTP proxy from a WPR log.

  Args:
    log_path: The path of the WPR log to parse.

  Returns:
    List of WprRequest.
  """
  requests = []
  wpr_http_netloc = None
  with open(log_path) as log_file:
    for line in log_file.readlines():
      # Extract WPR's HTTP proxy's listening network location.
      match = _PARSE_WPR_DOMAIN_REGEX.match(line)
      if match:
        wpr_http_netloc = match.group('netloc')
        assert wpr_http_netloc.startswith('127.0.0.1:')
        continue
      # Extract the WPR requested URLs.
      match = _PARSE_WPR_URL_REGEX.match(line)
      if match:
        parsed_url = urlparse(match.group('url'))
        # Ignore strange URL requests such as http://ousvtzkizg/
        # TODO(gabadie): Find and terminate the location where they are queried.
        if '.' not in parsed_url.netloc and ':' not in parsed_url.netloc:
          continue
        assert wpr_http_netloc
        request = WprRequest(is_served=(match.group('level') == 'DEBUG'),
            method=match.group('method'), url=match.group('url'),
            is_wpr_host=parsed_url.netloc == wpr_http_netloc)
        requests.append(request)
  return requests


if __name__ == '__main__':
  import argparse
  parser = argparse.ArgumentParser(description='Tests cache back-end.')
  parser.add_argument('wpr_archive', type=str)
  command_line_args = parser.parse_args()

  wpr_backend = WprArchiveBackend(command_line_args.wpr_archive)
  url_entries = wpr_backend.ListUrlEntries()
  print url_entries[0].url
  wpr_backend.Persist()
