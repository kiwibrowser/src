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

"""wsgi implement behaviour that provides service control as wsgi
middleware.

It provides the :class:`Middleware`, which is a WSGI middleware implementation
that wraps another WSGI application to uses a provided
:class:`google.api.control.client.Client` to provide service control.

"""
#pylint: disable=too-many-arguments

from __future__ import absolute_import

from datetime import datetime
import httplib
import logging
import os
import socket
import uuid
import urllib2
import urlparse
import wsgiref.util

from google.api.auth import suppliers, tokens
from . import check_request, messages, report_request, service


logger = logging.getLogger(__name__)


_CONTENT_LENGTH = 'content-length'
_DEFAULT_LOCATION = 'global'

_METADATA_SERVER_URL = 'http://metadata.google.internal'


def _running_on_gce():
  headers = {'Metadata-Flavor': 'Google'}

  try:
    request = urllib2.Request(_METADATA_SERVER_URL, headers=headers)
    response = urllib2.urlopen(request)
    if response.info().getheader('Metadata-Flavor') == 'Google':
      return True
  except (urllib2.URLError, socket.error):
    pass

  return False


def _get_platform():
  server_software = os.environ.get('SERVER_SOFTWARE', '')

  if server_software.startswith('Development'):
    return report_request.ReportedPlatforms.DEVELOPMENT
  elif os.environ.get('KUBERNETES_SERVICE_HOST'):
    return report_request.ReportedPlatforms.GKE
  elif _running_on_gce():
    # We're either in GAE Flex or GCE
    if os.environ.get('GAE_MODULE_NAME'):
      return report_request.ReportedPlatforms.GAE_FLEX
    else:
      return report_request.ReportedPlatforms.GCE
  elif os.environ.get('GAE_MODULE_NAME'):
    return report_request.ReportedPlatforms.GAE_STANDARD

  return report_request.ReportedPlatforms.UNKNOWN


platform = _get_platform()


def running_on_devserver():
  return platform == report_request.ReportedPlatforms.DEVELOPMENT


def add_all(application, project_id, control_client,
            loader=service.Loaders.FROM_SERVICE_MANAGEMENT):
    """Adds all endpoints middleware to a wsgi application.

    Sets up application to use all default endpoints middleware.

    Example:

      >>> application = MyWsgiApp()  # an existing WSGI application
      >>>
      >>> # the name of the controlled service
      >>> service_name = 'my-service-name'
      >>>
      >>> # A GCP project  with service control enabled
      >>> project_id = 'my-project-id'
      >>>
      >>> # wrap the app for service control
      >>> from google.api.control import wsgi
      >>> control_client = client.Loaders.DEFAULT.load(service_name)
      >>> control_client.start()
      >>> wrapped_app = add_all(application, project_id, control_client)
      >>>
      >>> # now use wrapped_app in place of app

    Args:
       application: the wrapped wsgi application
       project_id: the project_id thats providing service control support
       control_client: the service control client instance
       loader (:class:`google.api.control.service.Loader`): loads the service
          instance that configures this instance's behaviour
    """
    a_service = loader.load()
    if not a_service:
        raise ValueError("Failed to load service config")
    authenticator = _create_authenticator(a_service)

    wrapped_app = Middleware(application, project_id, control_client)
    if authenticator:
        wrapped_app = AuthenticationMiddleware(wrapped_app, authenticator)
    return EnvironmentMiddleware(wrapped_app, a_service)


def _next_operation_uuid():
    return uuid.uuid4().hex


class EnvironmentMiddleware(object):
    """A WSGI middleware that sets related variables in the environment.

    It attempts to add the following vars:

    - google.api.config.service
    - google.api.config.service_name
    - google.api.config.method_registry
    - google.api.config.reporting_rules
    - google.api.config.method_info
    """
    # pylint: disable=too-few-public-methods

    SERVICE = 'google.api.config.service'
    SERVICE_NAME = 'google.api.config.service_name'
    METHOD_REGISTRY = 'google.api.config.method_registry'
    METHOD_INFO = 'google.api.config.method_info'
    REPORTING_RULES = 'google.api.config.reporting_rules'

    def __init__(self, application, a_service):
        """Initializes a new Middleware instance.

        Args:
          application: the wrapped wsgi application
          a_service (:class:`google.api.gen.servicecontrol_v1_messages.Service`):
            a service instance
        """
        if not isinstance(a_service, messages.Service):
            raise ValueError("service is None or not an instance of Service")

        self._application = application
        self._service = a_service

        method_registry, reporting_rules = self._configure()
        self._method_registry = method_registry
        self._reporting_rules = reporting_rules

    def _configure(self):
        registry = service.MethodRegistry(self._service)
        logs, metric_names, label_names = service.extract_report_spec(self._service)
        reporting_rules = report_request.ReportingRules.from_known_inputs(
            logs=logs,
            metric_names=metric_names,
            label_names=label_names)

        return registry, reporting_rules

    def __call__(self, environ, start_response):
        environ[self.SERVICE] = self._service
        environ[self.SERVICE_NAME] = self._service.name
        environ[self.METHOD_REGISTRY] = self._method_registry
        environ[self.REPORTING_RULES] = self._reporting_rules
        parsed_uri = urlparse.urlparse(wsgiref.util.request_uri(environ))
        http_method = environ.get('REQUEST_METHOD')
        method_info = self._method_registry.lookup(http_method, parsed_uri.path)
        if method_info:
            environ[self.METHOD_INFO] = method_info

        return self._application(environ, start_response)


