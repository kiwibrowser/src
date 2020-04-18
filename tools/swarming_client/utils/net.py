# Copyright 2013 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""Classes and functions for generic network communication over HTTP."""

import cookielib
import itertools
import json
import logging
import math
import os
import random
import re
import socket
import ssl
import threading
import time
import urllib
import urlparse

from third_party import requests
from third_party.requests import adapters
from third_party.requests import structures

from utils import authenticators
from utils import oauth
from utils import tools

# TODO(vadimsh): Refactor this stuff to be less magical, less global and less
# bad.

# Default maximum number of attempts to trying opening a url before aborting.
URL_OPEN_MAX_ATTEMPTS = 30

# Default timeout when retrying.
URL_OPEN_TIMEOUT = 6*60.

# Default timeout when reading from open HTTP connection.
URL_READ_TIMEOUT = 60

# Content type for url encoded POST body.
URL_ENCODED_FORM_CONTENT_TYPE = 'application/x-www-form-urlencoded'
# Content type for JSON body.
JSON_CONTENT_TYPE = 'application/json; charset=UTF-8'
# Default content type for POST body.
DEFAULT_CONTENT_TYPE = URL_ENCODED_FORM_CONTENT_TYPE

# Content type -> function that encodes a request body.
CONTENT_ENCODERS = {
  URL_ENCODED_FORM_CONTENT_TYPE:
    urllib.urlencode,
  JSON_CONTENT_TYPE:
    lambda x: json.dumps(x, sort_keys=True, separators=(',', ':')),
}


# Google Storage URL regular expression.
GS_STORAGE_HOST_URL_RE = re.compile(r'https://(.+\.)?storage\.googleapis\.com')

# Global (for now) map: server URL (http://example.com) -> HttpService instance.
# Used by get_http_service to cache HttpService instances.
_http_services = {}
_http_services_lock = threading.Lock()

# This lock ensures that user won't be confused with multiple concurrent
# login prompts.
_auth_lock = threading.Lock()

# Set in 'set_oauth_config'. If 'set_oauth_config' is not called before the
# first request, will be set to oauth.make_oauth_config().
_auth_config = None

# A class to use to send HTTP requests. Can be changed by 'set_engine_class'.
# Default is RequestsLibEngine.
_request_engine_cls = None


class NetError(IOError):
  """Generic network related error."""

  def __init__(self, inner_exc=None):
    super(NetError, self).__init__(str(inner_exc or self.__doc__))
    self.inner_exc = inner_exc


class TimeoutError(NetError):
  """Timeout while reading HTTP response."""


class ConnectionError(NetError):
  """Failed to connect to the server."""


class HttpError(NetError):
  """Server returned HTTP error code.

  Contains the response in full (as HttpResponse object) and the original
  exception raised by the underlying network library (if any). The type of
  the original exception may depend on the kind of the network engine and should
  not be relied upon.
  """

  def __init__(self, response, inner_exc=None):
    assert isinstance(response, HttpResponse), response
    super(HttpError, self).__init__(
        inner_exc or 'Server returned HTTP code %d' % response.code)
    self.response = response
    self._body_for_desc = None

  def description(self, verbose=False):
    """Returns a short or verbose description of the error.

    The short description is one line of text, the verbose one is multiple lines
    that may contain response body and headers.

    May be called multiple times (results of the first call are cached).

    "Destroys" the response by reading from it. So if the callers plan to read
    from 'response' themselves, they should not use 'description()'.
    """
    if self._body_for_desc is None:
      try:
        self._body_for_desc = self.response.read()
      except Exception as exc:
        self._body_for_desc = '<failed to read the response body: %s>' % exc

    desc = str(self)

    # If the body is JSON, assume it is Cloud Endpoints response and fish out an
    # error message from it.
    if self.response.content_type.startswith('application/json'):
      msg = _fish_out_error_message(self._body_for_desc)
      if msg:
        desc += ' - ' + msg

    if not verbose:
      return desc

    out = [desc, '----------']
    if self.response.headers:
      for header, value in sorted(self.response.headers.items()):
        if not header.lower().startswith('x-'):
          out.append('%s: %s' % (header.capitalize(), value))
      out.append('')
    out.append(self._body_for_desc or '<empty body>')
    out.append('----------')
    return '\n'.join(out)


def _fish_out_error_message(maybe_json_blob):
  try:
    as_json = json.loads(maybe_json_blob)
    err = as_json.get('error')
    if isinstance(err, basestring):
      return err
    if isinstance(err, dict):
      return str(err.get('message') or '<no error message>')
  except (ValueError, KeyError, TypeError):
    return None  # not a JSON we recognize


