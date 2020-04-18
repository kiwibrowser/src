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

from __future__ import absolute_import

from apitools.base.py import encoding
import mock
import os
import tempfile
import unittest2
from expects import be_false, be_none, be_true, expect, equal, raise_error

from google.api.auth import suppliers
from google.api.auth import tokens
from google.api.control import client, messages, report_request, service, wsgi


def _dummy_start_response(content, dummy_response_headers):
    pass


_DUMMY_RESPONSE = ('All must answer "here!"',)


class _DummyWsgiApp(object):

    def __call__(self, environ, dummy_start_response):
        return _DUMMY_RESPONSE


class TestEnvironmentMiddleware(unittest2.TestCase):

    def test_should_add_service_et_al_to_environment(self):
        cls = wsgi.EnvironmentMiddleware
        wrappee = _DummyWsgiApp()
        wanted_service = service.Loaders.SIMPLE.load()
        wrapped = cls(wrappee, wanted_service)

        given = {
            'wsgi.url_scheme': 'http',
            'HTTP_HOST': 'localhost',
            'REQUEST_METHOD': 'GET'
        }
        wrapped(given, _dummy_start_response)
        expect(given.get(cls.SERVICE)).to(equal(wanted_service))
        expect(given.get(cls.SERVICE_NAME)).to(equal(wanted_service.name))
        expect(given.get(cls.METHOD_REGISTRY)).not_to(be_none)
        expect(given.get(cls.REPORTING_RULES)).not_to(be_none)
        expect(given.get(cls.METHOD_INFO)).not_to(be_none)


class TestMiddleware(unittest2.TestCase):
    PROJECT_ID = 'middleware'

    def test_should_not_send_requests_if_there_is_no_service(self):
        wrappee = _DummyWsgiApp()
        control_client = mock.MagicMock(spec=client.Client)

        given = {
            'wsgi.url_scheme': 'http',
            'PATH_INFO': '/any/method',
            'REMOTE_ADDR': '192.168.0.3',
            'HTTP_HOST': 'localhost',
            'HTTP_REFERER': 'example.myreferer.com',
            'REQUEST_METHOD': 'GET'
        }
        dummy_response = messages.CheckResponse(operationId='fake_operation_id')
        wrapped = wsgi.Middleware(wrappee, self.PROJECT_ID, control_client)
        wrapped(given, _dummy_start_response)
        expect(control_client.check.called).to(be_false)
        expect(control_client.report.called).to(be_false)

    def test_should_send_requests_using_the_client(self):
        wrappee = _DummyWsgiApp()
        control_client = mock.MagicMock(spec=client.Client)

        given = {
            'wsgi.url_scheme': 'http',
            'PATH_INFO': '/any/method',
            'REMOTE_ADDR': '192.168.0.3',
            'HTTP_HOST': 'localhost',
            'HTTP_REFERER': 'example.myreferer.com',
            'REQUEST_METHOD': 'GET'
        }
        dummy_response = messages.CheckResponse(operationId='fake_operation_id')
        with_control = wsgi.Middleware(wrappee, self.PROJECT_ID, control_client)
        wrapped = wsgi.EnvironmentMiddleware(with_control,
                                             service.Loaders.SIMPLE.load())
        control_client.check.return_value = dummy_response
        wrapped(given, _dummy_start_response)
        expect(control_client.check.called).to(be_true)
        expect(control_client.report.called).to(be_true)

    def test_should_send_report_request_if_check_fails(self):
        wrappee = _DummyWsgiApp()
        control_client = mock.MagicMock(spec=client.Client)
        given = {
            'wsgi.url_scheme': 'http',
            'PATH_INFO': '/any/method',
            'REMOTE_ADDR': '192.168.0.3',
            'HTTP_HOST': 'localhost',
            'HTTP_REFERER': 'example.myreferer.com',
            'REQUEST_METHOD': 'GET'
        }
        dummy_response = messages.CheckResponse(
            operationId = 'fake_operation_id',
            checkErrors = [
                messages.CheckError(
                    code=messages.CheckError.CodeValueValuesEnum.PROJECT_DELETED)
            ]
        )
        wrapped = wsgi.add_all(wrappee,
                               self.PROJECT_ID,
                               control_client,
                               loader=service.Loaders.SIMPLE)
        control_client.check.return_value = dummy_response
        wrapped(given, _dummy_start_response)
        expect(control_client.check.called).to(be_true)
        expect(control_client.report.called).to(be_true)

    def test_load_service_failed(self):
        loader = mock.MagicMock(load=lambda: None)
        with self.assertRaisesRegex(ValueError, "Failed to load service config"):
            wsgi.add_all(_DummyWsgiApp(),
                         self.PROJECT_ID,
                         mock.MagicMock(spec=client.Client),
                         loader=loader)