class Middleware(object):
    """A WSGI middleware implementation that provides service control.

    Example:

      >>> app = MyWsgiApp()  # an existing WSGI application
      >>>
      >>> # the name of the controlled service
      >>> service_name = 'my-service-name'
      >>>
      >>> # A GCP project  with service control enabled
      >>> project_id = 'my-project-id'
      >>>
      >>> # wrap the app for service control
      >>> from google.api.control import client, wsgi, service
      >>> control_client = client.Loaders.DEFAULT.load(service_name)
      >>> control_client.start()
      >>> wrapped_app = wsgi.Middleware(app, control_client, project_id)
      >>> env_app = wsgi.EnvironmentMiddleware(wrapped,app)
      >>>
      >>> # now use env_app in place of app

    """
    # pylint: disable=too-few-public-methods, fixme
    _NO_API_KEY_MSG = (
        'Method does not allow callers without established identity.'
        ' Please use an API key or other form of API consumer identity'
        ' to call this API.'
     )

    def __init__(self,
                 application,
                 project_id,
                 control_client,
                 next_operation_id=_next_operation_uuid,
                 timer=datetime.utcnow):
        """Initializes a new Middleware instance.

        Args:
           application: the wrapped wsgi application
           project_id: the project_id thats providing service control support
           control_client: the service control client instance
           next_operation_id (func): produces the next operation
           timer (func[[datetime.datetime]]): a func that obtains the current time
           """
        self._application = application
        self._project_id = project_id
        self._next_operation_id = next_operation_id
        self._control_client = control_client
        self._timer = timer

    def __call__(self, environ, start_response):
        # pylint: disable=too-many-locals
        method_info = environ.get(EnvironmentMiddleware.METHOD_INFO)
        if not method_info:
            # just allow the wrapped application to handle the request
            logger.debug('method_info not present in the wsgi environment'
                         ', no service control')
            return self._application(environ, start_response)

        latency_timer = _LatencyTimer(self._timer)
        latency_timer.start()

        # Determine if the request can proceed
        http_method = environ.get('REQUEST_METHOD')
        parsed_uri = urlparse.urlparse(wsgiref.util.request_uri(environ))
        app_info = _AppInfo()
        # TODO: determine if any of the more complex ways of getting the request size
        # (e.g) buffering and counting the wsgi input stream is more appropriate here
        try:
            app_info.request_size = int(environ.get('CONTENT_LENGTH',
                                                    report_request.SIZE_NOT_SET))
        except ValueError:
            logger.warn('ignored bad content-length: %s', environ.get('CONTENT_LENGTH'))

        app_info.http_method = http_method
        app_info.url = parsed_uri

        check_info = self._create_check_info(method_info, parsed_uri, environ)
        if not check_info.api_key and not method_info.allow_unregistered_calls:
            logger.debug("skipping %s, no api key was provided", parsed_uri)
            error_msg = self._handle_missing_api_key(app_info, start_response)
        else:
            check_req = check_info.as_check_request()
            logger.debug('checking %s with %s', method_info, check_request)
            check_resp = self._control_client.check(check_req)
            error_msg = self._handle_check_response(app_info, check_resp, start_response)

        if error_msg:
            # send a report request that indicates that the request failed
            rules = environ.get(EnvironmentMiddleware.REPORTING_RULES)
            latency_timer.end()
            report_req = self._create_report_request(method_info,
                                                     check_info,
                                                     app_info,
                                                     latency_timer,
                                                     rules)
            logger.debug('scheduling report_request %s', report_req)
            self._control_client.report(report_req)
            return error_msg

        # update the client with the response
        latency_timer.app_start()

        # run the application request in an inner handler that sets the status
        # and response code on app_info
        def inner_start_response(status, response_headers, exc_info=None):
            app_info.response_code = int(status.partition(' ')[0])
            for name, value in response_headers:
                if name.lower() == _CONTENT_LENGTH:
                    app_info.response_size = int(value)
                    break
            return start_response(status, response_headers, exc_info)

        result = self._application(environ, inner_start_response)

        # perform reporting, result must be joined otherwise the latency record
        # is incorrect
        result = b''.join(result)
        latency_timer.end()
        app_info.response_size = len(result)
        rules = environ.get(EnvironmentMiddleware.REPORTING_RULES)
        report_req = self._create_report_request(method_info,
                                                 check_info,
                                                 app_info,
                                                 latency_timer,
                                                 rules)
        logger.debug('scheduling report_request %s', report_req)
        self._control_client.report(report_req)
        return result

    def _create_report_request(self,
                               method_info,
                               check_info,
                               app_info,
                               latency_timer,
                               reporting_rules):
        # TODO: determine how to obtain the consumer_project_id and the location
        # correctly
        report_info = report_request.Info(
            api_key=check_info.api_key,
            api_key_valid=app_info.api_key_valid,
            api_method=method_info.selector,
            consumer_project_id=self._project_id,  # TODO: see above
            location=_DEFAULT_LOCATION,  # TODO: see above
            method=app_info.http_method,
            operation_id=check_info.operation_id,
            operation_name=check_info.operation_name,
            backend_time=latency_timer.backend_time,
            overhead_time=latency_timer.overhead_time,
            platform=platform,
            producer_project_id=self._project_id,
            protocol=report_request.ReportedProtocols.HTTP,
            request_size=app_info.request_size,
            request_time=latency_timer.request_time,
            response_code=app_info.response_code,
            response_size=app_info.response_size,
            referer=check_info.referer,
            service_name=check_info.service_name,
            url=app_info.url
        )
        return report_info.as_report_request(reporting_rules, timer=self._timer)

    def _create_check_info(self, method_info, parsed_uri, environ):
        service_name = environ.get(EnvironmentMiddleware.SERVICE_NAME)
        operation_id = self._next_operation_id()
        api_key_valid = False
        api_key = _find_api_key_param(method_info, parsed_uri)
        if not api_key:
            api_key = _find_api_key_header(method_info, environ)
        if not api_key:
            api_key = _find_default_api_key_param(parsed_uri)

        if api_key:
            api_key_valid = True

        check_info = check_request.Info(
            api_key=api_key,
            api_key_valid=api_key_valid,
            client_ip=environ.get('REMOTE_ADDR', ''),
            consumer_project_id=self._project_id,  # TODO: switch this to producer_project_id
            operation_id=operation_id,
            operation_name=method_info.selector,
            referer=environ.get('HTTP_REFERER', ''),
            service_name=service_name
        )
        return check_info

    def _handle_check_response(self, app_info, check_resp, start_response):
        code, detail, api_key_valid = check_request.convert_response(
            check_resp, self._project_id)
        if code == httplib.OK:
            return None  # the check was OK

        # there was problem; the request cannot proceed
        logger.warn('Check failed %d, %s', code, detail)
        error_msg = '%d %s' % (code, detail)
        start_response(error_msg, [])
        app_info.response_code = code
        app_info.api_key_valid = api_key_valid
        return error_msg  # the request cannot continue

    def _handle_missing_api_key(self, app_info, start_response):
        code = httplib.UNAUTHORIZED
        detail = self._NO_API_KEY_MSG
        logger.warn('Check not performed %d, %s', code, detail)
        error_msg = '%d %s' % (code, detail)
        start_response(error_msg, [])
        app_info.response_code = code
        app_info.api_key_valid = False
        return error_msg  # the request cannot continue


