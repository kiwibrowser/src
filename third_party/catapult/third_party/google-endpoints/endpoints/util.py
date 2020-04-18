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

"""Helper utilities for the endpoints package."""

# pylint: disable=g-bad-name

import cStringIO
import json
import os
import wsgiref.headers

from google.appengine.api import app_identity

from google.appengine.api.modules import modules


class StartResponseProxy(object):
  """Proxy for the typical WSGI start_response object."""

  def __init__(self):
    self.call_context = {}
    self.body_buffer = cStringIO.StringIO()

  def __enter__(self):
    return self

  def __exit__(self, exc_type, exc_value, traceback):
    # Close out the cStringIO.StringIO buffer to prevent memory leakage.
    if self.body_buffer:
      self.body_buffer.close()

  def Proxy(self, status, headers, exc_info=None):
    """Save args, defer start_response until response body is parsed.

    Create output buffer for body to be written into.
    Note: this is not quite WSGI compliant: The body should come back as an
      iterator returned from calling service_app() but instead, StartResponse
      returns a writer that will be later called to output the body.
    See google/appengine/ext/webapp/__init__.py::Response.wsgi_write()
        write = start_response('%d %s' % self.__status, self.__wsgi_headers)
        write(body)

    Args:
      status: Http status to be sent with this response
      headers: Http headers to be sent with this response
      exc_info: Exception info to be displayed for this response
    Returns:
      callable that takes as an argument the body content
    """
    self.call_context['status'] = status
    self.call_context['headers'] = headers
    self.call_context['exc_info'] = exc_info

    return self.body_buffer.write

  @property
  def response_body(self):
    return self.body_buffer.getvalue()

  @property
  def response_headers(self):
    return self.call_context.get('headers')

  @property
  def response_status(self):
    return self.call_context.get('status')

  @property
  def response_exc_info(self):
    return self.call_context.get('exc_info')


def send_wsgi_not_found_response(start_response, cors_handler=None):
  return send_wsgi_response('404 Not Found', [('Content-Type', 'text/plain')],
                            'Not Found', start_response,
                            cors_handler=cors_handler)


def send_wsgi_error_response(message, start_response, cors_handler=None):
  body = json.dumps({'error': {'message': message}})
  return send_wsgi_response('500', [('Content-Type', 'application/json')], body,
                            start_response, cors_handler=cors_handler)


def send_wsgi_rejected_response(rejection_error, start_response,
                                cors_handler=None):
  body = rejection_error.to_json()
  return send_wsgi_response('400', [('Content-Type', 'application/json')], body,
                            start_response, cors_handler=cors_handler)


def send_wsgi_redirect_response(redirect_location, start_response,
                                cors_handler=None):
  return send_wsgi_response('302', [('Location', redirect_location)], '',
                            start_response, cors_handler=cors_handler)


def send_wsgi_no_content_response(start_response, cors_handler=None):
  return send_wsgi_response('204 No Content', [], '', start_response,
                            cors_handler)


def send_wsgi_response(status, headers, content, start_response,
                       cors_handler=None):
  """Dump reformatted response to CGI start_response.

  This calls start_response and returns the response body.

  Args:
    status: A string containing the HTTP status code to send.
    headers: A list of (header, value) tuples, the headers to send in the
      response.
    content: A string containing the body content to write.
    start_response: A function with semantics defined in PEP-333.
    cors_handler: A handler to process CORS request headers and update the
      headers in the response.  Or this can be None, to bypass CORS checks.

  Returns:
    A string containing the response body.
  """
  if cors_handler:
    cors_handler.update_headers(headers)

  # Update content length.
  content_len = len(content) if content else 0
  headers = [(header, value) for header, value in headers
             if header.lower() != 'content-length']
  headers.append(('Content-Length', '%s' % content_len))

  start_response(status, headers)
  return content


