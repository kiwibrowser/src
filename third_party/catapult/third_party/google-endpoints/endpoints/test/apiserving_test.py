#!/usr/bin/python2.4
#
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

"""Tests for Apiserving library.

Tests:
  ModuleInterfaceTest: Tests interface of module.
  ApiServerTest: Tests core functionality: registry, requests, errors.
"""

import httplib
import json
import logging
import unittest
import urllib2

import endpoints.api_backend_service as api_backend_service
import endpoints.api_config as api_config
import endpoints.api_exceptions as api_exceptions
import endpoints.apiserving as apiserving
import mox
from protorpc import message_types
from protorpc import messages
from protorpc import protojson
from protorpc import registry
from protorpc import remote
from protorpc import transport

import endpoints.resource_container as resource_container
import test_util

import webtest

package = 'endpoints.test'


class ExceptionRequest(messages.Message):
  exception_type = messages.StringField(1)
  message = messages.StringField(2)


class Custom405Exception(api_exceptions.ServiceException):
  http_status = httplib.METHOD_NOT_ALLOWED


class Custom408Exception(api_exceptions.ServiceException):
  http_status = httplib.REQUEST_TIMEOUT


@api_config.api('aservice', 'v2', hostname='aservice.appspot.com',
                description='A Service API')
class AService(remote.Service):

  @api_config.method(name='noopname', path='nooppath')
  def Noop(self, unused_request):
    return message_types.VoidMessage()

  @api_config.method(path='type_error')
  def RaiseTypeError(self, unused_request):
    raise TypeError('a type error')

  @api_config.method(path='app_error')
  def RaiseNormalApplicationError(self, unused_request):
    raise remote.ApplicationError('whatever')

  # Silence warning about not deriving from Exception
  # pylint: disable=nonstandard-exception
  @api_config.method(ExceptionRequest, path='exception')
  def RaiseException(self, request):
    exception_class = (getattr(api_exceptions, request.exception_type, None) or
                       globals().get(request.exception_type))
    raise exception_class(request.message)


@api_config.api('bservice', 'v3', hostname='bservice.appspot.com',
                description='Another Service API')
class BService(remote.Service):

  @api_config.method(path='noop')
  def Noop(self, unused_request):
    return message_types.VoidMessage()


test_request = resource_container.ResourceContainer(
    message_types.VoidMessage,
    id=messages.IntegerField(1, required=True))


@api_config.api(name='testapi', version='v3', description='A wonderful API.')
class TestService(remote.Service):

  @api_config.method(test_request,
                     message_types.VoidMessage,
                     http_method='DELETE', path='items/{id}')
  # Silence lint warning about method naming conventions
  # pylint: disable=g-bad-name
  def delete(self, unused_request):
    return message_types.VoidMessage()


@api_config.api(name='testapicustomurl', version='v3',
                description='A wonderful API.', base_path='/my/base/path/')
class TestServiceCustomUrl(remote.Service):

  @api_config.method(test_request,
                     message_types.VoidMessage,
                     http_method='DELETE', path='items/{id}')
  # Silence lint warning about method naming conventions
  # pylint: disable=g-bad-name
  def delete(self, unused_request):
    return message_types.VoidMessage()


my_api = api_config.api(name='My Service', version='v1')


@my_api.api_class()
class MultiClassService1(remote.Service):

  @api_config.method(path='s1')
  def S1method(self, unused_request):
    return message_types.VoidMessage()


@my_api.api_class()
class MultiClassService2(remote.Service):

  @api_config.method(path='s2')
  def S2method(self, unused_request):
    return message_types.VoidMessage()