def set_engine_class(engine_cls):
  """Globally changes a class to use to execute HTTP requests.

  Default engine is RequestsLibEngine that uses 'requests' library. Changing the
  engine on the fly is not supported. It must be set before the first request.

  Custom engine class should support same public interface as RequestsLibEngine.
  """
  global _request_engine_cls
  assert _request_engine_cls is None
  _request_engine_cls = engine_cls


def get_engine_class():
  """Returns a class to use to execute HTTP requests."""
  return _request_engine_cls or RequestsLibEngine


def url_open(url, **kwargs):  # pylint: disable=W0621
  """Attempts to open the given url multiple times.

  |data| can be either:
    - None for a GET request
    - str for pre-encoded data
    - list for data to be encoded
    - dict for data to be encoded

  See HttpService.request for a full list of arguments.

  Returns HttpResponse object, where the response may be read from, or None
  if it was unable to connect.
  """
  urlhost, urlpath = split_server_request_url(url)
  service = get_http_service(urlhost)
  return service.request(urlpath, **kwargs)


def url_read(url, **kwargs):
  """Attempts to open the given url multiple times and read all data from it.

  Accepts same arguments as url_open function.

  Returns all data read or None if it was unable to connect or read the data.
  """
  response = url_open(url, stream=False, **kwargs)
  if not response:
    return None
  try:
    return response.read()
  except TimeoutError:
    return None


def url_read_json(url, **kwargs):
  """Attempts to open the given url multiple times and read all data from it.

  Accepts same arguments as url_open function.

  Returns all data read or None if it was unable to connect or read the data.
  """
  urlhost, urlpath = split_server_request_url(url)
  service = get_http_service(urlhost)
  try:
    return service.json_request(urlpath, **kwargs)
  except TimeoutError:
    return None


def url_retrieve(filepath, url, **kwargs):
  """Downloads an URL to a file. Returns True on success."""
  response = url_open(url, stream=False, **kwargs)
  if not response:
    return False
  try:
    with open(filepath, 'wb') as f:
      for buf in response.iter_content(65536):
        f.write(buf)
    return True
  except (IOError, OSError, TimeoutError):
    try:
      os.remove(filepath)
    except IOError:
      pass
    return False


def split_server_request_url(url):
  """Splits the url into scheme+netloc and path+params+query+fragment."""
  url_parts = list(urlparse.urlparse(url))
  urlhost = '%s://%s' % (url_parts[0], url_parts[1])
  urlpath = urlparse.urlunparse(['', ''] + url_parts[2:])
  return urlhost, urlpath


def fix_url(url):
  """Fixes an url to https."""
  parts = urlparse.urlparse(url, 'https')
  if parts.query:
    raise ValueError('doesn\'t support query parameter.')
  if parts.fragment:
    raise ValueError('doesn\'t support fragment in the url.')
  # urlparse('foo.com') will result in netloc='', path='foo.com', which is not
  # what is desired here.
  new = list(parts)
  if not new[1] and new[2]:
    new[1] = new[2].rstrip('/')
    new[2] = ''
  new[2] = new[2].rstrip('/')
  return urlparse.urlunparse(new)


def get_http_service(urlhost, allow_cached=True):
  """Returns existing or creates new instance of HttpService that can send
  requests to given base urlhost.
  """
  def new_service():
    # Create separate authenticator only if engine is not providing
    # authentication already. Also we use signed URLs for Google Storage, no
    # need for special authentication.
    authenticator = None
    engine_cls = get_engine_class()
    is_gs = GS_STORAGE_HOST_URL_RE.match(urlhost)
    conf = get_oauth_config()
    if not engine_cls.provides_auth and not is_gs and not conf.disabled:
      authenticator = (
          authenticators.LuciContextAuthenticator()
          if conf.use_luci_context_auth else
          authenticators.OAuthAuthenticator(urlhost, conf))
    return HttpService(
        urlhost,
        engine=engine_cls(),
        authenticator=authenticator)

  # Ensure consistency in url naming.
  urlhost = str(urlhost).lower().rstrip('/')

  if not allow_cached:
    return new_service()
  with _http_services_lock:
    service = _http_services.get(urlhost)
    if not service:
      service = new_service()
      _http_services[urlhost] = service
    return service


