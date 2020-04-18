# Copyright 2016 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""A library supporting use of the Google API Server.

This library helps you configure a set of ProtoRPC services to act as
Endpoints backends.  In addition to translating ProtoRPC to Endpoints
compatible errors, it exposes a helper service that describes your services.

  Usage:
  1) Create an endpoints.api_server instead of a webapp.WSGIApplication.
  2) Annotate your ProtoRPC Service class with @endpoints.api to give your
     API a name, version, and short description
  3) To return an error from Google API Server raise an endpoints.*Exception
     The ServiceException classes specify the http status code returned.

     For example:
     raise endpoints.UnauthorizedException("Please log in as an admin user")


  Sample usage:
  - - - - app.yaml - - - -

  handlers:
  # Path to your API backend.
  # /_ah/api/.* is the default. Using the base_path parameter, you can
  # customize this to whichever base path you desire.
  - url: /_ah/api/.*
    # For the legacy python runtime this would be "script: services.py"
    script: services.app

  - - - - services.py - - - -

  import endpoints
  import postservice

  app = endpoints.api_server([postservice.PostService], debug=True)

  - - - - postservice.py - - - -

  @endpoints.api(name='guestbook', version='v0.2', description='Guestbook API')
  class PostService(remote.Service):
    ...
    @endpoints.method(GetNotesRequest, Notes, name='notes.list', path='notes',
                       http_method='GET')
    def list(self, request):
      raise endpoints.UnauthorizedException("Please log in as an admin user")