TEST_SERVICE_API_CONFIG = {'items': [{
    'abstract': False,
    'adapter': {
        'bns': 'https://None/_ah/api',
        'type': 'lily',
        'deadline': 10.0
    },
    'defaultVersion': True,
    'description': 'A wonderful API.',
    'descriptor': {
        'methods': {
            'TestService.delete': {}
        },
        'schemas': {
            'ProtorpcMessageTypesVoidMessage': {
                'description': 'Empty message.',
                'id': 'ProtorpcMessageTypesVoidMessage',
                'properties': {},
                'type': 'object'
            }
        }
    },
    'extends': 'thirdParty.api',
    'methods': {
        'testapi.delete': {
            'httpMethod': 'DELETE',
            'path': 'items/{id}',
            'request': {
                'body': 'empty',
                'parameterOrder': ['id'],
                'parameters': {
                    'id': {
                        'required': True,
                        'type': 'int64',
                    }
                }
            },
            'response': {
                'body': 'empty'
            },
            'rosyMethod': 'TestService.delete',
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'clientIds': ['292824132082.apps.googleusercontent.com'],
            'authLevel': 'NONE'
        }
    },
    'name': 'testapi',
    'root': 'https://None/_ah/api',
    'version': 'v3'
}]}


TEST_SERVICE_CUSTOM_URL_API_CONFIG = {'items': [{
    'abstract': False,
    'adapter': {
        'bns': 'https://None/my/base/path',
        'type': 'lily',
        'deadline': 10.0
    },
    'defaultVersion': True,
    'description': 'A wonderful API.',
    'descriptor': {
        'methods': {
            'TestServiceCustomUrl.delete': {}
        },
        'schemas': {
            'ProtorpcMessageTypesVoidMessage': {
                'description': 'Empty message.',
                'id': 'ProtorpcMessageTypesVoidMessage',
                'properties': {},
                'type': 'object'
            }
        }
    },
    'extends': 'thirdParty.api',
    'methods': {
        'testapicustomurl.delete': {
            'httpMethod': 'DELETE',
            'path': 'items/{id}',
            'request': {
                'body': 'empty',
                'parameterOrder': ['id'],
                'parameters': {
                    'id': {
                        'required': True,
                        'type': 'int64',
                    }
                }
            },
            'response': {
                'body': 'empty'
            },
            'rosyMethod': 'TestServiceCustomUrl.delete',
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'clientIds': ['292824132082.apps.googleusercontent.com'],
            'authLevel': 'NONE'
        }
    },
    'name': 'testapicustomurl',
    'root': 'https://None/my/base/path',
    'version': 'v3'
}]}


class ModuleInterfaceTest(test_util.ModuleInterfaceTest,
                          unittest.TestCase):

  MODULE = apiserving


class ApiServerBaseTest(unittest.TestCase):

  def setUp(self):
    super(ApiServerBaseTest, self).setUp()


class ApiServerTestApiConfigRegistryEndToEnd(unittest.TestCase):
  # Show diff with expected API config
  maxDiff = None

  def setUp(self):
    super(ApiServerTestApiConfigRegistryEndToEnd, self).setUp()

  def testGetApiConfigs(self):
    my_app = apiserving.api_server([TestService])

    # 200 with X-Appengine-Peer: apiserving header
    configs = my_app.get_api_configs()
    self.assertEqual(TEST_SERVICE_API_CONFIG, configs)


class GetAppRevisionTest(unittest.TestCase):
  def testGetAppRevision(self):
    environ = {'CURRENT_VERSION_ID': '1.1'}
    self.assertEqual('1', apiserving._get_app_revision(environ=environ))

  def testGetAppRevisionWithNoEntry(self):
    environ = {}
    self.assertEqual(None, apiserving._get_app_revision(environ=environ))


class ApiServerTestApiConfigRegistryEndToEndCustomUrl(unittest.TestCase):
  # Show diff with expected API config
  maxDiff = None

  def setUp(self):
    super(ApiServerTestApiConfigRegistryEndToEndCustomUrl, self).setUp()

  def testGetApiConfigs(self):
    my_app = apiserving.api_server([TestServiceCustomUrl])

    # 200 with X-Appengine-Peer: apiserving header
    configs = my_app.get_api_configs()
    self.assertEqual(TEST_SERVICE_CUSTOM_URL_API_CONFIG, configs)


if __name__ == '__main__':
  unittest.main()