_SYSTEM_PARAMETER_CONFIG_TEST = """
{
    "name": "system-parameter-config",
    "systemParameters": {
       "rules": [{
         "selector": "Uvw.Method1",
         "parameters": [{
            "name": "name1",
            "httpHeader": "Header-Key1",
            "urlQueryParameter": "param_key1"
         }, {
            "name": "name2",
            "httpHeader": "Header-Key2",
            "urlQueryParameter": "param_key2"
         }, {
            "name": "api_key",
            "httpHeader": "ApiKeyHeader",
            "urlQueryParameter": "ApiKeyParam"
         }, {
            "httpHeader": "Ignored-NoName-Key3",
            "urlQueryParameter": "Ignored-NoName-key3"
         }]
       }, {
         "selector": "Bad.NotConfigured",
         "parameters": [{
            "name": "neverUsed",
            "httpHeader": "NeverUsed-Key1",
            "urlQueryParameter": "NeverUsed_key1"
         }]
       }]
    },
    "http": {
        "rules": [{
            "selector": "Uvw.Method1",
            "get": "/uvw/method1/*"
        }, {
            "selector": "Uvw.MethodNeedsApiKey",
            "get": "/uvw/method_needs_api_key/*"
        }, {
            "selector": "Uvw.DefaultParameters",
            "get": "/uvw/default_parameters"
        }]
    },
    "usage": {
        "rules": [{
            "selector" : "Uvw.Method1",
            "allowUnregisteredCalls" : true
        },  {
            "selector": "Uvw.MethodNeedsApiKey",
            "allowUnregisteredCalls" : false
        }, {
            "selector" : "Uvw.DefaultParameters",
            "allowUnregisteredCalls" : true
        }]
    }
}
"""