"""

import cgi
import httplib
import json
import logging
import os

import api_backend_service
import api_config
import api_exceptions
import endpoints_dispatcher
import protojson

from google.appengine.api import app_identity
from google.api.control import client as control_client
from google.api.control import wsgi as control_wsgi

from protorpc import messages
from protorpc import remote
from protorpc.wsgi import service as wsgi_service

import util


_logger = logging.getLogger(__name__)
package = 'google.appengine.endpoints'


__all__ = [
    'api_server',
    'EndpointsErrorMessage',
    'package',
]


class _Remapped405Exception(api_exceptions.ServiceException):
  """Method Not Allowed (405) ends up being remapped to 501.

  This is included here for compatibility with the Java implementation.  The
  Google Cloud Endpoints server remaps HTTP 405 to 501.
  """
  http_status = httplib.METHOD_NOT_ALLOWED


class _Remapped408Exception(api_exceptions.ServiceException):
  """Request Timeout (408) ends up being remapped to 503.

  This is included here for compatibility with the Java implementation.  The
  Google Cloud Endpoints server remaps HTTP 408 to 503.
  """
  http_status = httplib.REQUEST_TIMEOUT


_ERROR_NAME_MAP = dict((httplib.responses[c.http_status], c) for c in [
    api_exceptions.BadRequestException,
    api_exceptions.UnauthorizedException,
    api_exceptions.ForbiddenException,
    api_exceptions.NotFoundException,
    _Remapped405Exception,
    _Remapped408Exception,
    api_exceptions.ConflictException,
    api_exceptions.GoneException,
    api_exceptions.PreconditionFailedException,
    api_exceptions.RequestEntityTooLargeException,
    api_exceptions.InternalServerErrorException
    ])

_ALL_JSON_CONTENT_TYPES = frozenset(
    [protojson.EndpointsProtoJson.CONTENT_TYPE] +
    protojson.EndpointsProtoJson.ALTERNATIVE_CONTENT_TYPES)


# Message format for returning error back to Google Endpoints frontend.
class EndpointsErrorMessage(messages.Message):
  """Message for returning error back to Google Endpoints frontend.

  Fields:
    state: State of RPC, should be 'APPLICATION_ERROR'.
    error_message: Error message associated with status.
  """

  class State(messages.Enum):
    """Enumeration of possible RPC states.

    Values:
      OK: Completed successfully.
      RUNNING: Still running, not complete.
      REQUEST_ERROR: Request was malformed or incomplete.
      SERVER_ERROR: Server experienced an unexpected error.
      NETWORK_ERROR: An error occured on the network.
      APPLICATION_ERROR: The application is indicating an error.
        When in this state, RPC should also set application_error.
    """
    OK = 0
    RUNNING = 1

    REQUEST_ERROR = 2
    SERVER_ERROR = 3
    NETWORK_ERROR = 4
    APPLICATION_ERROR = 5
    METHOD_NOT_FOUND_ERROR = 6

  state = messages.EnumField(State, 1, required=True)
  error_message = messages.StringField(2)


# pylint: disable=g-bad-name
def _get_app_revision(environ=None):
  """Gets the app revision (minor app version) of the current app.

  Args:
    environ: A dictionary with a key CURRENT_VERSION_ID that maps to a version
      string of the format <major>.<minor>.

  Returns:
    The app revision (minor version) of the current app, or None if one couldn't
    be found.
  """
  if environ is None:
    environ = os.environ
  if 'CURRENT_VERSION_ID' in environ:
    return environ['CURRENT_VERSION_ID'].split('.')[1]


class _ApiServer(object):
  """ProtoRPC wrapper, registers APIs and formats errors for Google API Server.

  - - - - ProtoRPC error format - - - -
  HTTP/1.0 400 Please log in as an admin user.
  content-type: application/json

  {
    "state": "APPLICATION_ERROR",
    "error_message": "Please log in as an admin user",
    "error_name": "unauthorized",
  }

  - - - - Reformatted error format - - - -
  HTTP/1.0 401 UNAUTHORIZED
  content-type: application/json

  {
    "state": "APPLICATION_ERROR",
    "error_message": "Please log in as an admin user"
  }
  """
  # Silence lint warning about invalid const name
  # pylint: disable=g-bad-name
  __SERVER_SOFTWARE = 'SERVER_SOFTWARE'
  __HEADER_NAME_PEER = 'HTTP_X_APPENGINE_PEER'
  __GOOGLE_PEER = 'apiserving'
  # A common EndpointsProtoJson for all _ApiServer instances.  At the moment,
  # EndpointsProtoJson looks to be thread safe.
  __PROTOJSON = protojson.EndpointsProtoJson()

  def __init__(self, api_services, **kwargs):
    """Initialize an _ApiServer instance.

    The primary function of this method is to set up the WSGIApplication
    instance for the service handlers described by the services passed in.
    Additionally, it registers each API in ApiConfigRegistry for later use
    in the BackendService.getApiConfigs() (API config enumeration service).

    Args:
      api_services: List of protorpc.remote.Service classes implementing the API
        or a list of _ApiDecorator instances that decorate the service classes
        for an API.
      **kwargs: Passed through to protorpc.wsgi.service.service_handlers except:
        protocols - ProtoRPC protocols are not supported, and are disallowed.

    Raises:
      TypeError: if protocols are configured (this feature is not supported).
      ApiConfigurationError: if there's a problem with the API config.
    """
    self.base_paths = set()

    for entry in api_services[:]:
      # pylint: disable=protected-access
      if isinstance(entry, api_config._ApiDecorator):
        api_services.remove(entry)
        api_services.extend(entry.get_api_classes())
        self.base_paths.add(entry.base_path)

    # Record the API services for quick discovery doc generation
    self.api_services = api_services

    # Record the base paths
    for entry in api_services:
      self.base_paths.add(entry.api_info.base_path)

    self.api_config_registry = api_backend_service.ApiConfigRegistry()
    self.api_name_version_map = self.__create_name_version_map(api_services)
    protorpc_services = self.__register_services(self.api_name_version_map,
                                                 self.api_config_registry)

    # Disallow protocol configuration for now, Lily is json-only.
    if 'protocols' in kwargs:
      raise TypeError('__init__() got an unexpected keyword argument '
                      "'protocols'")
    protocols = remote.Protocols()
    protocols.add_protocol(self.__PROTOJSON, 'protojson')
    remote.Protocols.set_default(protocols)

    # This variable is not used in Endpoints 1.1, but let's pop it out here
    # so it doesn't result in an unexpected keyword argument downstream.
    kwargs.pop('restricted', None)

    self.service_app = wsgi_service.service_mappings(protorpc_services,
                                                     **kwargs)

  @staticmethod
  def __create_name_version_map(api_services):
    """Create a map from API name/version to Service class/factory.

    This creates a map from an API name and version to a list of remote.Service
    factories that implement that API.

    Args:
      api_services: A list of remote.Service-derived classes or factories
        created with remote.Service.new_factory.

    Returns:
      A mapping from (api name, api version) to a list of service factories,
      for service classes that implement that API.

    Raises:
      ApiConfigurationError: If a Service class appears more than once
        in api_services.
    """
    api_name_version_map = {}
    for service_factory in api_services:
      try:
        service_class = service_factory.service_class
      except AttributeError:
        service_class = service_factory
        service_factory = service_class.new_factory()

      key = service_class.api_info.name, service_class.api_info.version
      service_factories = api_name_version_map.setdefault(key, [])
      if service_factory in service_factories:
        raise api_config.ApiConfigurationError(
            'Can\'t add the same class to an API twice: %s' %
            service_factory.service_class.__name__)

      service_factories.append(service_factory)
    return api_name_version_map

  @staticmethod
  def __register_services(api_name_version_map, api_config_registry):
    """Register & return a list of each URL and class that handles that URL.

    This finds every service class in api_name_version_map, registers it with
    the given ApiConfigRegistry, builds the URL for that class, and adds
    the URL and its factory to a list that's returned.

    Args:
      api_name_version_map: A mapping from (api name, api version) to a list of
        service factories, as returned by __create_name_version_map.
      api_config_registry: The ApiConfigRegistry where service classes will
        be registered.

    Returns:
      A list of (URL, service_factory) for each service class in
      api_name_version_map.

    Raises:
      ApiConfigurationError: If a Service class appears more than once
        in api_name_version_map.  This could happen if one class is used to
        implement multiple APIs.
    """
    generator = api_config.ApiConfigGenerator()
    protorpc_services = []
    for service_factories in api_name_version_map.itervalues():
      service_classes = [service_factory.service_class
                         for service_factory in service_factories]
      config_file = generator.pretty_print_config_to_json(service_classes)
      api_config_registry.register_backend(config_file)

      for service_factory in service_factories:
        protorpc_class_name = service_factory.service_class.__name__
        root = '%s%s' % (service_factory.service_class.api_info.base_path,
                         protorpc_class_name)
        if any(service_map[0] == root or service_map[1] == service_factory
               for service_map in protorpc_services):
          raise api_config.ApiConfigurationError(
              'Can\'t reuse the same class in multiple APIs: %s' %
              protorpc_class_name)
        protorpc_services.append((root, service_factory))
    return protorpc_services

  def __is_json_error(self, status, headers):
    """Determine if response is an error.

    Args:
      status: HTTP status code.
      headers: Dictionary of (lowercase) header name to value.

    Returns:
      True if the response was an error, else False.
    """
    content_header = headers.get('content-type', '')
    content_type, unused_params = cgi.parse_header(content_header)
    return (status.startswith('400') and
            content_type.lower() in _ALL_JSON_CONTENT_TYPES)

  def __write_error(self, status_code, error_message=None):
    """Return the HTTP status line and body for a given error code and message.

    Args:
      status_code: HTTP status code to be returned.
      error_message: Error message to be returned.

    Returns:
      Tuple (http_status, body):
        http_status: HTTP status line, e.g. 200 OK.
        body: Body of the HTTP request.
    """
    if error_message is None:
      error_message = httplib.responses[status_code]
    status = '%d %s' % (status_code, httplib.responses[status_code])
    message = EndpointsErrorMessage(
        state=EndpointsErrorMessage.State.APPLICATION_ERROR,
        error_message=error_message)
    return status, self.__PROTOJSON.encode_message(message)

  def protorpc_to_endpoints_error(self, status, body):
    """Convert a ProtoRPC error to the format expected by Google Endpoints.

    If the body does not contain an ProtoRPC message in state APPLICATION_ERROR
    the status and body will be returned unchanged.

    Args:
      status: HTTP status of the response from the backend
      body: JSON-encoded error in format expected by Endpoints frontend.

    Returns:
      Tuple of (http status, body)
    """
    try:
      rpc_error = self.__PROTOJSON.decode_message(remote.RpcStatus, body)
    except (ValueError, messages.ValidationError):
      rpc_error = remote.RpcStatus()

    if rpc_error.state == remote.RpcStatus.State.APPLICATION_ERROR:

      # Try to map to HTTP error code.
      error_class = _ERROR_NAME_MAP.get(rpc_error.error_name)
      if error_class:
        status, body = self.__write_error(error_class.http_status,
                                          rpc_error.error_message)
    return status, body

  def get_api_configs(self):
    return {
        'items': [json.loads(c) for c in
                  self.api_config_registry.all_api_configs()]}

  def __call__(self, environ, start_response):
    """Wrapper for the Endpoints server app.

    Args:
      environ: WSGI request environment.
      start_response: WSGI start response function.

    Returns:
      Response from service_app or appropriately transformed error response.
    """
    # Call the ProtoRPC App and capture its response
    with util.StartResponseProxy() as start_response_proxy:
      body_iter = self.service_app(environ, start_response_proxy.Proxy)
      status = start_response_proxy.response_status
      headers = start_response_proxy.response_headers
      exception = start_response_proxy.response_exc_info

      # Get response body
      body = start_response_proxy.response_body
      # In case standard WSGI behavior is implemented later...
      if not body:
        body = ''.join(body_iter)

    # Transform ProtoRPC error into format expected by endpoints.
    headers_dict = dict([(k.lower(), v) for k, v in headers])
    if self.__is_json_error(status, headers_dict):
      status, body = self.protorpc_to_endpoints_error(status, body)
      # If the content-length header is present, update it with the new
      # body length.
      if 'content-length' in headers_dict:
        for index, (header_name, _) in enumerate(headers):
          if header_name.lower() == 'content-length':
            headers[index] = (header_name, str(len(body)))
            break

    start_response(status, headers, exception)
    return [body]


# Silence lint warning about invalid function name
# pylint: disable=g-bad-name
def api_server(api_services, **kwargs):
  """Create an api_server.

  The primary function of this method is to set up the WSGIApplication
  instance for the service handlers described by the services passed in.
  Additionally, it registers each API in ApiConfigRegistry for later use
  in the BackendService.getApiConfigs() (API config enumeration service).
  It also configures service control.

  Args:
    api_services: List of protorpc.remote.Service classes implementing the API
      or a list of _ApiDecorator instances that decorate the service classes
      for an API.
    **kwargs: Passed through to protorpc.wsgi.service.service_handlers except:
      protocols - ProtoRPC protocols are not supported, and are disallowed.

  Returns:
    A new WSGIApplication that serves the API backend and config registry.

  Raises:
    TypeError: if protocols are configured (this feature is not supported).
  """
  # Disallow protocol configuration for now, Lily is json-only.
  if 'protocols' in kwargs:
    raise TypeError("__init__() got an unexpected keyword argument 'protocols'")

  # Construct the api serving app
  apis_app = _ApiServer(api_services, **kwargs)
  dispatcher = endpoints_dispatcher.EndpointsDispatcherMiddleware(apis_app)

  # Determine the service name
  service_name = os.environ.get('ENDPOINTS_SERVICE_NAME')
  if not service_name:
    _logger.warn('Did not specify the ENDPOINTS_SERVICE_NAME environment'
                 ' variable so service control is disabled.  Please specify'
                 ' the name of service in ENDPOINTS_SERVICE_NAME to enable'
                 ' it.')
    return dispatcher

  # If we're using a local server, just return the dispatcher now to bypass
  # control client.
  if control_wsgi.running_on_devserver():
    _logger.warn('Running on local devserver, so service control is disabled.')
    return dispatcher

  # The DEFAULT 'config' should be tuned so that it's always OK for python
  # App Engine workloads.  The config can be adjusted, but that's probably
  # unnecessary on App Engine.
  controller = control_client.Loaders.DEFAULT.load(service_name)

  # Start the GAE background thread that powers the control client's cache.
  control_client.use_gae_thread()
  controller.start()

  return control_wsgi.add_all(
      dispatcher,
      app_identity.get_application_id(),
      controller)