class _AppInfo(object):
    # pylint: disable=too-few-public-methods

    def __init__(self):
        self.api_key_valid = True
        self.response_code = httplib.INTERNAL_SERVER_ERROR
        self.response_size = report_request.SIZE_NOT_SET
        self.request_size = report_request.SIZE_NOT_SET
        self.http_method = None
        self.url = None


class _LatencyTimer(object):

    def __init__(self, timer):
        self._timer = timer
        self._start = None
        self._app_start = None
        self._end = None

    def start(self):
        self._start = self._timer()

    def app_start(self):
        self._app_start = self._timer()

    def end(self):
        self._end = self._timer()
        if self._app_start is None:
            self._app_start = self._end

    @property
    def request_time(self):
        if self._start and self._end:
            return self._end - self._start
        return None

    @property
    def overhead_time(self):
        if self._start and self._app_start:
            return self._app_start - self._start
        return None

    @property
    def backend_time(self):
        if self._end and self._app_start:
            return self._end - self._app_start
        return None


def _find_api_key_param(info, parsed_uri):
    params = info.api_key_url_query_params
    if not params:
        return None

    param_dict = urlparse.parse_qs(parsed_uri.query)
    if not param_dict:
        return None

    for q in params:
        value = param_dict.get(q)
        if value:
            # param's values are lists, assume the first value
            # is what's needed
            return value[0]

    return None