def disable_oauth_config():
  """Disables OAuth-based authentication performed by this module.

  With disabled OAuth config callers of url_open (and other functions) are
  supposed to prepare auth headers themselves and pass them via 'headers'
  argument.
  """
  set_oauth_config(oauth.DISABLED_OAUTH_CONFIG)


def set_oauth_config(config):
  """Defines what OAuth configuration to use for authentication.

  If request engine (see get_engine_class) provides authentication already (as
  indicated by its 'provides_auth=True' class property) this setting is ignored.

  Arguments:
    config: oauth.OAuthConfig instance.
  """
  global _auth_config
  _auth_config = config


def get_oauth_config():
  """Returns global OAuthConfig as set by 'set_oauth_config' or default one."""
  return _auth_config or oauth.make_oauth_config()


def get_case_insensitive_dict(original):
  """Given a dict with string keys returns new CaseInsensitiveDict.

  Raises ValueError if there are duplicate keys.
  """
  normalized = structures.CaseInsensitiveDict(original or {})
  if len(normalized) != len(original):
    raise ValueError('Duplicate keys in: %s' % repr(original))
  return normalized


class HttpService(object):
  """Base class for a class that provides an API to HTTP based service:
    - Provides 'request' method.
    - Supports automatic request retries.
    - Thread safe.
  """

  def __init__(self, urlhost, engine, authenticator=None):
    self.urlhost = urlhost
    self.engine = engine
    self.authenticator = authenticator

  @staticmethod
  def is_transient_http_error(resp, retry_50x, suburl):
    """Returns True if given HTTP response indicates a transient error."""
    # Google Storage can return this and it should be retried.
    if resp.code == 408:
      return True
    if resp.code == 404:
      # Transparently retry 404 IIF it is a CloudEndpoints API call *and* the
      # result is not JSON. This assumes that we only use JSON encoding. This
      # is workaround for known Cloud Endpoints bug.
      return (
          suburl.startswith('/_ah/api/') and
          not resp.content_type.startswith('application/json'))
    # All other 4** errors are fatal.
    if resp.code < 500:
      return False
    # Retry >= 500 error only if allowed by the caller.
    return retry_50x

  @staticmethod
  def encode_request_body(body, content_type):
    """Returns request body encoded according to its content type."""
    # No body or it is already encoded.
    if body is None or isinstance(body, str):
      return body
    # Any body should have content type set.
    assert content_type, 'Request has body, but no content type'
    encoder = CONTENT_ENCODERS.get(content_type)
    assert encoder, ('Unknown content type %s' % content_type)
    return encoder(body)

  def login(self, allow_user_interaction):
    """Runs authentication flow to refresh short lived access token.

    Authentication flow may need to interact with the user (read username from
    stdin, open local browser for OAuth2, etc.). If interaction is required and
    |allow_user_interaction| is False, the login will silently be considered
    failed (i.e. this function returns False).

    'request' method always uses non-interactive login, so long-lived
    authentication tokens (OAuth2 refresh token, etc) have to be set up
    manually by developer (by calling 'auth.py login' perhaps) prior running
    any swarming or isolate scripts.
    """
    # Use global lock to ensure two authentication flows never run in parallel.
    with _auth_lock:
      if self.authenticator and self.authenticator.supports_login:
        return self.authenticator.login(allow_user_interaction)
      return False

  def logout(self):
    """Purges access credentials from local cache."""
    if self.authenticator and self.authenticator.supports_login:
      self.authenticator.logout()

  def request(
      self,
      urlpath,
      data=None,
      content_type=None,
      max_attempts=URL_OPEN_MAX_ATTEMPTS,
      retry_50x=True,
      timeout=URL_OPEN_TIMEOUT,
      read_timeout=URL_READ_TIMEOUT,
      stream=True,
      method=None,
      headers=None,
      follow_redirects=True):
    """Attempts to open the given url multiple times.

    |urlpath| is relative to the server root, i.e. '/some/request?param=1'.

    |data| can be either:
      - None for a GET request
      - str for pre-encoded data
      - list for data to be form-encoded
      - dict for data to be form-encoded

    - Optionally retries HTTP 404 and 50x.
    - Retries up to |max_attempts| times. If None or 0, there's no limit in the
      number of retries.
    - Retries up to |timeout| duration in seconds. If None or 0, there's no
      limit in the time taken to do retries.
    - If both |max_attempts| and |timeout| are None or 0, this functions retries
      indefinitely.

    If |method| is given it can be 'DELETE', 'GET', 'POST' or 'PUT' and it will
    be used when performing the request. By default it's GET if |data| is None
    and POST if |data| is not None.

    If |headers| is given, it should be a dict with HTTP headers to append
    to request. Caller is responsible for providing headers that make sense.

    If |follow_redirects| is True, will transparently follow HTTP redirects,
    otherwise redirect response will be returned as is. It can be recognized
    by the presence of 'Location' response header.

    If |read_timeout| is not None will configure underlying socket to
    raise TimeoutError exception whenever there's no response from the server
    for more than |read_timeout| seconds. It can happen during any read
    operation so once you pass non-None |read_timeout| be prepared to handle
    these exceptions in subsequent reads from the stream.

    Returns a file-like object, where the response may be read from, or None
    if it was unable to connect. If |stream| is False will read whole response
    into memory buffer before returning file-like object that reads from this
    memory buffer.
    """
    assert urlpath and urlpath[0] == '/', urlpath

    if data is not None:
      assert method in (None, 'DELETE', 'POST', 'PUT')
      method = method or 'POST'
      content_type = content_type or DEFAULT_CONTENT_TYPE
      body = self.encode_request_body(data, content_type)
    else:
      assert method in (None, 'DELETE', 'GET')
      method = method or 'GET'
      body = None
      assert not content_type, 'Can\'t use content_type on %s' % method

    # Prepare request info.
    parsed = urlparse.urlparse('/' + urlpath.lstrip('/'))
    resource_url = urlparse.urljoin(self.urlhost, parsed.path)
    query_params = urlparse.parse_qsl(parsed.query)

    # Prepare headers.
    headers = get_case_insensitive_dict(headers or {})
    if body is not None:
      headers['Content-Length'] = len(body)
      if content_type:
        headers['Content-Type'] = content_type

    last_error = None
    auth_attempted = False

    for attempt in retry_loop(max_attempts, timeout):
      # Log non-first attempt.
      if attempt.attempt:
        logging.warning(
            'Retrying request %s, attempt %d/%d...',
            resource_url, attempt.attempt, max_attempts)

      try:
        # Prepare and send a new request.
        request = HttpRequest(
            method, resource_url, query_params, body,
            headers, read_timeout, stream, follow_redirects)
        if self.authenticator:
          self.authenticator.authorize(request)
        response = self.engine.perform_request(request)
        logging.debug('Request %s succeeded', request.get_full_url())
        return response

      except (ConnectionError, TimeoutError) as e:
        last_error = e
        logging.warning(
            'Unable to open url %s on attempt %d: %s',
            request.get_full_url(), attempt.attempt, e)
        continue

      except HttpError as e:
        last_error = e

        # Access denied -> authenticate.
        if e.response.code in (401, 403):
          logging.warning(
              'Got a reply with HTTP status code %d for %s on attempt %d: %s',
              e.response.code, request.get_full_url(),
              attempt.attempt, e.description())
          # Try forcefully refresh the token. If it doesn't help, then server
          # does not support authentication or user doesn't have required
          # access.
          if not auth_attempted:
            auth_attempted = True
            if self.login(allow_user_interaction=False):
              # Success! Run request again immediately.
              attempt.skip_sleep = True
              continue
          # Authentication attempt was unsuccessful.
          logging.error(
              'Request to %s failed with HTTP status code %d: %s',
              request.get_full_url(), e.response.code, e.description())
          if self.authenticator and self.authenticator.supports_login:
            logging.error(
                'Use auth.py to login if haven\'t done so already:\n'
                '    python auth.py login --service=%s', self.urlhost)
          return None

        # Hit a error that can not be retried -> stop retry loop.
        if not self.is_transient_http_error(e.response, retry_50x, parsed.path):
          # This HttpError means we reached the server and there was a problem
          # with the request, so don't retry. Dump entire reply to debug log and
          # only a friendly error message to error log.
          logging.debug(
              'Request to %s failed with HTTP status code %d.\n%s',
              request.get_full_url(), e.response.code,
              e.description(verbose=True))
          logging.error(
              'Request to %s failed with HTTP status code %d: %s',
              request.get_full_url(), e.response.code, e.description())
          return None

        # Retry all other errors.
        logging.warning(
            'Server responded with error on %s on attempt %d: %s',
            request.get_full_url(), attempt.attempt, e.description())
        continue

    if isinstance(last_error, HttpError):
      error_msg = last_error.description(verbose=True)
    else:
      error_msg = str(last_error)
    logging.error(
        'Unable to open given url, %s, after %d attempts.\n%s',
        request.get_full_url(), max_attempts, error_msg)

    return None

  def json_request(self, urlpath, data=None, **kwargs):
    """Sends JSON request to the server and parses JSON response it get back.

    Arguments:
      urlpath: relative request path (e.g. '/auth/v1/...').
      data: object to serialize to JSON and sent in the request.

    See self.request() for more details.

    Returns:
      Deserialized JSON response on success, None on error or timeout.
    """
    content_type = JSON_CONTENT_TYPE if data is not None else None
    response = self.request(
        urlpath, content_type=content_type, data=data, stream=False, **kwargs)
    if not response:
      return None
    try:
      text = response.read()
      if not text:
        return None
    except TimeoutError:
      return None
    try:
      return json.loads(text)
    except ValueError as e:
      logging.error('Not a JSON response when calling %s: %s; full text: %s',
                    urlpath, e, text)
      return None


