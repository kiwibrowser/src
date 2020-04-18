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

"""Hook into the live Discovery service and get API configuration info."""

# pylint: disable=g-bad-name
import json
import logging

import api_config
import discovery_api_proxy
import discovery_generator
import util


class DiscoveryService(object):
  """Implements the local discovery service.

  This has a static minimal version of the discoverable part of the
  discovery .api file.

  It only handles returning the discovery doc and directory, and ignores
  directory parameters to filter the results.

  The discovery docs/directory are created by calling a Cloud Endpoints
  discovery service to generate the discovery docs/directory from an .api
  file/set of .api files.
  """

  _GET_REST_API = 'apisdev.getRest'
  _GET_RPC_API = 'apisdev.getRpc'
  _LIST_API = 'apisdev.list'
  API_CONFIG = {
      'name': 'discovery',
      'version': 'v1',
      'methods': {
          'discovery.apis.getRest': {
              'path': 'apis/{api}/{version}/rest',
              'httpMethod': 'GET',
              'rosyMethod': _GET_REST_API,
          },
          'discovery.apis.getRpc': {
              'path': 'apis/{api}/{version}/rpc',
              'httpMethod': 'GET',
              'rosyMethod': _GET_RPC_API,
          },
          'discovery.apis.list': {
              'path': 'apis',
              'httpMethod': 'GET',
              'rosyMethod': _LIST_API,
          },
      }
  }

  def __init__(self, config_manager, backend):
    """Initializes an instance of the DiscoveryService.

    Args:
      config_manager: An instance of ApiConfigManager.
      backend: An _ApiServer instance for API config generation.
    """
    self._config_manager = config_manager
    self._backend = backend
    self._discovery_proxy = discovery_api_proxy.DiscoveryApiProxy()

  def _send_success_response(self, response, start_response):
    """Sends an HTTP 200 json success response.

    This calls start_response and returns the response body.

    Args:
      response: A string containing the response body to return.
      start_response: A function with semantics defined in PEP-333.

    Returns:
      A string, the response body.
    """
    headers = [('Content-Type', 'application/json; charset=UTF-8')]
    return util.send_wsgi_response('200', headers, response, start_response)

  def _get_rest_doc(self, request, start_response):
    """Sends back HTTP response with API directory.

    This calls start_response and returns the response body.  It will return
    the discovery doc for the requested api/version.

    Args:
      request: An ApiRequest, the transformed request sent to the Discovery API.
      start_response: A function with semantics defined in PEP-333.

    Returns:
      A string, the response body.
    """
    api = request.body_json['api']
    version = request.body_json['version']

    generator = discovery_generator.DiscoveryGenerator()
    doc = generator.pretty_print_config_to_json(self._backend.api_services)
    if not doc:
      error_msg = ('Failed to convert .api to discovery doc for '
                   'version %s of api %s') % (version, api)
      logging.error('%s', error_msg)
      return util.send_wsgi_error_response(error_msg, start_response)
    return self._send_success_response(doc, start_response)

  def _generate_api_config_with_root(self, request):
    """Generate an API config with a specific root hostname.

    This uses the backend object and the ApiConfigGenerator to create an API
    config specific to the hostname of the incoming request. This allows for
    flexible API configs for non-standard environments, such as localhost.

    Args:
      request: An ApiRequest, the transformed request sent to the Discovery API.

    Returns:
      A string representation of the generated API config.
    """
    actual_root = self._get_actual_root(request)
    generator = api_config.ApiConfigGenerator()
    api = request.body_json['api']
    version = request.body_json['version']
    lookup_key = (api, version)

    service_factories = self._backend.api_name_version_map.get(lookup_key)
    if not service_factories:
      return None

    service_classes = [service_factory.service_class
                       for service_factory in service_factories]
    config_dict = generator.get_config_dict(
        service_classes, hostname=actual_root)

    # Save to cache
    for config in config_dict.get('items', []):
      lookup_key_with_root = (
          config.get('name', ''), config.get('version', ''), actual_root)
      self._config_manager.save_config(lookup_key_with_root, config)

    return config_dict

  def _get_actual_root(self, request):
    url = request.server

    # Append the port if not the default
    if ((request.url_scheme == 'https' and request.port != '443') or
        (request.url_scheme != 'https' and request.port != '80')):
      url += ':%s' % request.port

    return url

  def _list(self, start_response):
    """Sends HTTP response containing the API directory.

    This calls start_response and returns the response body.

    Args:
      start_response: A function with semantics defined in PEP-333.

    Returns:
      A string containing the response body.
    """
    configs = []
    for config in self._config_manager.configs.itervalues():
      if config != self.API_CONFIG:
        configs.append(json.dumps(config))
    directory = self._discovery_proxy.generate_directory(configs)
    if not directory:
      logging.error('Failed to get API directory')
      # By returning a 404, code explorer still works if you select the
      # API in the URL
      return util.send_wsgi_not_found_response(start_response)
    return self._send_success_response(directory, start_response)

  def handle_discovery_request(self, path, request, start_response):
    """Returns the result of a discovery service request.

    This calls start_response and returns the response body.

    Args:
      path: A string containing the API path (the portion of the path
        after /_ah/api/).
      request: An ApiRequest, the transformed request sent to the Discovery API.
      start_response: A function with semantics defined in PEP-333.

    Returns:
      The response body.  Or returns False if the request wasn't handled by
      DiscoveryService.
    """
    if path == self._GET_REST_API:
      return self._get_rest_doc(request, start_response)
    elif path == self._GET_RPC_API:
      error_msg = ('RPC format documents are no longer supported with the '
                   'Endpoints Framework for Python. Please use the REST '
                   'format.')
      logging.error('%s', error_msg)
      return util.send_wsgi_error_response(error_msg, start_response)
    elif path == self._LIST_API:
      return self._list(start_response)
    return False