_DEFAULT_API_KEYS = ('key', 'api_key')


def _find_default_api_key_param(parsed_uri):
    param_dict = urlparse.parse_qs(parsed_uri.query)
    if not param_dict:
        return None

    for q in _DEFAULT_API_KEYS:
        value = param_dict.get(q)
        if value:
            # param's values are lists, assume the first value
            # is what's needed
            return value[0]

    return None


def _find_api_key_header(info, environ):
    headers = info.api_key_http_header
    if not headers:
        return None

    for h in headers:
        value = environ.get('HTTP_' + h.upper())
        if value:
            return value  # headers have single values

    return None

def _create_authenticator(a_service):
    """Create an instance of :class:`google.auth.tokens.Authenticator`.

    Args:
      a_service (:class:`google.api.gen.servicecontrol_v1_messages.Service`): a
        service instance
    """
    if not isinstance(a_service, messages.Service):
        raise ValueError("service is None or not an instance of Service")

    authentication = a_service.authentication
    if not authentication:
        logger.info("authentication is not configured in service, "
                    "authentication checks will be disabled")
        return

    issuers_to_provider_ids = {}
    issuer_uri_configs = {}
    for provider in authentication.providers:
        issuer = provider.issuer
        jwks_uri = provider.jwksUri

        # Enable openID discovery if jwks_uri is unset
        open_id = jwks_uri is None
        issuer_uri_configs[issuer] = suppliers.IssuerUriConfig(open_id, jwks_uri)
        issuers_to_provider_ids[issuer] = provider.id

    key_uri_supplier = suppliers.KeyUriSupplier(issuer_uri_configs)
    jwks_supplier = suppliers.JwksSupplier(key_uri_supplier)
    authenticator = tokens.Authenticator(issuers_to_provider_ids, jwks_supplier)
    return authenticator


class AuthenticationMiddleware(object):
    """A WSGI middleware that does authentication checks for incoming
    requests.

    In environments where os.environ is replaced with a request-local and
    thread-independent copy (e.g. Google Appengine), authentication result is
    added to os.environ so that the wrapped application can make use of the
    authentication result.
    """
    # pylint: disable=too-few-public-methods

    USER_INFO = "google.api.auth.user_info"

    def __init__(self, application, authenticator):
        """Initializes an authentication middleware instance.

        Args:
          application: a WSGI application to be wrapped
          authenticator (:class:`google.auth.tokens.Authenticator`): an
            authenticator that authenticates incoming requests
        """
        if not isinstance(authenticator, tokens.Authenticator):
            raise ValueError("Invalid authenticator")

        self._application = application
        self._authenticator = authenticator

    def __call__(self, environ, start_response):
        method_info = environ.get(EnvironmentMiddleware.METHOD_INFO)
        if not method_info or not method_info.auth_info:
            # No authentication configuration for this method
            logger.debug("authentication is not configured")
            return self._application(environ, start_response)

        auth_token = _extract_auth_token(environ)
        user_info = None
        if not auth_token:
            logger.debug("No auth token is attached to the request")
        else:
            try:
                service_name = environ.get(EnvironmentMiddleware.SERVICE_NAME)
                user_info = self._authenticator.authenticate(auth_token,
                                                             method_info.auth_info,
                                                             service_name)
            except Exception:  # pylint: disable=broad-except
                logger.debug("Cannot decode and verify the auth token. The backend "
                             "will not be able to retrieve user info", exc_info=True)

        environ[self.USER_INFO] = user_info

        # pylint: disable=protected-access
        if user_info and not isinstance(os.environ, os._Environ):
            # Set user info into os.environ only if os.environ is replaced
            # with a request-local copy
            os.environ[self.USER_INFO] = user_info

        response = self._application(environ, start_response)

        # Erase user info from os.environ for safety and sanity.
        if self.USER_INFO in os.environ:
            del os.environ[self.USER_INFO]

        return response


_ACCESS_TOKEN_PARAM_NAME = "access_token"
_BEARER_TOKEN_PREFIX = "Bearer "
_BEARER_TOKEN_PREFIX_LEN = len(_BEARER_TOKEN_PREFIX)


def _extract_auth_token(environ):
    # First try to extract auth token from HTTP authorization header.
    auth_header = environ.get("HTTP_AUTHORIZATION")
    if auth_header:
        if auth_header.startswith(_BEARER_TOKEN_PREFIX):
            return auth_header[_BEARER_TOKEN_PREFIX_LEN:]
        return

    # Then try to read auth token from query.
    parameters = urlparse.parse_qs(environ.get("QUERY_STRING", ""))
    if _ACCESS_TOKEN_PARAM_NAME in parameters:
        auth_token, = parameters[_ACCESS_TOKEN_PARAM_NAME]
        return auth_token
