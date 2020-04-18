# Copyright 2016 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Dispatcher middleware for Cloud Endpoints API server.

This middleware does simple transforms on requests that come into the base path
and then re-dispatches them to the main backend. It does not do any
authentication, quota checking, DoS checking, etc.

In addition, the middleware loads API configs prior to each call, in case the
configuration has changed.
"""

# pylint: disable=g-bad-name
import cStringIO
import httplib
import json
import logging
import re
import urlparse
import wsgiref

import api_config_manager
import api_request
import discovery_api_proxy
import discovery_service
import errors
import parameter_converter
import util


__all__ = ['EndpointsDispatcherMiddleware']

_SERVER_SOURCE_IP = '0.2.0.3'

# Internal constants
_CORS_HEADER_ORIGIN = 'Origin'
_CORS_HEADER_REQUEST_METHOD = 'Access-Control-Request-Method'
_CORS_HEADER_REQUEST_HEADERS = 'Access-Control-Request-Headers'
_CORS_HEADER_ALLOW_ORIGIN = 'Access-Control-Allow-Origin'
_CORS_HEADER_ALLOW_METHODS = 'Access-Control-Allow-Methods'
_CORS_HEADER_ALLOW_HEADERS = 'Access-Control-Allow-Headers'
_CORS_HEADER_ALLOW_CREDS = 'Access-Control-Allow-Credentials'
_CORS_HEADER_EXPOSE_HEADERS = 'Access-Control-Expose-Headers'
_CORS_ALLOWED_METHODS = frozenset(('DELETE', 'GET', 'PATCH', 'POST', 'PUT'))
_CORS_EXPOSED_HEADERS = frozenset(
    ('Content-Encoding', 'Content-Length', 'Date', 'ETag', 'Server')
)


class EndpointsDispatcherMiddleware(object):
  """Dispatcher that handles requests to the built-in apiserver handlers."""

  _API_EXPLORER_URL = 'https://apis-explorer.appspot.com/apis-explorer/?base='

  def __init__(self, backend_wsgi_app, config_manager=None):
    """Constructor for EndpointsDispatcherMiddleware.

    Args:
      backend_wsgi_app: A WSGI server that serves the app's endpoints.
      config_manager: An ApiConfigManager instance that allows a caller to
        set up an existing configuration for testing.
    """
    if config_manager is None:
      config_manager = api_config_manager.ApiConfigManager()
    self.config_manager = config_manager

    self._backend = backend_wsgi_app
    self._dispatchers = []
    for base_path in self._backend.base_paths:
      self._add_dispatcher('%sexplorer/?$' % base_path,
                           self.handle_api_explorer_request)
      self._add_dispatcher('%sstatic/.*$' % base_path,
                           self.handle_api_static_request)

  def _add_dispatcher(self, path_regex, dispatch_function):
    """Add a request path and dispatch handler.

    Args:
      path_regex: A string regex, the path to match against incoming requests.
      dispatch_function: The function to call for these requests.  The function
        should take (request, start_response) as arguments and
        return the contents of the response body.
    """
    self._dispatchers.append((re.compile(path_regex), dispatch_function))

  def __call__(self, environ, start_response):
    """Handle an incoming request.

    Args:
      environ: An environ dict for the request as defined in PEP-333.
      start_response: A function used to begin the response to the caller.
        This follows the semantics defined in PEP-333.  In particular, it's
        called with (status, response_headers, exc_info=None), and it returns
        an object with a write(body_data) function that can be used to write
        the body of the response.

    Yields:
      An iterable over strings containing the body of the HTTP response.
    """
    request = api_request.ApiRequest(environ,
                                     base_paths=self._backend.base_paths)

    # PEP-333 requires that we return an iterator that iterates over the
    # response body.  Yielding the returned body accomplishes this.
    yield self.dispatch(request, start_response)

  def dispatch(self, request, start_response):
    """Handles dispatch to apiserver handlers.

    This typically ends up calling start_response and returning the entire
      body of the response.

    Args:
      request: An ApiRequest, the request from the user.
      start_response: A function with semantics defined in PEP-333.

    Returns:
      A string, the body of the response.
    """
    # Check if this matches any of our special handlers.
    dispatched_response = self.dispatch_non_api_requests(request,
                                                         start_response)
    if dispatched_response is not None:
      return dispatched_response

    # Get API configuration first.  We need this so we know how to
    # call the back end.
    api_config_response = self.get_api_configs()
    if api_config_response:
      self.config_manager.process_api_config_response(api_config_response)
    else:
      return self.fail_request(request, 'get_api_configs Error',
                               start_response)

    # Call the service.
    try:
      return self.call_backend(request, start_response)
    except errors.RequestError as error:
      return self._handle_request_error(request, error, start_response)

  def dispatch_non_api_requests(self, request, start_response):
    """Dispatch this request if this is a request to a reserved URL.

    If the request matches one of our reserved URLs, this calls
    start_response and returns the response body.  This also handles OPTIONS
    CORS requests.

    Args:
      request: An ApiRequest, the request from the user.
      start_response: A function with semantics defined in PEP-333.

    Returns:
      None if the request doesn't match one of the reserved URLs this
      handles.  Otherwise, returns the response body.
    """
    for path_regex, dispatch_function in self._dispatchers:
      if path_regex.match(request.relative_url):
        return dispatch_function(request, start_response)

    if request.http_method == 'OPTIONS':
      cors_handler = self._create_cors_handler(request)
      if cors_handler.allow_cors_request:
        # The server returns 200 rather than 204, for some reason.
        return util.send_wsgi_response('200', [], '', start_response,
                                       cors_handler)

    return None

  def handle_api_explorer_request(self, request, start_response):
    """Handler for requests to {base_path}/explorer.

    This calls start_response and returns the response body.

    Args:
      request: An ApiRequest, the request from the user.
      start_response: A function with semantics defined in PEP-333.

    Returns:
      A string containing the response body (which is empty, in this case).
    """
    protocol = 'http' if 'localhost' in request.server else 'https'
    base_path = request.base_path.strip('/')
    base_url = '{0}://{1}:{2}/{3}'.format(
        protocol, request.server, request.port, base_path)
    redirect_url = self._API_EXPLORER_URL + base_url
    return util.send_wsgi_redirect_response(redirect_url, start_response)

  def handle_api_static_request(self, request, start_response):
    """Handler for requests to {base_path}/static/.*.

    This calls start_response and returns the response body.

    Args:
      request: An ApiRequest, the request from the user.
      start_response: A function with semantics defined in PEP-333.

    Returns:
      A string containing the response body.
    """
    discovery_api = discovery_api_proxy.DiscoveryApiProxy()
    response, body = discovery_api.get_static_file(request.relative_url)
    status_string = '%d %s' % (response.status, response.reason)
    if response.status == 200:
      # Some of the headers that come back from the server can't be passed
      # along in our response.  Specifically, the response from the server has
      # transfer-encoding: chunked, which doesn't apply to the response that
      # we're forwarding.  There may be other problematic headers, so we strip
      # off everything but Content-Type.
      return util.send_wsgi_response(status_string,
                                     [('Content-Type',
                                       response.getheader('Content-Type'))],
                                     body, start_response)
    else:
      logging.error('Discovery API proxy failed on %s with %d. Details: %s',
                    request.relative_url, response.status, body)
      return util.send_wsgi_response(status_string, response.getheaders(), body,
                                     start_response)

  def get_api_configs(self):
    return self._backend.get_api_configs()

  @staticmethod
  def verify_response(response, status_code, content_type=None):
    """Verifies that a response has the expected status and content type.

    Args:
      response: The ResponseTuple to be checked.
      status_code: An int, the HTTP status code to be compared with response
        status.
      content_type: A string with the acceptable Content-Type header value.
        None allows any content type.

    Returns:
      True if both status_code and content_type match, else False.
    """
    status = int(response.status.split(' ', 1)[0])
    if status != status_code:
      return False

    if content_type is None:
      return True

    for header, value in response.headers:
      if header.lower() == 'content-type':
        return value == content_type

    # If we fall through to here, the verification has failed, so return False.
    return False

  def prepare_backend_environ(self, host, method, relative_url, headers, body,
                              source_ip, port):
    """Build an environ object for the backend to consume.

    Args:
      host: A string containing the host serving the request.
      method: A string containing the HTTP method of the request.
      relative_url: A string containing path and query string of the request.
      headers: A list of (key, value) tuples where key and value are both
               strings.
      body: A string containing the request body.
      source_ip: The source IP address for the request.
      port: The port to which to direct the request.

    Returns:
      An environ object with all the information necessary for the backend to
      process the request.
    """
    if isinstance(body, unicode):
      body = body.encode('ascii')

    url = urlparse.urlsplit(relative_url)
    if port != 80:
      host = '%s:%s' % (host, port)
    else:
      host = host
    environ = {'CONTENT_LENGTH': str(len(body)),
               'PATH_INFO': url.path,
               'QUERY_STRING': url.query,
               'REQUEST_METHOD': method,
               'REMOTE_ADDR': source_ip,
               'SERVER_NAME': host,
               'SERVER_PORT': str(port),
               'SERVER_PROTOCOL': 'HTTP/1.1',
               'wsgi.version': (1, 0),
               'wsgi.url_scheme': 'http',
               'wsgi.errors': cStringIO.StringIO(),
               'wsgi.multithread': True,
               'wsgi.multiprocess': True,
               'wsgi.input': cStringIO.StringIO(body)}
    util.put_headers_in_environ(headers, environ)
    environ['HTTP_HOST'] = host
    return environ

  def call_backend(self, orig_request, start_response):
    """Generate API call (from earlier-saved request).

    This calls start_response and returns the response body.

    Args:
      orig_request: An ApiRequest, the original request from the user.
      start_response: A function with semantics defined in PEP-333.

    Returns:
      A string containing the response body.
    """
    if orig_request.is_rpc():
      method_config = self.lookup_rpc_method(orig_request)
      params = None
    else:
      method_config, params = self.lookup_rest_method(orig_request)
    if not method_config:
      cors_handler = self._create_cors_handler(orig_request)
      return util.send_wsgi_not_found_response(start_response,
                                               cors_handler=cors_handler)

    # Prepare the request for the back end.
    transformed_request = self.transform_request(
        orig_request, params, method_config)

    # Check if this call is for the Discovery service.  If so, route
    # it to our Discovery handler.
    discovery = discovery_service.DiscoveryService(
        self.config_manager, self._backend)
    discovery_response = discovery.handle_discovery_request(
        transformed_request.path, transformed_request, start_response)
    if discovery_response:
      return discovery_response

    url = transformed_request.base_path + transformed_request.path
    transformed_request.headers['Content-Type'] = 'application/json'
    transformed_environ = self.prepare_backend_environ(
        orig_request.server, 'POST', url, transformed_request.headers.items(),
        transformed_request.body, transformed_request.source_ip,
        orig_request.port)

    # Send the transformed request to the backend app and capture the response.
    with util.StartResponseProxy() as start_response_proxy:
      body_iter = self._backend(transformed_environ, start_response_proxy.Proxy)
      status = start_response_proxy.response_status
      headers = start_response_proxy.response_headers

      # Get response body
      body = start_response_proxy.response_body
      # In case standard WSGI behavior is implemented later...
      if not body:
        body = ''.join(body_iter)

    return self.handle_backend_response(orig_request, transformed_request,
                                        status, headers, body, method_config,
                                        start_response)

  class __CheckCorsHeaders(object):
    """Track information about CORS headers and our response to them."""

    def __init__(self, request):
      self.allow_cors_request = False
      self.origin = None
      self.cors_request_method = None
      self.cors_request_headers = None

      self.__check_cors_request(request)

    def __check_cors_request(self, request):
      """Check for a CORS request, and see if it gets a CORS response."""
      # Check for incoming CORS headers.
      self.origin = request.headers[_CORS_HEADER_ORIGIN]
      self.cors_request_method = request.headers[_CORS_HEADER_REQUEST_METHOD]
      self.cors_request_headers = request.headers[
          _CORS_HEADER_REQUEST_HEADERS]

      # Check if the request should get a CORS response.
      if (self.origin and
          ((self.cors_request_method is None) or
           (self.cors_request_method.upper() in _CORS_ALLOWED_METHODS))):
        self.allow_cors_request = True

    def update_headers(self, headers_in):
      """Add CORS headers to the response, if needed."""
      if not self.allow_cors_request:
        return

      # Add CORS headers.
      headers = wsgiref.headers.Headers(headers_in)
      headers[_CORS_HEADER_ALLOW_CREDS] = 'true'
      headers[_CORS_HEADER_ALLOW_ORIGIN] = self.origin
      headers[_CORS_HEADER_ALLOW_METHODS] = ','.join(tuple(
          _CORS_ALLOWED_METHODS))
      headers[_CORS_HEADER_EXPOSE_HEADERS] = ','.join(tuple(
          _CORS_EXPOSED_HEADERS))
      if self.cors_request_headers is not None:
        headers[_CORS_HEADER_ALLOW_HEADERS] = self.cors_request_headers

  def _create_cors_handler(self, request):
    return EndpointsDispatcherMiddleware.__CheckCorsHeaders(request)

  def handle_backend_response(self, orig_request, backend_request,
                              response_status, response_headers,
                              response_body, method_config, start_response):
    """Handle backend response, transforming output as needed.

    This calls start_response and returns the response body.

    Args:
      orig_request: An ApiRequest, the original request from the user.
      backend_request: An ApiRequest, the transformed request that was
                       sent to the backend handler.
      response_status: A string, the status from the response.
      response_headers: A dict, the headers from the response.
      response_body: A string, the body of the response.
      method_config: A dict, the API config of the method to be called.
      start_response: A function with semantics defined in PEP-333.

    Returns:
      A string containing the response body.
    """
    # Verify that the response is json.  If it isn't treat, the body as an
    # error message and wrap it in a json error response.
    for header, value in response_headers:
      if (header.lower() == 'content-type' and
          not value.lower().startswith('application/json')):
        return self.fail_request(orig_request,
                                 'Non-JSON reply: %s' % response_body,
                                 start_response)

    self.check_error_response(response_body, response_status)

    # Need to check is_rpc() against the original request, because the
    # incoming request here has had its path modified.
    if orig_request.is_rpc():
      body = self.transform_jsonrpc_response(backend_request, response_body)
    else:
      # Check if the response from the API was empty.  Empty REST responses
      # generate a HTTP 204.
      empty_response = self.check_empty_response(orig_request, method_config,
                                                 start_response)
      if empty_response is not None:
        return empty_response

      body = self.transform_rest_response(response_body)

    cors_handler = self._create_cors_handler(orig_request)
    return util.send_wsgi_response(response_status, response_headers, body,
                                   start_response, cors_handler=cors_handler)

  def fail_request(self, orig_request, message, start_response):
    """Write an immediate failure response to outfile, no redirect.

    This calls start_response and returns the error body.

    Args:
      orig_request: An ApiRequest, the original request from the user.
      message: A string containing the error message to be displayed to user.
      start_response: A function with semantics defined in PEP-333.

    Returns:
      A string containing the body of the error response.
    """
    cors_handler = self._create_cors_handler(orig_request)
    return util.send_wsgi_error_response(
        message, start_response, cors_handler=cors_handler)

  def lookup_rest_method(self, orig_request):
    """Looks up and returns rest method for the currently-pending request.

    Args:
      orig_request: An ApiRequest, the original request from the user.

    Returns:
      A tuple of (method descriptor, parameters), or (None, None) if no method
      was found for the current request.
    """
    method_name, method, params = self.config_manager.lookup_rest_method(
        orig_request.path, orig_request.http_method)
    orig_request.method_name = method_name
    return method, params

  def lookup_rpc_method(self, orig_request):
    """Looks up and returns RPC method for the currently-pending request.

    Args:
      orig_request: An ApiRequest, the original request from the user.

    Returns:
      The RPC method descriptor that was found for the current request, or None
      if none was found.
    """
    if not orig_request.body_json:
      return None
    method_name = orig_request.body_json.get('method', '')
    version = orig_request.body_json.get('apiVersion', '')
    orig_request.method_name = method_name
    return self.config_manager.lookup_rpc_method(method_name, version)

  def transform_request(self, orig_request, params, method_config):
    """Transforms orig_request to apiserving request.

    This method uses orig_request to determine the currently-pending request
    and returns a new transformed request ready to send to the backend.  This
    method accepts a rest-style or RPC-style request.

    Args:
      orig_request: An ApiRequest, the original request from the user.
      params: A dictionary containing path parameters for rest requests, or
        None for an RPC request.
      method_config: A dict, the API config of the method to be called.

    Returns:
      An ApiRequest that's a copy of the current request, modified so it can
      be sent to the backend.  The path is updated and parts of the body or
      other properties may also be changed.
    """
    if orig_request.is_rpc():
      request = self.transform_jsonrpc_request(orig_request)
    else:
      method_params = method_config.get('request', {}).get('parameters', {})
      request = self.transform_rest_request(orig_request, params, method_params)
    request.path = method_config.get('rosyMethod', '')
    return request

  def _add_message_field(self, field_name, value, params):
    """Converts a . delimitied field name to a message field in parameters.

    This adds the field to the params dict, broken out so that message
    parameters appear as sub-dicts within the outer param.

    For example:
      {'a.b.c': ['foo']}
    becomes:
      {'a': {'b': {'c': ['foo']}}}

    Args:
      field_name: A string containing the '.' delimitied name to be converted
        into a dictionary.
      value: The value to be set.
      params: The dictionary holding all the parameters, where the value is
        eventually set.
    """
    if '.' not in field_name:
      params[field_name] = value
      return

    root, remaining = field_name.split('.', 1)
    sub_params = params.setdefault(root, {})
    self._add_message_field(remaining, value, sub_params)

  def _update_from_body(self, destination, source):
    """Updates the dictionary for an API payload with the request body.

    The values from the body should override those already in the payload, but
    for nested fields (message objects) the values can be combined
    recursively.

    Args:
      destination: A dictionary containing an API payload parsed from the
        path and query parameters in a request.
      source: A dictionary parsed from the body of the request.
    """
    for key, value in source.iteritems():
      destination_value = destination.get(key)
      if isinstance(value, dict) and isinstance(destination_value, dict):
        self._update_from_body(destination_value, value)
      else:
        destination[key] = value

  def transform_rest_request(self, orig_request, params, method_parameters):
    """Translates a Rest request into an apiserving request.

    This makes a copy of orig_request and transforms it to apiserving
    format (moving request parameters to the body).

    The request can receive values from the path, query and body and combine
    them before sending them along to the backend. In cases of collision,
    objects from the body take precedence over those from the query, which in
    turn take precedence over those from the path.

    In the case that a repeated value occurs in both the query and the path,
    those values can be combined, but if that value also occurred in the body,
    it would override any other values.

    In the case of nested values from message fields, non-colliding values
    from subfields can be combined. For example, if '?a.c=10' occurs in the
    query string and "{'a': {'b': 11}}" occurs in the body, then they will be
    combined as

    {
      'a': {
        'b': 11,
        'c': 10,
      }
    }

    before being sent to the backend.

    Args:
      orig_request: An ApiRequest, the original request from the user.
      params: A dict with URL path parameters extracted by the config_manager
        lookup.
      method_parameters: A dictionary containing the API configuration for the
        parameters for the request.

    Returns:
      A copy of the current request that's been modified so it can be sent
      to the backend.  The body is updated to include parameters from the
      URL.
    """
    request = orig_request.copy()
    body_json = {}

    # Handle parameters from the URL path.
    for key, value in params.iteritems():
      # Values need to be in a list to interact with query parameter values
      # and to account for case of repeated parameters
      body_json[key] = [value]

    # Add in parameters from the query string.
    if request.parameters:
      # For repeated elements, query and path work together
      for key, value in request.parameters.iteritems():
        if key in body_json:
          body_json[key] = value + body_json[key]
        else:
          body_json[key] = value

    # Validate all parameters we've merged so far and convert any '.' delimited
    # parameters to nested parameters.  We don't use iteritems since we may
    # modify body_json within the loop.  For instance, 'a.b' is not a valid key
    # and would be replaced with 'a'.
    for key, value in body_json.items():
      current_parameter = method_parameters.get(key, {})
      repeated = current_parameter.get('repeated', False)

      if not repeated:
        body_json[key] = body_json[key][0]

      # Order is important here.  Parameter names are dot-delimited in
      # parameters instead of nested in dictionaries as a message field is, so
      # we need to call transform_parameter_value on them before calling
      # _add_message_field.
      body_json[key] = parameter_converter.transform_parameter_value(
          key, body_json[key], current_parameter)
      # Remove the old key and try to convert to nested message value
      message_value = body_json.pop(key)
      self._add_message_field(key, message_value, body_json)

    # Add in values from the body of the request.
    if request.body_json:
      self._update_from_body(body_json, request.body_json)

    request.body_json = body_json
    request.body = json.dumps(request.body_json)
    return request

  def transform_jsonrpc_request(self, orig_request):
    """Translates a JsonRpc request/response into apiserving request/response.

    Args:
      orig_request: An ApiRequest, the original request from the user.

    Returns:
      A new request with the request_id updated and params moved to the body.
    """
    request = orig_request.copy()
    request.request_id = request.body_json.get('id')
    request.body_json = request.body_json.get('params', {})
    request.body = json.dumps(request.body_json)
    return request

  def check_error_response(self, body, status):
    """Raise an exception if the response from the backend was an error.

    Args:
      body: A string containing the backend response body.
      status: A string containing the backend response status.

    Raises:
      BackendError if the response is an error.
    """
    status_code = int(status.split(' ', 1)[0])
    if status_code >= 300:
      raise errors.BackendError(body, status)

  def check_empty_response(self, orig_request, method_config, start_response):
    """If the response from the backend is empty, return a HTTP 204 No Content.

    Args:
      orig_request: An ApiRequest, the original request from the user.
      method_config: A dict, the API config of the method to be called.
      start_response: A function with semantics defined in PEP-333.

    Returns:
      If the backend response was empty, this returns a string containing the
      response body that should be returned to the user.  If the backend
      response wasn't empty, this returns None, indicating that we should not
      exit early with a 204.
    """
    response_config = method_config.get('response', {}).get('body')
    if response_config == 'empty':
      # The response to this function should be empty.  We should return a 204.
      # Note that it's possible that the backend returned something, but we'll
      # ignore it.  This matches the behavior in the Endpoints server.
      cors_handler = self._create_cors_handler(orig_request)
      return util.send_wsgi_no_content_response(start_response, cors_handler)

  def transform_rest_response(self, response_body):
    """Translates an apiserving REST response so it's ready to return.

    Currently, the only thing that needs to be fixed here is indentation,
    so it's consistent with what the live app will return.

    Args:
      response_body: A string containing the backend response.

    Returns:
      A reformatted version of the response JSON.
    """
    body_json = json.loads(response_body)
    return json.dumps(body_json, indent=1, sort_keys=True)

  def transform_jsonrpc_response(self, backend_request, response_body):
    """Translates an apiserving response to a JsonRpc response.

    Args:
      backend_request: An ApiRequest, the transformed request that was sent to
        the backend handler.
      response_body: A string containing the backend response to transform
        back to JsonRPC.

    Returns:
      A string with the updated, JsonRPC-formatted request body.
    """
    body_json = {'result': json.loads(response_body)}
    return self._finish_rpc_response(backend_request.request_id,
                                     backend_request.is_batch(), body_json)

  def _finish_rpc_response(self, request_id, is_batch, body_json):
    """Finish adding information to a JSON RPC response.

    Args:
      request_id: None if the request didn't have a request ID.  Otherwise, this
        is a string containing the request ID for the request.
      is_batch: A boolean indicating whether the request is a batch request.
      body_json: A dict containing the JSON body of the response.

    Returns:
      A string with the updated, JsonRPC-formatted request body.
    """
    if request_id is not None:
      body_json['id'] = request_id
    if is_batch:
      body_json = [body_json]
    return json.dumps(body_json, indent=1, sort_keys=True)

  def _handle_request_error(self, orig_request, error, start_response):
    """Handle a request error, converting it to a WSGI response.

    Args:
      orig_request: An ApiRequest, the original request from the user.
      error: A RequestError containing information about the error.
      start_response: A function with semantics defined in PEP-333.

    Returns:
      A string containing the response body.
    """
    headers = [('Content-Type', 'application/json')]
    if orig_request.is_rpc():
      # JSON RPC errors are returned with status 200 OK and the
      # error details in the body.
      status_code = 200
      body = self._finish_rpc_response(orig_request.body_json.get('id'),
                                       orig_request.is_batch(),
                                       error.rpc_error())
    else:
      status_code = error.status_code()
      body = error.rest_error()

    response_status = '%d %s' % (status_code,
                                 httplib.responses.get(status_code,
                                                       'Unknown Error'))
    cors_handler = self._create_cors_handler(orig_request)
    return util.send_wsgi_response(response_status, headers, body,
                                   start_response, cors_handler=cors_handler)