class TestMiddlewareWithParams(unittest2.TestCase):
    PROJECT_ID = 'middleware-with-params'

    def setUp(self):
        _config_fd = tempfile.NamedTemporaryFile(delete=False)
        with _config_fd as f:
            f.write(_SYSTEM_PARAMETER_CONFIG_TEST)
        self._config_file = _config_fd.name
        os.environ[service.CONFIG_VAR] = self._config_file

    def tearDown(self):
        if os.path.exists(self._config_file):
            os.remove(self._config_file)

    def test_should_send_requests_with_no_param(self):
        wrappee = _DummyWsgiApp()
        control_client = mock.MagicMock(spec=client.Client)
        given = {
            'wsgi.url_scheme': 'http',
            'PATH_INFO': '/uvw/method1/with_no_param',
            'REMOTE_ADDR': '192.168.0.3',
            'HTTP_HOST': 'localhost',
            'HTTP_REFERER': 'example.myreferer.com',
            'REQUEST_METHOD': 'GET'
        }
        dummy_response = messages.CheckResponse(operationId='fake_operation_id')
        wrapped = wsgi.add_all(wrappee,
                               self.PROJECT_ID,
                               control_client,
                               loader=service.Loaders.ENVIRONMENT)
        control_client.check.return_value = dummy_response
        wrapped(given, _dummy_start_response)
        expect(control_client.check.called).to(be_true)
        req = control_client.check.call_args[0][0]
        expect(req.checkRequest.operation.consumerId).to(
            equal('project:middleware-with-params'))
        expect(control_client.report.called).to(be_true)

    def test_should_send_requests_with_configured_query_param_api_key(self):
        wrappee = _DummyWsgiApp()
        control_client = mock.MagicMock(spec=client.Client)
        given = {
            'wsgi.url_scheme': 'http',
            'QUERY_STRING': 'ApiKeyParam=my-query-value',
            'PATH_INFO': '/uvw/method1/with_query_param',
            'REMOTE_ADDR': '192.168.0.3',
            'HTTP_HOST': 'localhost',
            'HTTP_REFERER': 'example.myreferer.com',
            'REQUEST_METHOD': 'GET'
        }
        dummy_response = messages.CheckResponse(operationId='fake_operation_id')
        wrapped = wsgi.add_all(wrappee,
                               self.PROJECT_ID,
                               control_client,
                               loader=service.Loaders.ENVIRONMENT)
        control_client.check.return_value = dummy_response
        wrapped(given, _dummy_start_response)
        expect(control_client.check.called).to(be_true)
        check_req = control_client.check.call_args[0][0]
        expect(check_req.checkRequest.operation.consumerId).to(
            equal('api_key:my-query-value'))
        expect(control_client.report.called).to(be_true)
        report_req = control_client.report.call_args[0][0]
        expect(report_req.reportRequest.operations[0].consumerId).to(
            equal('api_key:my-query-value'))

    def test_should_send_requests_with_configured_header_api_key(self):
        wrappee = _DummyWsgiApp()
        control_client = mock.MagicMock(spec=client.Client)
        given = {
            'wsgi.url_scheme': 'http',
            'PATH_INFO': '/uvw/method1/with_query_param',
            'REMOTE_ADDR': '192.168.0.3',
            'HTTP_HOST': 'localhost',
            'HTTP_APIKEYHEADER': 'my-header-value',
            'HTTP_REFERER': 'example.myreferer.com',
            'REQUEST_METHOD': 'GET'
        }
        dummy_response = messages.CheckResponse(operationId='fake_operation_id')
        wrapped = wsgi.add_all(wrappee,
                               self.PROJECT_ID,
                               control_client,
                               loader=service.Loaders.ENVIRONMENT)
        control_client.check.return_value = dummy_response
        wrapped(given, _dummy_start_response)
        expect(control_client.check.called).to(be_true)
        check_request = control_client.check.call_args_list[0].checkRequest
        check_req = control_client.check.call_args[0][0]
        expect(check_req.checkRequest.operation.consumerId).to(
            equal('api_key:my-header-value'))
        expect(control_client.report.called).to(be_true)
        report_req = control_client.report.call_args[0][0]
        expect(report_req.reportRequest.operations[0].consumerId).to(
            equal('api_key:my-header-value'))

    def test_should_send_requests_with_default_query_param_api_key(self):
        for default_key in ('key', 'api_key'):
            wrappee = _DummyWsgiApp()
            control_client = mock.MagicMock(spec=client.Client)
            given = {
                'wsgi.url_scheme': 'http',
                'QUERY_STRING': '%s=my-default-api-key-value' % (default_key,),
                'PATH_INFO': '/uvw/method_needs_api_key/with_query_param',
                'REMOTE_ADDR': '192.168.0.3',
                'HTTP_HOST': 'localhost',
                'HTTP_REFERER': 'example.myreferer.com',
                'REQUEST_METHOD': 'GET'
            }
            dummy_response = messages.CheckResponse(operationId='fake_operation_id')
            wrapped = wsgi.add_all(wrappee,
                                   self.PROJECT_ID,
                                   control_client,
                                   loader=service.Loaders.ENVIRONMENT)
            control_client.check.return_value = dummy_response
            wrapped(given, _dummy_start_response)
            expect(control_client.check.called).to(be_true)
            check_request = control_client.check.call_args_list[0].checkRequest
            check_req = control_client.check.call_args[0][0]
            expect(check_req.checkRequest.operation.consumerId).to(
                equal('api_key:my-default-api-key-value'))
            expect(control_client.report.called).to(be_true)
            report_req = control_client.report.call_args[0][0]
            expect(report_req.reportRequest.operations[0].consumerId).to(
                equal('api_key:my-default-api-key-value'))

    def test_should_not_perform_check_if_needed_api_key_is_missing(self):
        wrappee = _DummyWsgiApp()
        control_client = mock.MagicMock(spec=client.Client)
        given = {
            'wsgi.url_scheme': 'http',
            'PATH_INFO': '/uvw/method_needs_api_key/more_stuff',
            'REMOTE_ADDR': '192.168.0.3',
            'HTTP_HOST': 'localhost',
            'HTTP_REFERER': 'example.myreferer.com',
            'REQUEST_METHOD': 'GET'
        }
        dummy_response = messages.CheckResponse(operationId='fake_operation_id')
        wrapped = wsgi.add_all(wrappee,
                               self.PROJECT_ID,
                               control_client,
                               loader=service.Loaders.ENVIRONMENT)
        control_client.check.return_value = dummy_response
        wrapped(given, _dummy_start_response)
        expect(control_client.check.called).to(be_false)
        expect(control_client.report.called).to(be_true)
        report_req = control_client.report.call_args[0][0]
        expect(report_req.reportRequest.operations[0].consumerId).to(
            equal('project:middleware-with-params'))