class HttpRequest(object):
  """Request to HttpService."""

  def __init__(
      self, method, url, params, body,
      headers, timeout, stream, follow_redirects):
    """Arguments:
      |method| - HTTP method to use
      |url| - relative URL to the resource, without query parameters
      |params| - list of (key, value) pairs to put into GET parameters
      |body| - encoded body of the request (None or str)
      |headers| - dict with request headers
      |timeout| - socket read timeout (None to disable)
      |stream| - True to stream response from socket
      |follow_redirects| - True to follow HTTP redirects.
    """
    self.method = method
    self.url = url
    self.params = params[:]
    self.body = body
    self.headers = headers.copy()
    self.timeout = timeout
    self.stream = stream
    self.follow_redirects = follow_redirects
    self._cookies = None

  @property
  def cookies(self):
    """CookieJar object that will be used for cookies in this request."""
    if self._cookies is None:
      self._cookies = cookielib.CookieJar()
    return self._cookies

  def get_full_url(self):
    """Resource URL with url-encoded GET parameters."""
    if not self.params:
      return self.url
    else:
      return '%s?%s' % (self.url, urllib.urlencode(self.params))


class HttpResponse(object):
  """Response from HttpService.

  Wraps a file-like object that holds the body of the response. This object may
  optionally provide 'iter_content(chunk_size)' generator method if it supports
  iterator interface, and 'content' field if it can return the entire response
  body at once.
  """

  def __init__(self, response, url, code, headers, timeout_exc_classes=None):
    self._response = response
    self._url = url
    self._code = code
    self._headers = get_case_insensitive_dict(headers)
    self._timeout_exc_classes = timeout_exc_classes or ()

  def iter_content(self, chunk_size):
    """Yields the response in chunks of given size.

    This is a destructive operation in general. The response can not be reread.
    """
    assert all(issubclass(e, Exception) for e in self._timeout_exc_classes)
    try:
      read = 0
      if hasattr(self._response, 'iter_content'):
        # request.Response.
        for buf in self._response.iter_content(chunk_size):
          read += len(buf)
          yield buf
      else:
        # File-like object.
        while True:
          buf = self._response.read(chunk_size)
          if not buf:
            break
          read += len(buf)
          yield buf
    except self._timeout_exc_classes as e:
      logging.error('Timeout while reading from %s, read %d of %s: %s',
          self._url, read, self.get_header('Content-Length'), e)
      raise TimeoutError(e)

  def read(self):
    """Reads the entire response and returns it as bytes.

    This is a destructive operation in general. The response can not be reread.
    """
    assert all(issubclass(e, Exception) for e in self._timeout_exc_classes)
    try:
      if hasattr(self._response, 'content'):
        # request.Response.
        return self._response.content
      # File-like object.
      return self._response.read()
    except self._timeout_exc_classes as e:
      logging.error('Timeout while reading from %s, expected %s bytes: %s',
          self._url, self.get_header('Content-Length'), e)
      raise TimeoutError(e)

  def get_header(self, header):
    """Returns response header (as str) or None if no such header."""
    return self._headers.get(header)

  @property
  def headers(self):
    """Case insensitive dict with response headers."""
    return self._headers

  @property
  def code(self):
    """HTTP status code, as integer."""
    return self._code

  @property
  def content_type(self):
    """Response content type or empty string if not presented by the server."""
    return self.get_header('Content-Type') or ''