def get_headers_from_environ(environ):
  """Get a wsgiref.headers.Headers object with headers from the environment.

  Headers in environ are prefixed with 'HTTP_', are all uppercase, and have
  had dashes replaced with underscores.  This strips the HTTP_ prefix and
  changes underscores back to dashes before adding them to the returned set
  of headers.

  Args:
    environ: An environ dict for the request as defined in PEP-333.

  Returns:
    A wsgiref.headers.Headers object that's been filled in with any HTTP
    headers found in environ.
  """
  headers = wsgiref.headers.Headers([])
  for header, value in environ.iteritems():
    if header.startswith('HTTP_'):
      headers[header[5:].replace('_', '-')] = value
  # Content-Type is special; it does not start with 'HTTP_'.
  if 'CONTENT_TYPE' in environ:
    headers['CONTENT-TYPE'] = environ['CONTENT_TYPE']
  return headers


def put_headers_in_environ(headers, environ):
  """Given a list of headers, put them into environ based on PEP-333.

  This converts headers to uppercase, prefixes them with 'HTTP_', and
  converts dashes to underscores before adding them to the environ dict.

  Args:
    headers: A list of (header, value) tuples.  The HTTP headers to add to the
      environment.
    environ: An environ dict for the request as defined in PEP-333.
  """
  for key, value in headers:
    environ['HTTP_%s' % key.upper().replace('-', '_')] = value


def is_running_on_app_engine():
  return os.environ.get('GAE_MODULE_NAME') is not None


def is_running_on_devserver():
  return os.environ.get('SERVER_SOFTWARE', '').startswith('Development/')


def is_running_on_localhost():
  return os.environ.get('SERVER_NAME') == 'localhost'


def get_app_hostname():
  """Return hostname of a running Endpoints service.

  Returns hostname of an running Endpoints API. It can be 1) "localhost:PORT"
  if running on development server, or 2) "app_id.appspot.com" if running on
  external app engine prod, or "app_id.googleplex.com" if running as Google
  first-party Endpoints API, or 4) None if not running on App Engine
  (e.g. Tornado Endpoints API).

  Returns:
    A string representing the hostname of the service.
  """
  if not is_running_on_app_engine() or is_running_on_localhost():
    return None

  version = modules.get_current_version_name()
  app_id = app_identity.get_application_id()

  suffix = 'appspot.com'

  if ':' in app_id:
    tokens = app_id.split(':')
    api_name = tokens[1]
    if tokens[0] == 'google.com':
      suffix = 'googleplex.com'
  else:
    api_name = app_id

  # Check if this is the default version
  default_version = modules.get_default_version()
  if version == default_version:
    return '{0}.{1}'.format(app_id, suffix)
  else:
    return '{0}-dot-{1}.{2}'.format(version, api_name, suffix)


def check_list_type(objects, allowed_type, name, allow_none=True):
  """Verify that objects in list are of the allowed type or raise TypeError.

  Args:
    objects: The list of objects to check.
    allowed_type: The allowed type of items in 'settings'.
    name: Name of the list of objects, added to the exception.
    allow_none: If set, None is also allowed.

  Raises:
    TypeError: if object is not of the allowed type.

  Returns:
    The list of objects, for convenient use in assignment.
  """
  if objects is None:
    if not allow_none:
      raise TypeError('%s is None, which is not allowed.' % name)
    return objects
  if not isinstance(objects, (tuple, list)):
    raise TypeError('%s is not a list.' % name)
  if not all(isinstance(i, allowed_type) for i in objects):
    type_list = sorted(list(set(type(obj) for obj in objects)))
    raise TypeError('%s contains types that don\'t match %s: %s' %
                    (name, allowed_type.__name__, type_list))
  return objects


def snake_case_to_headless_camel_case(snake_string):
  """Convert snake_case to headlessCamelCase.

  Args:
    snake_string: The string to be converted.
  Returns:
    The input string converted to headlessCamelCase.
  """
  return ''.join([snake_string.split('_')[0]] +
                 list(sub_string.capitalize()
                      for sub_string in snake_string.split('_')[1:]))