AuthMiddleware = wsgi.AuthenticationMiddleware


class TestAuthenticationMiddleware(unittest2.TestCase):

  def setUp(self):
      self._mock_application = _DummyWsgiApp()
      self._mock_authenticator = mock.MagicMock(spec=tokens.Authenticator)
      self._middleware = AuthMiddleware(self._mock_application,
                                        self._mock_authenticator)

  def test_no_authentication(self):
      with self.assertRaisesRegex(ValueError, "Invalid authenticator"):
          AuthMiddleware(self._mock_application, None)

  def test_no_method_info(self):
      environ = {}
      self.assertEqual(_DUMMY_RESPONSE,
                       self._middleware(environ, _dummy_start_response))

  def test_no_auth_token(self):
      auth_app = AuthMiddleware(self.UserInfoWsgiApp(), self._mock_authenticator)
      method_info = mock.MagicMock()
      method_info.auth_info = mock.MagicMock()
      environ = {
          wsgi.EnvironmentMiddleware.METHOD_INFO: method_info
      }
      self.assertIsNone(auth_app(environ, _dummy_start_response))

  def test_malformed_authorization_header(self):
      auth_app = AuthMiddleware(self.UserInfoWsgiApp(), self._mock_authenticator)
      environ = {
          "HTTP_AUTHORIZATION": "malformed-auth-token",
          wsgi.EnvironmentMiddleware.METHOD_INFO: mock.MagicMock(),
          wsgi.EnvironmentMiddleware.SERVICE_NAME: "service-name"
      }
      self._mock_authenticator.authenticate.side_effect = suppliers.UnauthenticatedException()
      self.assertIsNone(auth_app(environ, _dummy_start_response))

  def test_successful_authentication(self):
      auth_token = "Bearer test-bearer-token"
      auth_info = mock.MagicMock()
      service_name = "test-service-name"
      method_info = mock.MagicMock()
      method_info.auth_info = auth_info
      environ = {
          "HTTP_AUTHORIZATION": auth_token,
          wsgi.EnvironmentMiddleware.METHOD_INFO: method_info,
          wsgi.EnvironmentMiddleware.SERVICE_NAME: service_name
      }

      user_info = mock.MagicMock()
      self._mock_authenticator.authenticate.return_value = user_info
      self._middleware(environ, _dummy_start_response)
      self.assertEqual(user_info, environ.get(AuthMiddleware.USER_INFO))
      authenticate_mock = self._mock_authenticator.authenticate
      authenticate_mock.assert_called_once_with("test-bearer-token", auth_info,
                                                service_name)

  def test_auth_token_in_query(self):
      auth_token = "test-bearer-token"
      auth_info = mock.MagicMock()
      service_name = "test-service-name"
      method_info = mock.MagicMock()
      method_info.auth_info = auth_info
      environ = {
          "QUERY_STRING": "access_token=" + auth_token,
          wsgi.EnvironmentMiddleware.METHOD_INFO: method_info,
          wsgi.EnvironmentMiddleware.SERVICE_NAME: service_name
      }

      user_info = mock.MagicMock()
      self._mock_authenticator.authenticate.return_value = user_info
      self._middleware(environ, _dummy_start_response)
      self.assertEqual(user_info, environ.get(AuthMiddleware.USER_INFO))
      authenticate_mock = self._mock_authenticator.authenticate
      authenticate_mock.assert_called_once_with("test-bearer-token", auth_info,
                                                service_name)

  patched_environ = {}
  @mock.patch("os.environ", patched_environ)
  def test_set_user_info(self):
    environ = {
        "QUERY_STRING": "access_token=test-token",
        wsgi.EnvironmentMiddleware.METHOD_INFO: mock.MagicMock(),
        wsgi.EnvironmentMiddleware.SERVICE_NAME: "test-service-name"
    }
    application = self.UserInfoWsgiApp()
    auth_middleware = AuthMiddleware(application, self._mock_authenticator)
    user_info = mock.MagicMock()
    self._mock_authenticator.authenticate.return_value = user_info
    self.assertEqual(user_info, auth_middleware(environ,
                                                _dummy_start_response))
    self.assertFalse(self.patched_environ)

  class UserInfoWsgiApp(object):
    def __call__(self, environ, start_response):
      return os.environ.get(wsgi.AuthenticationMiddleware.USER_INFO)