class RequestsLibEngine(object):
  """Class that knows how to execute HttpRequests via requests library."""

  # This engine doesn't know how to authenticate requests on transport level.
  provides_auth = False

  # A tuple of exception classes that represent timeout.
  #
  # Will be caught while reading a streaming response in HttpResponse.read and
  # transformed to TimeoutError.
  timeout_exception_classes = (
      socket.timeout, ssl.SSLError,
      requests.Timeout,
      requests.ConnectionError,
      requests.packages.urllib3.exceptions.ProtocolError,
      requests.packages.urllib3.exceptions.TimeoutError)

  def __init__(self):
    super(RequestsLibEngine, self).__init__()
    self.session = requests.Session()
    # Configure session.
    self.session.trust_env = False
    self.session.verify = tools.get_cacerts_bundle()
    # Configure connection pools.
    for protocol in ('https://', 'http://'):
      self.session.mount(protocol, adapters.HTTPAdapter(
          pool_connections=64,
          pool_maxsize=64,
          max_retries=0,
          pool_block=False))

  def perform_request(self, request):
    """Sends a HttpRequest to the server and reads back the response.

    Returns HttpResponse.

    Raises:
      ConnectionError - failed to establish connection to the server.
      TimeoutError - timeout while connecting or reading response.
      HttpError - server responded with >= 400 error code.
    """
    resp = None  # will be HttpResponse
    try:
      # response is a requests.models.Response.
      response = self.session.request(
          method=request.method,
          url=request.url,
          params=request.params,
          data=request.body,
          headers=request.headers,
          cookies=request.cookies,
          timeout=request.timeout,
          stream=request.stream,
          allow_redirects=request.follow_redirects)
      # Convert it to HttpResponse (that doesn't depend on the engine).
      resp = HttpResponse(
          response=response,
          url=request.get_full_url(),
          code=response.status_code,
          headers=response.headers,
          timeout_exc_classes=self.timeout_exception_classes)
      response.raise_for_status()
      return resp
    except requests.Timeout as e:
      raise TimeoutError(e)
    except requests.HTTPError as e:
      assert resp  # must be set, since HTTPError is raised by raise_for_status
      raise HttpError(resp, e)
    except (requests.ConnectionError, socket.timeout, ssl.SSLError) as e:
      raise ConnectionError(e)