class TestCreateAuthenticator(unittest2.TestCase):
    def test_create_without_service(self):
        with self.assertRaises(ValueError):
            wsgi._create_authenticator(None)

    def test_load_service_without_auth(self):
        service = _read_service_from_json("{}")
        self.assertIsNone(wsgi._create_authenticator(service))

    def test_load_service(self):
        json = """{
            "authentication": {
                "providers": [{
                    "issuer": "auth-issuer"
                }]
            }
        }"""
        service = _read_service_from_json(json)
        self.assertIsNotNone(wsgi._create_authenticator(service))


patched_platform_environ = {}
@mock.patch.dict('os.environ', patched_platform_environ, clear=True)
class TestPlatformDetection(unittest2.TestCase):

  def test_development(self):
    os.environ['SERVER_SOFTWARE'] = 'Development/2.0.0'
    self.assertEqual(report_request.ReportedPlatforms.DEVELOPMENT,
                     wsgi._get_platform())

  def test_gke(self):
    os.environ['KUBERNETES_SERVICE_HOST'] = 'hostname'
    self.assertEqual(report_request.ReportedPlatforms.GKE,
                     wsgi._get_platform())

  @mock.patch.object(wsgi, '_running_on_gce', return_value=True)
  def test_gae_flex(self, _running_on_gce):
    os.environ['GAE_MODULE_NAME'] = 'gae_module'
    self.assertEqual(report_request.ReportedPlatforms.GAE_FLEX,
                     wsgi._get_platform())

  @mock.patch.object(wsgi, '_running_on_gce', return_value=True)
  def test_gce(self, _running_on_gce):
    self.assertEqual(report_request.ReportedPlatforms.GCE,
                     wsgi._get_platform())

  @mock.patch.object(wsgi, '_running_on_gce', return_value=False)
  def test_gae_standard(self, _running_on_gce):
    os.environ['GAE_MODULE_NAME'] = 'gae_module'
    self.assertEqual(report_request.ReportedPlatforms.GAE_STANDARD,
                     wsgi._get_platform())

  @mock.patch.object(wsgi, '_running_on_gce', return_value=False)
  def test_unknown(self, _running_on_gce):
    self.assertEqual(report_request.ReportedPlatforms.UNKNOWN,
                     wsgi._get_platform())


def _read_service_from_json(json):
    return encoding.JsonToMessage(messages.Service, json)