class RetryAttempt(object):
  """Contains information about current retry attempt.

  Yielded from retry_loop.
  """

  def __init__(self, attempt, remaining):
    """Information about current attempt in retry loop:
      |attempt| - zero based index of attempt.
      |remaining| - how much time is left before retry loop finishes retries.
    """
    self.attempt = attempt
    self.remaining = remaining
    self.skip_sleep = False


def calculate_sleep_before_retry(attempt, max_duration):
  """How long to sleep before retrying an attempt in retry_loop."""
  # Maximum sleeping time. We're hammering a cloud-distributed service, it'll
  # survive.
  MAX_SLEEP = 10.
  # random.random() returns [0.0, 1.0). Starts with relatively short waiting
  # time by starting with 1.5/2+1.5^-1 median offset.
  duration = (random.random() * 1.5) + math.pow(1.5, (attempt - 1))
  assert duration > 0.1
  duration = min(MAX_SLEEP, duration)
  if max_duration:
    duration = min(max_duration, duration)
  return duration


def sleep_before_retry(attempt, max_duration):
  """Sleeps for some amount of time when retrying the attempt in retry_loop.

  To be mocked in tests.
  """
  time.sleep(calculate_sleep_before_retry(attempt, max_duration))


def current_time():
  """Used by retry loop to get current time.

  To be mocked in tests.
  """
  return time.time()


def retry_loop(max_attempts=None, timeout=None):
  """Yields whenever new attempt to perform some action is needed.

  Yields instances of RetryAttempt class that contains information about current
  attempt. Setting |skip_sleep| attribute of RetryAttempt to True will cause
  retry loop to run next attempt immediately.
  """
  start = current_time()
  for attempt in itertools.count():
    # Too many attempts?
    if max_attempts and attempt == max_attempts:
      break
    # Retried for too long?
    remaining = (timeout - (current_time() - start)) if timeout else None
    if remaining is not None and remaining < 0:
      break
    # Kick next iteration.
    attemp_obj = RetryAttempt(attempt, remaining)
    yield attemp_obj
    if attemp_obj.skip_sleep:
      continue
    # Only sleep if we are going to try again.
    if max_attempts and attempt != max_attempts - 1:
      remaining = (timeout - (current_time() - start)) if timeout else None
      if remaining is not None and remaining < 0:
        break
      sleep_before_retry(attempt, remaining)
