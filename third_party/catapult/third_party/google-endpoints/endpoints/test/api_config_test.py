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

"""Tests for endpoints.api_config."""

import itertools
import json
import logging
import unittest

import endpoints.api_config as api_config
from endpoints.api_config import ApiConfigGenerator
from endpoints.api_config import AUTH_LEVEL
import endpoints.api_exceptions as api_exceptions
import mock
from protorpc import message_types
from protorpc import messages
from protorpc import remote

import endpoints.resource_container as resource_container

import test_util

package = 'api_config_test'
_DESCRIPTOR_PATH_PREFIX = ''


class ModuleInterfaceTest(test_util.ModuleInterfaceTest,
                          unittest.TestCase):

  MODULE = api_config


class Nested(messages.Message):
  """Message class to be used in a message field."""
  int_value = messages.IntegerField(1)
  string_value = messages.StringField(2)


class SimpleEnum(messages.Enum):
  """Simple enumeration type."""
  VAL1 = 1
  VAL2 = 2


class AllFields(messages.Message):
  """Contains all field types."""

  bool_value = messages.BooleanField(1, variant=messages.Variant.BOOL)
  bytes_value = messages.BytesField(2, variant=messages.Variant.BYTES)
  double_value = messages.FloatField(3, variant=messages.Variant.DOUBLE)
  enum_value = messages.EnumField(SimpleEnum, 4)
  float_value = messages.FloatField(5, variant=messages.Variant.FLOAT)
  int32_value = messages.IntegerField(6, variant=messages.Variant.INT32)
  int64_value = messages.IntegerField(7, variant=messages.Variant.INT64)
  string_value = messages.StringField(8, variant=messages.Variant.STRING)
  uint32_value = messages.IntegerField(9, variant=messages.Variant.UINT32)
  uint64_value = messages.IntegerField(10, variant=messages.Variant.UINT64)
  sint32_value = messages.IntegerField(11, variant=messages.Variant.SINT32)
  sint64_value = messages.IntegerField(12, variant=messages.Variant.SINT64)
  message_field_value = messages.MessageField(Nested, 13)
  datetime_value = message_types.DateTimeField(14)


# This is used test "all fields" as query parameters instead of the body
# in a request.
ALL_FIELDS_AS_PARAMETERS = resource_container.ResourceContainer(
    **{field.name: field for field in AllFields.all_fields()})


class ApiConfigTest(unittest.TestCase):

  def setUp(self):
    self.generator = ApiConfigGenerator()
    self.maxDiff = None

  def testAllVariantsCovered(self):
    variants_covered = set([field.variant for field in AllFields.all_fields()])

    for variant in variants_covered:
      self.assertTrue(isinstance(variant, messages.Variant))

    variants_covered_dict = {}
    for variant in variants_covered:
      number = variant.number
      if variants_covered_dict.get(variant.name, number) != number:
        self.fail('Somehow have two variants with same name and '
                  'different number')
      variants_covered_dict[variant.name] = number

    test_util.AssertDictEqual(
        messages.Variant.to_dict(), variants_covered_dict, self)

  def testAllFieldTypes(self):

    class PutRequest(messages.Message):
      """Message with just a body field."""
      body = messages.MessageField(AllFields, 1)

    class ItemsPutRequest(messages.Message):
      """Message with path params and a body field."""
      body = messages.MessageField(AllFields, 1)
      entryId = messages.StringField(2, required=True)

    class ItemsPutRequestForContainer(messages.Message):
      """Message with path params and a body field."""
      body = messages.MessageField(AllFields, 1)
    items_put_request_container = resource_container.ResourceContainer(
        ItemsPutRequestForContainer,
        entryId=messages.StringField(2, required=True))

    class EntryPublishRequest(messages.Message):
      """Message with two required params, one in path, one in body."""
      title = messages.StringField(1, required=True)
      entryId = messages.StringField(2, required=True)

    class EntryPublishRequestForContainer(messages.Message):
      """Message with two required params, one in path, one in body."""
      title = messages.StringField(1, required=True)
    entry_publish_request_container = resource_container.ResourceContainer(
        EntryPublishRequestForContainer,
        entryId=messages.StringField(2, required=True))

    @api_config.api(name='root', hostname='example.appspot.com', version='v1')
    class MyService(remote.Service):
      """Describes MyService."""

      @api_config.method(AllFields, message_types.VoidMessage, path='entries',
                         http_method='GET', name='entries.get')
      def entries_get(self, unused_request):
        """All field types in the query parameters."""
        return message_types.VoidMessage()

      @api_config.method(ALL_FIELDS_AS_PARAMETERS, message_types.VoidMessage,
                         path='entries/container', http_method='GET',
                         name='entries.getContainer')
      def entries_get_container(self, unused_request):
        """All field types in the query parameters."""
        return message_types.VoidMessage()

      @api_config.method(PutRequest, message_types.VoidMessage, path='entries',
                         name='entries.put')
      def entries_put(self, unused_request):
        """Request body is in the body field."""
        return message_types.VoidMessage()

      @api_config.method(AllFields, message_types.VoidMessage, path='process',
                         name='entries.process')
      def entries_process(self, unused_request):
        """Message is the request body."""
        return message_types.VoidMessage()

      @api_config.method(message_types.VoidMessage, message_types.VoidMessage,
                         name='entries.nested.collection.action',
                         path='nested')
      def entries_nested_collection_action(self, unused_request):
        """A VoidMessage for a request body."""
        return message_types.VoidMessage()

      @api_config.method(AllFields, AllFields, name='entries.roundtrip',
                         path='roundtrip')
      def entries_roundtrip(self, unused_request):
        """All field types in the request and response."""
        pass

      # Test a method with a required parameter in the request body.
      @api_config.method(EntryPublishRequest, message_types.VoidMessage,
                         path='entries/{entryId}/publish',
                         name='entries.publish')
      def entries_publish(self, unused_request):
        """Path has a parameter and request body has a required param."""
        return message_types.VoidMessage()

      @api_config.method(entry_publish_request_container,
                         message_types.VoidMessage,
                         path='entries/container/{entryId}/publish',
                         name='entries.publishContainer')
      def entries_publish_container(self, unused_request):
        """Path has a parameter and request body has a required param."""
        return message_types.VoidMessage()

      # Test a method with a parameter in the path and a request body.
      @api_config.method(ItemsPutRequest, message_types.VoidMessage,
                         path='entries/{entryId}/items',
                         name='entries.items.put')
      def items_put(self, unused_request):
        """Path has a parameter and request body is in the body field."""
        return message_types.VoidMessage()

      @api_config.method(items_put_request_container, message_types.VoidMessage,
                         path='entries/container/{entryId}/items',
                         name='entries.items.putContainer')
      def items_put_container(self, unused_request):
        """Path has a parameter and request body is in the body field."""
        return message_types.VoidMessage()

    api = json.loads(self.generator.pretty_print_config_to_json(MyService))

    expected = {
        'root.entries.get': {
            'description': 'All field types in the query parameters.',
            'httpMethod': 'GET',
            'path': 'entries',
            'request': {
                'body': 'empty',
                'parameters': {
                    'bool_value': {
                        'type': 'boolean',
                        },
                    'bytes_value': {
                        'type': 'bytes',
                        },
                    'double_value': {
                        'type': 'double',
                        },
                    'enum_value': {
                        'type': 'string',
                        'enum': {
                            'VAL1': {
                                'backendValue': 'VAL1',
                                },
                            'VAL2': {
                                'backendValue': 'VAL2',
                                },
                            },
                        },
                    'float_value': {
                        'type': 'float',
                        },
                    'int32_value': {
                        'type': 'int32',
                        },
                    'int64_value': {
                        'type': 'int64',
                        },
                    'string_value': {
                        'type': 'string',
                        },
                    'uint32_value': {
                        'type': 'uint32',
                        },
                    'uint64_value': {
                        'type': 'uint64',
                        },
                    'sint32_value': {
                        'type': 'int32',
                        },
                    'sint64_value': {
                        'type': 'int64',
                        },
                    'message_field_value.int_value': {
                        'type': 'int64',
                        },
                    'message_field_value.string_value': {
                        'type': 'string',
                        },
                    'datetime_value.milliseconds': {
                        'type': 'int64',
                        },
                    'datetime_value.time_zone_offset': {
                        'type': 'int64',
                        },
                    },
                },
            'response': {
                'body': 'empty',
                },
            'rosyMethod': 'MyService.entries_get',
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'clientIds': [api_config.API_EXPLORER_CLIENT_ID],
            'authLevel': 'NONE',
            },
        'root.entries.getContainer': {
            'description': 'All field types in the query parameters.',
            'httpMethod': 'GET',
            'path': 'entries/container',
            'request': {
                'body': 'empty',
                'parameters': {
                    'bool_value': {
                        'type': 'boolean'
                        },
                    'bytes_value': {
                        'type': 'bytes'
                        },
                    'datetime_value.milliseconds': {
                        'type': 'int64'
                        },
                    'datetime_value.time_zone_offset': {
                        'type': 'int64'
                        },
                    'double_value': {
                        'type': 'double'
                        },
                    'enum_value': {
                        'enum': {
                            'VAL1': {'backendValue': 'VAL1'},
                            'VAL2': {'backendValue': 'VAL2'},
                            },
                        'type': 'string',
                        },
                    'float_value': {
                        'type': 'float'
                        },
                    'int32_value': {
                        'type': 'int32'
                        },
                    'int64_value': {
                        'type': 'int64'
                        },
                    'message_field_value.int_value': {
                        'type': 'int64'
                        },
                    'message_field_value.string_value': {
                        'type': 'string'
                        },
                    'sint32_value': {
                        'type': 'int32'
                        },
                    'sint64_value': {
                        'type': 'int64'
                        },
                    'string_value': {
                        'type': 'string'
                        },
                    'uint32_value': {
                        'type': 'uint32'
                        },
                    'uint64_value': {
                        'type': 'uint64'
                        }
                    }
                },
            'response': {
                'body': 'empty'
                },
            'rosyMethod': 'MyService.entries_get_container',
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'clientIds': [api_config.API_EXPLORER_CLIENT_ID],
            'authLevel': 'NONE',
        },
        'root.entries.publishContainer': {
            'description': ('Path has a parameter and request body has a '
                            'required param.'),
            'httpMethod': 'POST',
            'path': 'entries/container/{entryId}/publish',
            'request': {
                'body': 'autoTemplate(backendRequest)',
                'bodyName': 'resource',
                'parameterOrder': ['entryId'],
                'parameters': {
                    'entryId': {
                        'required': True,
                        'type': 'string',
                        }
                    }
                },
            'response': {
                'body': 'empty'
                },
            'rosyMethod': 'MyService.entries_publish_container',
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'clientIds': [api_config.API_EXPLORER_CLIENT_ID],
            'authLevel': 'NONE',
            },
        'root.entries.put': {
            'description': 'Request body is in the body field.',
            'httpMethod': 'POST',
            'path': 'entries',
            'request': {
                'body': 'autoTemplate(backendRequest)',
                'bodyName': 'resource'
                },
            'response': {
                'body': 'empty'
                },
            'rosyMethod': 'MyService.entries_put',
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'clientIds': [api_config.API_EXPLORER_CLIENT_ID],
            'authLevel': 'NONE',
            },
        'root.entries.process': {
            'description': 'Message is the request body.',
            'httpMethod': 'POST',
            'path': 'process',
            'request': {
                'body': 'autoTemplate(backendRequest)',
                'bodyName': 'resource'
                },
            'response': {
                'body': 'empty'
                },
            'rosyMethod': 'MyService.entries_process',
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'clientIds': [api_config.API_EXPLORER_CLIENT_ID],
            'authLevel': 'NONE',
            },
        'root.entries.nested.collection.action': {
            'description': 'A VoidMessage for a request body.',
            'httpMethod': 'POST',
            'path': 'nested',
            'request': {
                'body': 'empty'
                },
            'response': {
                'body': 'empty'
                },
            'rosyMethod': 'MyService.entries_nested_collection_action',
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'clientIds': [api_config.API_EXPLORER_CLIENT_ID],
            'authLevel': 'NONE',
            },
        'root.entries.roundtrip': {
            'description': 'All field types in the request and response.',
            'httpMethod': 'POST',
            'path': 'roundtrip',
            'request': {
                'body': 'autoTemplate(backendRequest)',
                'bodyName': 'resource'
                },
            'response': {
                'body': 'autoTemplate(backendResponse)',
                'bodyName': 'resource'
                },
            'rosyMethod': 'MyService.entries_roundtrip',
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'clientIds': [api_config.API_EXPLORER_CLIENT_ID],
            'authLevel': 'NONE',
            },
        'root.entries.publish': {
            'description':
            'Path has a parameter and request body has a required param.',
            'httpMethod': 'POST',
            'path': 'entries/{entryId}/publish',
            'request': {
                'body': 'autoTemplate(backendRequest)',
                'bodyName': 'resource',
                'parameterOrder': [
                    'entryId'
                    ],
                'parameters': {
                    'entryId': {
                        'type': 'string',
                        'required': True,
                        },
                    },
                },
            'response': {
                'body': 'empty'
                 },
            'rosyMethod': 'MyService.entries_publish',
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'clientIds': [api_config.API_EXPLORER_CLIENT_ID],
            'authLevel': 'NONE',
            },
        'root.entries.items.put': {
            'description':
                'Path has a parameter and request body is in the body field.',
            'httpMethod': 'POST',
            'path': 'entries/{entryId}/items',
            'request': {
                'body': 'autoTemplate(backendRequest)',
                'bodyName': 'resource',
                'parameterOrder': [
                    'entryId'
                    ],
                'parameters': {
                    'entryId': {
                        'type': 'string',
                        'required': True,
                        },
                    },
                },
           'response': {
                'body': 'empty'
                 },
            'rosyMethod': 'MyService.items_put',
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'clientIds': [api_config.API_EXPLORER_CLIENT_ID],
            'authLevel': 'NONE',
            },
        'root.entries.items.putContainer': {
            'description': ('Path has a parameter and request body is in '
                            'the body field.'),
            'httpMethod': 'POST',
            'path': 'entries/container/{entryId}/items',
            'request': {
                'body': 'autoTemplate(backendRequest)',
                'bodyName': 'resource',
                'parameterOrder': [
                    'entryId'
                    ],
                'parameters': {
                    'entryId': {
                        'type': 'string',
                        'required': True,
                        },
                    },
                },
            'response': {
                'body': 'empty'
                },
            'rosyMethod': 'MyService.items_put_container',
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'clientIds': [api_config.API_EXPLORER_CLIENT_ID],
            'authLevel': 'NONE',
            }
        }
    expected_descriptor = {
        'methods': {
            'MyService.entries_get': {},
            'MyService.entries_get_container': {},
            'MyService.entries_nested_collection_action': {},
            'MyService.entries_process': {
                'request': {
                    '$ref': (_DESCRIPTOR_PATH_PREFIX +
                             'ApiConfigTestAllFields')
                    }
                },
            'MyService.entries_publish': {
                'request': {
                    '$ref': (_DESCRIPTOR_PATH_PREFIX +
                             'ApiConfigTestEntryPublishRequest')
                    }
                },
            'MyService.entries_publish_container': {
                'request': {
                    '$ref': (_DESCRIPTOR_PATH_PREFIX +
                             'ApiConfigTestEntryPublishRequestForContainer')
                    }
                },
            'MyService.entries_put': {
                'request': {
                    '$ref': (_DESCRIPTOR_PATH_PREFIX +
                             'ApiConfigTestPutRequest')
                    }
                },
            'MyService.entries_roundtrip': {
                'request': {
                    '$ref': (_DESCRIPTOR_PATH_PREFIX +
                             'ApiConfigTestAllFields')
                    },
                'response': {
                    '$ref': (_DESCRIPTOR_PATH_PREFIX +
                             'ApiConfigTestAllFields')
                    }
                },
            'MyService.items_put': {
                'request': {
                    '$ref': (_DESCRIPTOR_PATH_PREFIX +
                             'ApiConfigTestItemsPutRequest')
                    }
                },
            'MyService.items_put_container': {
                'request': {
                    '$ref': (_DESCRIPTOR_PATH_PREFIX +
                             'ApiConfigTestItemsPutRequestForContainer')
                    }
                }
            },
        'schemas': {
            _DESCRIPTOR_PATH_PREFIX + 'ApiConfigTestAllFields': {
                'description': 'Contains all field types.',
                'id': _DESCRIPTOR_PATH_PREFIX + 'ApiConfigTestAllFields',
                'properties': {
                    'bool_value': {
                        'type': 'boolean'
                        },
                    'bytes_value': {
                        'type': 'string',
                        'format': 'byte'
                        },
                    'double_value': {
                        'format': 'double',
                        'type': 'number'
                        },
                    'enum_value': {
                        'type': 'string',
                        'enum': ['VAL1', 'VAL2']
                        },
                    'float_value': {
                        'format': 'float',
                        'type': 'number'
                        },
                    'int32_value': {
                        'format': 'int32',
                        'type': 'integer'
                        },
                    'int64_value': {
                        'format': 'int64',
                        'type': 'string'
                        },
                    'string_value': {
                        'type': 'string'
                        },
                    'uint32_value': {
                        'format': 'uint32',
                        'type': 'integer'
                        },
                    'uint64_value': {
                        'format': 'uint64',
                        'type': 'string'
                        },
                    'sint32_value': {
                        'format': 'int32',
                        'type': 'integer'
                        },
                    'sint64_value': {
                        'format': 'int64',
                        'type': 'string'
                        },
                    'message_field_value': {
                        '$ref': (_DESCRIPTOR_PATH_PREFIX +
                                 'ApiConfigTestNested'),
                        'description': ('Message class to be used in a '
                                        'message field.'),
                        },
                    'datetime_value': {
                        'format': 'date-time',
                        'type': 'string'
                        },
                    },
                'type': 'object'
                },
            _DESCRIPTOR_PATH_PREFIX + 'ApiConfigTestEntryPublishRequest': {
                'description': ('Message with two required params, '
                                'one in path, one in body.'),
                'id': (_DESCRIPTOR_PATH_PREFIX +
                       'ApiConfigTestEntryPublishRequest'),
                'properties': {
                    'entryId': {
                        'required': True,
                        'type': 'string'
                        },
                    'title': {
                        'required': True,
                        'type': 'string'
                        }
                    },
                'type': 'object'
                },
            (_DESCRIPTOR_PATH_PREFIX +
             'ApiConfigTestEntryPublishRequestForContainer'): {
                 'description': ('Message with two required params, '
                                 'one in path, one in body.'),
                 'id': (_DESCRIPTOR_PATH_PREFIX +
                        'ApiConfigTestEntryPublishRequestForContainer'),
                 'properties': {
                     'title': {
                         'required': True,
                         'type': 'string'
                         }
                     },
                 'type': 'object'
                 },
            _DESCRIPTOR_PATH_PREFIX + 'ApiConfigTestItemsPutRequest': {
                'description': 'Message with path params and a body field.',
                'id': (_DESCRIPTOR_PATH_PREFIX +
                       'ApiConfigTestItemsPutRequest'),
                'properties': {
                    'body': {
                        '$ref': (_DESCRIPTOR_PATH_PREFIX +
                                 'ApiConfigTestAllFields'),
                        'description': 'Contains all field types.'
                        },
                    'entryId': {
                        'required': True,
                        'type': 'string'
                        }
                    },
                'type': 'object'
                },
            (_DESCRIPTOR_PATH_PREFIX +
             'ApiConfigTestItemsPutRequestForContainer'): {
                 'description': 'Message with path params and a body field.',
                 'id': (_DESCRIPTOR_PATH_PREFIX +
                        'ApiConfigTestItemsPutRequestForContainer'),
                 'properties': {
                     'body': {
                         '$ref': (_DESCRIPTOR_PATH_PREFIX +
                                  'ApiConfigTestAllFields'),
                         'description': 'Contains all field types.'
                         },
                     },
                 'type': 'object'
                 },
            _DESCRIPTOR_PATH_PREFIX + 'ApiConfigTestNested': {
                'description': 'Message class to be used in a message field.',
                'id': _DESCRIPTOR_PATH_PREFIX + 'ApiConfigTestNested',
                'properties': {
                    'int_value': {
                        'format': 'int64',
                        'type': 'string'
                        },
                    'string_value': {
                        'type': 'string'
                        }
                    },
                'type': 'object'
                },
            _DESCRIPTOR_PATH_PREFIX + 'ApiConfigTestPutRequest': {
                'description': 'Message with just a body field.',
                'id': _DESCRIPTOR_PATH_PREFIX + 'ApiConfigTestPutRequest',
                'properties': {
                    'body': {
                        '$ref': (_DESCRIPTOR_PATH_PREFIX +
                                 'ApiConfigTestAllFields'),
                        'description': 'Contains all field types.'
                        }
                    },
                'type': 'object'
                },
            'ProtorpcMessageTypesVoidMessage': {
                'description': 'Empty message.',
                'id': 'ProtorpcMessageTypesVoidMessage',
                'properties': {},
                'type': 'object'
                }
            }
        }
    expected_adapter = {
        'bns': 'https://example.appspot.com/_ah/api',
        'type': 'lily',
        'deadline': 10.0}

    test_util.AssertDictEqual(expected, api['methods'], self)
    test_util.AssertDictEqual(expected_descriptor, api['descriptor'], self)
    test_util.AssertDictEqual(expected_adapter, api['adapter'], self)

    self.assertEqual('Describes MyService.', api['description'])

    methods = api['descriptor']['methods']
    self.assertTrue('MyService.entries_get' in methods)
    self.assertTrue('MyService.entries_put' in methods)
    self.assertTrue('MyService.entries_process' in methods)
    self.assertTrue('MyService.entries_nested_collection_action' in methods)

  def testEmptyRequestNonEmptyResponse(self):
    class MyResponse(messages.Message):
      bool_value = messages.BooleanField(1)
      int32_value = messages.IntegerField(2)

    @api_config.api(name='root', version='v1', hostname='example.appspot.com')
    class MySimpleService(remote.Service):

      @api_config.method(message_types.VoidMessage, MyResponse,
                         name='entries.get')
      def entries_get(self, request):
        pass

    api = json.loads(self.generator.pretty_print_config_to_json(
        MySimpleService))

    expected_request = {
        'body': 'empty'
        }
    expected_response = {
        'body': 'autoTemplate(backendResponse)',
        'bodyName': 'resource'
        }

    test_util.AssertDictEqual(
        expected_response, api['methods']['root.entries.get']['response'], self)

    test_util.AssertDictEqual(
        expected_request, api['methods']['root.entries.get']['request'], self)

  def testEmptyService(self):

    @api_config.api('root', 'v1', hostname='example.appspot.com')
    class EmptyService(remote.Service):
      pass

    api = json.loads(self.generator.pretty_print_config_to_json(EmptyService))

    self.assertTrue('methods' not in api)

  def testOptionalProperties(self):
    """Verify that optional config properties show up if they're supposed to."""
    optional_props = (
        ('canonical_name', 'canonicalName', 'Test Canonical Name'),
        ('owner_domain', 'ownerDomain', 'google.com'),
        ('owner_name', 'ownerName', 'Google'),
        ('package_path', 'packagePath', 'cloud/platform'),
        ('title', 'title', 'My Root API'),
        ('documentation', 'documentation', 'http://link.to/docs'))

    # Try all combinations of the above properties.
    for length in range(1, len(optional_props) + 1):
      for combination in itertools.combinations(optional_props, length):
        kwargs = {}
        for property_name, _, value in combination:
          kwargs[property_name] = value

        @api_config.api('root', 'v1', **kwargs)
        class MyService(remote.Service):
          pass

        api = json.loads(self.generator.pretty_print_config_to_json(MyService))

        for _, config_name, value in combination:
          self.assertEqual(api[config_name], value)

    # If the value is not set, verify that it's not there.
    for property_name, config_name, value in optional_props:

      @api_config.api('root2', 'v2')
      class EmptyService2(remote.Service):
        pass

      api = json.loads(self.generator.pretty_print_config_to_json(
          EmptyService2))
      self.assertNotIn(config_name, api)

  def testAuth(self):
    """Verify that auth shows up in the config if it's supposed to."""

    empty_auth = api_config.ApiAuth()
    used_auth = api_config.ApiAuth(allow_cookie_auth=False)
    cookie_auth = api_config.ApiAuth(allow_cookie_auth=True)
    empty_blocked_regions = api_config.ApiAuth(blocked_regions=[])
    one_blocked = api_config.ApiAuth(blocked_regions=['us'])
    many_blocked = api_config.ApiAuth(blocked_regions=['CU', 'IR', 'KP', 'SD',
                                                       'SY', 'MM'])
    mixed = api_config.ApiAuth(allow_cookie_auth=True,
                               blocked_regions=['US', 'IR'])

    for auth, expected_result in ((None, None),
                                  (empty_auth, None),
                                  (used_auth, {'allowCookieAuth': False}),
                                  (cookie_auth, {'allowCookieAuth': True}),
                                  (empty_blocked_regions, None),
                                  (one_blocked, {'blockedRegions': ['us']}),
                                  (many_blocked, {'blockedRegions':
                                                  ['CU', 'IR', 'KP', 'SD',
                                                   'SY', 'MM']}),
                                  (mixed, {'allowCookieAuth': True,
                                           'blockedRegions': ['US', 'IR']})):

      @api_config.api('root', 'v1', auth=auth)
      class EmptyService(remote.Service):
        pass

      api = json.loads(self.generator.pretty_print_config_to_json(EmptyService))
      if expected_result is None:
        self.assertNotIn('auth', api)
      else:
        self.assertEqual(api['auth'], expected_result)

  def testFrontEndLimits(self):
    """Verify that frontendLimits info in the API is written to the config."""
    rules = [
        api_config.ApiFrontEndLimitRule(match='foo', qps=234, user_qps=567,
                                        daily=8910, analytics_id='asdf'),
        api_config.ApiFrontEndLimitRule(match='bar', qps=0, user_qps=0,
                                        analytics_id='sdf1'),
        api_config.ApiFrontEndLimitRule()]
    frontend_limits = api_config.ApiFrontEndLimits(unregistered_user_qps=123,
                                                   unregistered_qps=456,
                                                   unregistered_daily=789,
                                                   rules=rules)

    @api_config.api('root', 'v1', frontend_limits=frontend_limits)
    class EmptyService(remote.Service):
      pass

    api = json.loads(self.generator.pretty_print_config_to_json(EmptyService))
    self.assertIn('frontendLimits', api)
    self.assertEqual(123, api['frontendLimits'].get('unregisteredUserQps'))
    self.assertEqual(456, api['frontendLimits'].get('unregisteredQps'))
    self.assertEqual(789, api['frontendLimits'].get('unregisteredDaily'))
    self.assertEqual(2, len(api['frontendLimits'].get('rules')))
    self.assertEqual('foo', api['frontendLimits']['rules'][0]['match'])
    self.assertEqual(234, api['frontendLimits']['rules'][0]['qps'])
    self.assertEqual(567, api['frontendLimits']['rules'][0]['userQps'])
    self.assertEqual(8910, api['frontendLimits']['rules'][0]['daily'])
    self.assertEqual('asdf', api['frontendLimits']['rules'][0]['analyticsId'])
    self.assertEqual('bar', api['frontendLimits']['rules'][1]['match'])
    self.assertEqual(0, api['frontendLimits']['rules'][1]['qps'])
    self.assertEqual(0, api['frontendLimits']['rules'][1]['userQps'])
    self.assertNotIn('daily', api['frontendLimits']['rules'][1])
    self.assertEqual('sdf1', api['frontendLimits']['rules'][1]['analyticsId'])

  def testAllCombinationsRepeatedRequiredDefault(self):

    # TODO(kdeus): When the backwards compatibility for non-ResourceContainer
    #              parameters requests is removed, this class and the
    #              accompanying method should be removed.
    class AllCombinations(messages.Message):
      """Documentation for AllCombinations."""
      string = messages.StringField(1)
      string_required = messages.StringField(2, required=True)
      string_default_required = messages.StringField(3, required=True,
                                                     default='Foo')
      string_repeated = messages.StringField(4, repeated=True)
      enum_value = messages.EnumField(SimpleEnum, 5, default=SimpleEnum.VAL2)

    all_combinations_container = resource_container.ResourceContainer(
        **{field.name: field for field in AllCombinations.all_fields()})

    @api_config.api('root', 'v1', hostname='example.appspot.com')
    class MySimpleService(remote.Service):

      @api_config.method(AllCombinations, message_types.VoidMessage,
                         path='foo', http_method='GET')
      def get(self, unused_request):
        return message_types.VoidMessage()

      @api_config.method(all_combinations_container, message_types.VoidMessage,
                         name='getContainer',
                         path='bar', http_method='GET')
      def get_container(self, unused_request):
        return message_types.VoidMessage()

    api = json.loads(self.generator.pretty_print_config_to_json(
        MySimpleService))

    get_config = {
        'httpMethod': 'GET',
        'path': 'foo',
        'request': {
            'body': 'empty',
            'parameterOrder': [
                'string_required',
                'string_default_required',
                ],
            'parameters': {
                'enum_value': {
                    'default': 'VAL2',
                    'type': 'string',
                    'enum': {
                        'VAL1': {
                            'backendValue': 'VAL1',
                            },
                        'VAL2': {
                            'backendValue': 'VAL2',
                            },
                        },
                    },
                'string': {
                    'type': 'string',
                    },
                'string_default_required': {
                    'default': 'Foo',
                    'required': True,
                    'type': 'string',
                    },
                'string_repeated': {
                    'type': 'string',
                    'repeated': True,
                    },
                'string_required': {
                    'required': True,
                    'type': 'string',
                    },
                },
            },
        'response': {
            'body': 'empty',
            },
        'rosyMethod': 'MySimpleService.get',
        'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
        'clientIds': [api_config.API_EXPLORER_CLIENT_ID],
        'authLevel': 'NONE',
        }

    get_container_config = get_config.copy()
    get_container_config['path'] = 'bar'
    get_container_config['rosyMethod'] = 'MySimpleService.get_container'
    expected = {
        'root.get': get_config,
        'root.getContainer': get_container_config
    }

    test_util.AssertDictEqual(expected, api['methods'], self)

  def testMultipleClassesSingleApi(self):
    """Test an API that's split into multiple classes."""

    root_api = api_config.api('root', 'v1', hostname='example.appspot.com')

    # First class has a request that reads some arguments.
    class Response1(messages.Message):
      string_value = messages.StringField(1)

    @root_api.api_class(resource_name='request')
    class RequestService(remote.Service):

      @api_config.method(message_types.VoidMessage, Response1,
                         path='request_path', http_method='GET')
      def my_request(self, unused_request):
        pass

    # Second class, no methods.
    @root_api.api_class(resource_name='empty')
    class EmptyService(remote.Service):
      pass

    # Third class (& data), one method that returns a response.
    class Response2(messages.Message):
      bool_value = messages.BooleanField(1)
      int32_value = messages.IntegerField(2)

    @root_api.api_class(resource_name='simple')
    class MySimpleService(remote.Service):

      @api_config.method(message_types.VoidMessage, Response2,
                         name='entries.get', path='entries')
      def EntriesGet(self, request):
        pass

    # Make sure api info is the same for all classes and all the _ApiInfo
    # properties are accessible.
    for cls in (RequestService, EmptyService, MySimpleService):
      self.assertEqual(cls.api_info.name, 'root')
      self.assertEqual(cls.api_info.version, 'v1')
      self.assertEqual(cls.api_info.hostname, 'example.appspot.com')
      self.assertIsNone(cls.api_info.audiences)
      self.assertEqual(cls.api_info.allowed_client_ids,
                       [api_config.API_EXPLORER_CLIENT_ID])
      self.assertEqual(cls.api_info.scopes, [api_config.EMAIL_SCOPE])

    # Get the config for the combination of all 3.
    api = json.loads(self.generator.pretty_print_config_to_json(
        [RequestService, EmptyService, MySimpleService]))
    expected = {
        'root.request.my_request': {
            'httpMethod': 'GET',
            'path': 'request_path',
            'request': {'body': 'empty'},
            'response': {
                'body': 'autoTemplate(backendResponse)',
                'bodyName': 'resource'},
            'rosyMethod': 'RequestService.my_request',
            'clientIds': ['292824132082.apps.googleusercontent.com'],
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'authLevel': 'NONE',
            },
        'root.simple.entries.get': {
            'httpMethod': 'POST',
            'path': 'entries',
            'request': {'body': 'empty'},
            'response': {
                'body': 'autoTemplate(backendResponse)',
                'bodyName': 'resource'},
            'rosyMethod': 'MySimpleService.EntriesGet',
            'clientIds': ['292824132082.apps.googleusercontent.com'],
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'authLevel': 'NONE',
            },
        }
    test_util.AssertDictEqual(expected, api['methods'], self)
    expected_descriptor = {
        'methods': {
            'MySimpleService.EntriesGet': {
                'response': {
                    '$ref': (_DESCRIPTOR_PATH_PREFIX +
                             'ApiConfigTestResponse2')
                    }
                },
            'RequestService.my_request': {
                'response': {
                    '$ref': (_DESCRIPTOR_PATH_PREFIX +
                             'ApiConfigTestResponse1')
                    }
                }
            },
        'schemas': {
            _DESCRIPTOR_PATH_PREFIX + 'ApiConfigTestResponse1': {
                'id': _DESCRIPTOR_PATH_PREFIX + 'ApiConfigTestResponse1',
                'properties': {
                    'string_value': {
                        'type': 'string'
                        }
                    },
                'type': 'object'
                },
            _DESCRIPTOR_PATH_PREFIX + 'ApiConfigTestResponse2': {
                'id': _DESCRIPTOR_PATH_PREFIX + 'ApiConfigTestResponse2',
                'properties': {
                    'bool_value': {
                        'type': 'boolean'
                        },
                    'int32_value': {
                        'format': 'int64',
                        'type': 'string'
                        }
                    },
                'type': 'object'
                }
            }
        }

    test_util.AssertDictEqual(expected_descriptor, api['descriptor'], self)

  def testMultipleClassesDifferentDecoratorInstance(self):
    """Test that using different instances of @api fails."""

    root_api1 = api_config.api('root', 'v1', hostname='example.appspot.com')
    root_api2 = api_config.api('root', 'v1', hostname='example.appspot.com')

    @root_api1.api_class()
    class EmptyService1(remote.Service):
      pass

    @root_api2.api_class()
    class EmptyService2(remote.Service):
      pass

    self.assertRaises(api_exceptions.ApiConfigurationError,
                      self.generator.pretty_print_config_to_json,
                      [EmptyService1, EmptyService2])

  def testMultipleClassesUsingSingleApiDecorator(self):
    """Test an API that's split into multiple classes using @api."""

    @api_config.api('api', 'v1')
    class EmptyService1(remote.Service):
      pass

    @api_config.api('api', 'v1')
    class EmptyService2(remote.Service):
      pass

    self.assertRaises(api_exceptions.ApiConfigurationError,
                      self.generator.pretty_print_config_to_json,
                      [EmptyService1, EmptyService2])

  def testMultipleClassesRepeatedResourceName(self):
    """Test a multiclass API that reuses a resource_name."""

    root_api = api_config.api('root', 'v1', hostname='example.appspot.com')

    @root_api.api_class(resource_name='repeated')
    class Service1(remote.Service):

      @api_config.method(message_types.VoidMessage, message_types.VoidMessage,
                         name='get', http_method='GET', path='get')
      def get(self, request):
        pass

    @root_api.api_class(resource_name='repeated')
    class Service2(remote.Service):

      @api_config.method(message_types.VoidMessage, message_types.VoidMessage,
                         name='list', http_method='GET', path='list')
      def list(self, request):
        pass

    api = json.loads(self.generator.pretty_print_config_to_json(
        [Service1, Service2]))
    expected = {
        'root.repeated.get': {
            'httpMethod': 'GET',
            'path': 'get',
            'request': {'body': 'empty'},
            'response': {'body': 'empty'},
            'rosyMethod': 'Service1.get',
            'clientIds': [api_config.API_EXPLORER_CLIENT_ID],
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'authLevel': 'NONE',
            },
        'root.repeated.list': {
            'httpMethod': 'GET',
            'path': 'list',
            'request': {'body': 'empty'},
            'response': {'body': 'empty'},
            'rosyMethod': 'Service2.list',
            'clientIds': [api_config.API_EXPLORER_CLIENT_ID],
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'authLevel': 'NONE',
            },
        }
    test_util.AssertDictEqual(expected, api['methods'], self)

  def testMultipleClassesRepeatedMethodName(self):
    """Test a multiclass API that reuses a method name."""

    root_api = api_config.api('root', 'v1', hostname='example.appspot.com')

    @root_api.api_class(resource_name='repeated')
    class Service1(remote.Service):

      @api_config.method(message_types.VoidMessage, message_types.VoidMessage,
                         name='get', http_method='GET')
      def get(self, request):
        pass

    @root_api.api_class(resource_name='repeated')
    class Service2(remote.Service):

      @api_config.method(message_types.VoidMessage, message_types.VoidMessage,
                         name='get', http_method='POST')
      def get(self, request):
        pass

    self.assertRaises(api_exceptions.ApiConfigurationError,
                      self.generator.pretty_print_config_to_json,
                      [Service1, Service2])

  def testRepeatedRestPathAndHttpMethod(self):
    """If the same HTTP method & path are reused, that should raise an error."""

    @api_config.api(name='root', version='v1', hostname='example.appspot.com')
    class MySimpleService(remote.Service):

      @api_config.method(message_types.VoidMessage, message_types.VoidMessage,
                         path='path', http_method='GET')
      def Path1(self, unused_request):
        return message_types.VoidMessage()

      @api_config.method(message_types.VoidMessage, message_types.VoidMessage,
                         path='path', http_method='GET')
      def Path2(self, unused_request):
        return message_types.VoidMessage()

    self.assertRaises(api_exceptions.ApiConfigurationError,
                      self.generator.pretty_print_config_to_json,
                      MySimpleService)

  def testMulticlassRepeatedRestPathAndHttpMethod(self):
    """If the same HTTP method & path are reused, that should raise an error."""

    root_api = api_config.api('root', 'v1', hostname='example.appspot.com')

    @root_api.api_class(resource_name='resource1')
    class Service1(remote.Service):

      @api_config.method(message_types.VoidMessage, message_types.VoidMessage,
                         path='path', http_method='GET')
      def Path1(self, unused_request):
        return message_types.VoidMessage()

    @root_api.api_class(resource_name='resource2')
    class Service2(remote.Service):

      @api_config.method(message_types.VoidMessage, message_types.VoidMessage,
                         path='path', http_method='GET')
      def Path2(self, unused_request):
        return message_types.VoidMessage()

    self.assertRaises(api_exceptions.ApiConfigurationError,
                      self.generator.pretty_print_config_to_json,
                      [Service1, Service2])

  def testRepeatedRpcMethodName(self):
    """Test an API that reuses the same RPC name for two methods."""

    @api_config.api('root', 'v1', hostname='example.appspot.com')
    class MyService(remote.Service):

      @api_config.method(message_types.VoidMessage, message_types.VoidMessage,
                         name='get', http_method='GET', path='path1')
      def get(self, request):
        pass

      @api_config.method(message_types.VoidMessage, message_types.VoidMessage,
                         name='get', http_method='GET', path='path2')
      def another_get(self, request):
        pass

    self.assertRaises(api_exceptions.ApiConfigurationError,
                      self.generator.pretty_print_config_to_json, [MyService])

  def testMultipleClassesRepeatedMethodNameUniqueResource(self):
    """Test a multiclass API reusing a method name but different resource."""

    root_api = api_config.api('root', 'v1', hostname='example.appspot.com')

    @root_api.api_class(resource_name='resource1')
    class Service1(remote.Service):

      @api_config.method(message_types.VoidMessage, message_types.VoidMessage,
                         name='get', http_method='GET', path='get1')
      def get(self, request):
        pass

    @root_api.api_class(resource_name='resource2')
    class Service2(remote.Service):

      @api_config.method(message_types.VoidMessage, message_types.VoidMessage,
                         name='get', http_method='GET', path='get2')
      def get(self, request):
        pass

    api = json.loads(self.generator.pretty_print_config_to_json(
        [Service1, Service2]))
    expected = {
        'root.resource1.get': {
            'httpMethod': 'GET',
            'path': 'get1',
            'request': {'body': 'empty'},
            'response': {'body': 'empty'},
            'rosyMethod': 'Service1.get',
            'clientIds': [api_config.API_EXPLORER_CLIENT_ID],
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'authLevel': 'NONE',
            },
        'root.resource2.get': {
            'httpMethod': 'GET',
            'path': 'get2',
            'request': {'body': 'empty'},
            'response': {'body': 'empty'},
            'rosyMethod': 'Service2.get',
            'clientIds': [api_config.API_EXPLORER_CLIENT_ID],
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'authLevel': 'NONE',
            },
        }
    test_util.AssertDictEqual(expected, api['methods'], self)

  def testMultipleClassesRepeatedMethodNameUniqueResourceParams(self):
    """Test the same method name with different args in different resources."""

    root_api = api_config.api('root', 'v1', hostname='example.appspot.com')

    class Request1(messages.Message):
      bool_value = messages.BooleanField(1)

    class Response1(messages.Message):
      bool_value = messages.BooleanField(1)

    class Request2(messages.Message):
      bool_value = messages.BooleanField(1)

    class Response2(messages.Message):
      bool_value = messages.BooleanField(1)

    @root_api.api_class(resource_name='resource1')
    class Service1(remote.Service):

      @api_config.method(Request1, Response1,
                         name='get', http_method='GET', path='get1')
      def get(self, request):
        pass

    @root_api.api_class(resource_name='resource2')
    class Service2(remote.Service):

      @api_config.method(Request2, Response2,
                         name='get', http_method='GET', path='get2')
      def get(self, request):
        pass

    api = json.loads(self.generator.pretty_print_config_to_json(
        [Service1, Service2]))
    expected = {
        'root.resource1.get': {
            'httpMethod': 'GET',
            'path': 'get1',
            'request': {
                'body': 'empty',
                'parameters': {
                    'bool_value': {
                        'type': 'boolean'
                        }
                    }
                },
            'response': {'body': 'autoTemplate(backendResponse)',
                         'bodyName': 'resource'},
            'rosyMethod': 'Service1.get',
            'clientIds': [api_config.API_EXPLORER_CLIENT_ID],
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'authLevel': 'NONE',
            },
        'root.resource2.get': {
            'httpMethod': 'GET',
            'path': 'get2',
            'request': {
                'body': 'empty',
                'parameters': {
                    'bool_value': {
                        'type': 'boolean'
                        }
                    }
                },
            'response': {'body': 'autoTemplate(backendResponse)',
                         'bodyName': 'resource'},
            'rosyMethod': 'Service2.get',
            'clientIds': [api_config.API_EXPLORER_CLIENT_ID],
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'authLevel': 'NONE',
            },
        }
    test_util.AssertDictEqual(expected, api['methods'], self)

    expected_descriptor = {
        'methods': {
            'Service1.get': {
                'response': {
                    '$ref': (_DESCRIPTOR_PATH_PREFIX +
                             'ApiConfigTestResponse1')
                    }
                },
            'Service2.get': {
                'response': {
                    '$ref': (_DESCRIPTOR_PATH_PREFIX +
                             'ApiConfigTestResponse2')
                    }
                }
            },
        'schemas': {
            _DESCRIPTOR_PATH_PREFIX + 'ApiConfigTestResponse1': {
                'id': _DESCRIPTOR_PATH_PREFIX + 'ApiConfigTestResponse1',
                'properties': {
                    'bool_value': {
                        'type': 'boolean'
                        }
                    },
                'type': 'object'
                },
            _DESCRIPTOR_PATH_PREFIX + 'ApiConfigTestResponse2': {
                'id': _DESCRIPTOR_PATH_PREFIX + 'ApiConfigTestResponse2',
                'properties': {
                    'bool_value': {
                        'type': 'boolean'
                        }
                    },
                'type': 'object'
                }
            }
        }

    test_util.AssertDictEqual(expected_descriptor, api['descriptor'], self)

  def testMultipleClassesNoResourceName(self):
    """Test a multiclass API with a collection with no resource_name."""

    root_api = api_config.api('root', 'v1', hostname='example.appspot.com')

    @root_api.api_class()
    class TestService(remote.Service):

      @api_config.method(http_method='GET')
      def donothing(self):
        pass

      @api_config.method(http_method='POST', name='alternate')
      def foo(self):
        pass

    api = json.loads(self.generator.pretty_print_config_to_json(
        [TestService]))
    expected = {
        'root.donothing': {
            'httpMethod': 'GET',
            'path': 'donothing',
            'request': {'body': 'empty'},
            'response': {'body': 'empty'},
            'rosyMethod': 'TestService.donothing',
            'clientIds': ['292824132082.apps.googleusercontent.com'],
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'authLevel': 'NONE',
            },
        'root.alternate': {
            'httpMethod': 'POST',
            'path': 'foo',
            'request': {'body': 'empty'},
            'response': {'body': 'empty'},
            'rosyMethod': 'TestService.foo',
            'clientIds': ['292824132082.apps.googleusercontent.com'],
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'authLevel': 'NONE',
            },
        }
    test_util.AssertDictEqual(expected, api['methods'], self)

  def testMultipleClassesBasePathInteraction(self):
    """Test path appending in a multiclass API."""

    root_api = api_config.api('root', 'v1', hostname='example.appspot.com')

    @root_api.api_class(path='base_path')
    class TestService(remote.Service):

      @api_config.method(http_method='GET')
      def at_base(self):
        pass

      @api_config.method(http_method='GET', path='appended')
      def append_to_base(self):
        pass

      @api_config.method(http_method='GET', path='appended/more')
      def append_to_base2(self):
        pass

      @api_config.method(http_method='GET', path='/ignore_base')
      def absolute(self):
        pass

    api = json.loads(self.generator.pretty_print_config_to_json(
        [TestService]))
    expected = {
        'root.at_base': {
            'httpMethod': 'GET',
            'path': 'base_path/at_base',
            'request': {'body': 'empty'},
            'response': {'body': 'empty'},
            'rosyMethod': 'TestService.at_base',
            'clientIds': ['292824132082.apps.googleusercontent.com'],
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'authLevel': 'NONE',
            },
        'root.append_to_base': {
            'httpMethod': 'GET',
            'path': 'base_path/appended',
            'request': {'body': 'empty'},
            'response': {'body': 'empty'},
            'rosyMethod': 'TestService.append_to_base',
            'clientIds': ['292824132082.apps.googleusercontent.com'],
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'authLevel': 'NONE',
            },
        'root.append_to_base2': {
            'httpMethod': 'GET',
            'path': 'base_path/appended/more',
            'request': {'body': 'empty'},
            'response': {'body': 'empty'},
            'rosyMethod': 'TestService.append_to_base2',
            'clientIds': ['292824132082.apps.googleusercontent.com'],
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'authLevel': 'NONE',
            },
        'root.absolute': {
            'httpMethod': 'GET',
            'path': 'ignore_base',
            'request': {'body': 'empty'},
            'response': {'body': 'empty'},
            'rosyMethod': 'TestService.absolute',
            'clientIds': ['292824132082.apps.googleusercontent.com'],
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'authLevel': 'NONE',
            },
        }
    test_util.AssertDictEqual(expected, api['methods'], self)

  def testMultipleClassesDifferentCollectionDefaults(self):
    """Test a multi-class API with settings overridden per collection."""

    BASE_SCOPES = ['base_scope']
    BASE_CLIENT_IDS = ['base_client_id']
    root_api = api_config.api('root', 'v1', hostname='example.appspot.com',
                              audiences=['base_audience'],
                              scopes=BASE_SCOPES,
                              allowed_client_ids=BASE_CLIENT_IDS,
                              auth_level=AUTH_LEVEL.REQUIRED)

    @root_api.api_class(resource_name='one', audiences=[])
    class Service1(remote.Service):
      pass

    @root_api.api_class(resource_name='two', audiences=['audience2', 'foo'],
                        scopes=['service2_scope'],
                        allowed_client_ids=['s2_client_id'],
                        auth_level=AUTH_LEVEL.OPTIONAL)
    class Service2(remote.Service):
      pass

    self.assertEqual(Service1.api_info.audiences, [])
    self.assertEqual(Service1.api_info.scopes, BASE_SCOPES)
    self.assertEqual(Service1.api_info.allowed_client_ids, BASE_CLIENT_IDS)
    self.assertEqual(Service1.api_info.auth_level, AUTH_LEVEL.REQUIRED)
    self.assertEqual(Service2.api_info.audiences, ['audience2', 'foo'])
    self.assertEqual(Service2.api_info.scopes, ['service2_scope'])
    self.assertEqual(Service2.api_info.allowed_client_ids, ['s2_client_id'])
    self.assertEqual(Service2.api_info.auth_level, AUTH_LEVEL.OPTIONAL)

  def testResourceContainerWarning(self):
    """Check the warning if a ResourceContainer isn't used when it should be."""

    class TestGetRequest(messages.Message):
      item_id = messages.StringField(1)

    @api_config.api('myapi', 'v0', hostname='example.appspot.com')
    class MyApi(remote.Service):

      @api_config.method(TestGetRequest, message_types.VoidMessage,
                         path='test/{item_id}')
      def Test(self, unused_request):
        return message_types.VoidMessage()

    # Verify that there's a warning and the name of the method is included
    # in the warning.
    logging.warning = mock.Mock()
    self.generator.pretty_print_config_to_json(MyApi)
    logging.warning.assert_called_with(mock.ANY, 'myapi.test')

  def testFieldInPathWithBodyIsRequired(self):

    # TODO(kdeus): When the backwards compatibility for non-ResourceContainer
    #              parameters requests is removed, this class and the
    #              accompanying method should be removed.
    class ItemsUpdateRequest(messages.Message):
      itemId = messages.StringField(1)

    items_update_request_container = resource_container.ResourceContainer(
        **{field.name: field for field in ItemsUpdateRequest.all_fields()})

    @api_config.api(name='root', hostname='example.appspot.com', version='v1')
    class MyService(remote.Service):
      """Describes MyService."""

      @api_config.method(ItemsUpdateRequest, message_types.VoidMessage,
                         path='items/{itemId}', name='items.update',
                         http_method='PUT')
      def items_update(self, unused_request):
        return message_types.VoidMessage()

      @api_config.method(items_update_request_container,
                         path='items/container/{itemId}',
                         name='items.updateContainer',
                         http_method='PUT')
      def items_update_container(self, unused_request):
        return message_types.VoidMessage()

    api = json.loads(self.generator.pretty_print_config_to_json(MyService))
    params = {'itemId': {'required': True,
                         'type': 'string'}}
    param_order = ['itemId']
    items_update_config = {
        'httpMethod': 'PUT',
        'path': 'items/{itemId}',
        'request': {'body': 'autoTemplate(backendRequest)',
                    'bodyName': 'resource',
                    'parameters': params,
                    'parameterOrder': param_order},
        'response': {'body': 'empty'},
        'rosyMethod': 'MyService.items_update',
        'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
        'clientIds': [api_config.API_EXPLORER_CLIENT_ID],
        'authLevel': 'NONE',
        }

    update_container_cfg = items_update_config.copy()
    update_container_cfg['path'] = 'items/container/{itemId}'
    update_container_cfg['rosyMethod'] = 'MyService.items_update_container'
    # Since we don't have a body in our container, the request will be empty.
    request = update_container_cfg['request'].copy()
    request.pop('bodyName')
    request['body'] = 'empty'
    update_container_cfg['request'] = request
    expected = {
        'root.items.update': items_update_config,
        'root.items.updateContainer': update_container_cfg,
    }
    test_util.AssertDictEqual(expected, api['methods'], self)

  def testFieldInPathNoBodyIsRequired(self):

    class ItemsGetRequest(messages.Message):
      itemId = messages.StringField(1)

    @api_config.api(name='root', hostname='example.appspot.com', version='v1')
    class MyService(remote.Service):
      """Describes MyService."""

      @api_config.method(ItemsGetRequest, message_types.VoidMessage,
                         path='items/{itemId}', name='items.get',
                         http_method='GET')
      def items_get(self, unused_request):
        return message_types.VoidMessage()

    api = json.loads(self.generator.pretty_print_config_to_json(MyService))
    params = {'itemId': {'required': True,
                         'type': 'string'}}
    param_order = ['itemId']
    expected = {
        'root.items.get': {
            'httpMethod': 'GET',
            'path': 'items/{itemId}',
            'request': {'body': 'empty',
                        'parameters': params,
                        'parameterOrder': param_order},
            'response': {'body': 'empty'},
            'rosyMethod': 'MyService.items_get',
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'clientIds': [api_config.API_EXPLORER_CLIENT_ID],
            'authLevel': 'NONE',
            }
        }

    test_util.AssertDictEqual(expected, api['methods'], self)

  def testAuthLevelRequired(self):

    class ItemsGetRequest(messages.Message):
      itemId = messages.StringField(1)

    @api_config.api(name='root', hostname='example.appspot.com', version='v1')
    class MyService(remote.Service):
      """Describes MyService."""

      @api_config.method(ItemsGetRequest, message_types.VoidMessage,
                         path='items/{itemId}', name='items.get',
                         http_method='GET', auth_level=AUTH_LEVEL.REQUIRED)
      def items_get(self, unused_request):
        return message_types.VoidMessage()

    api = json.loads(self.generator.pretty_print_config_to_json(MyService))
    params = {'itemId': {'required': True,
                         'type': 'string'}}
    param_order = ['itemId']
    expected = {
        'root.items.get': {
            'httpMethod': 'GET',
            'path': 'items/{itemId}',
            'request': {'body': 'empty',
                        'parameters': params,
                        'parameterOrder': param_order},
            'response': {'body': 'empty'},
            'rosyMethod': 'MyService.items_get',
            'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
            'clientIds': [api_config.API_EXPLORER_CLIENT_ID],
            'authLevel': 'REQUIRED',
            }
        }

    test_util.AssertDictEqual(expected, api['methods'], self)

  def testCustomUrl(self):

    test_request = resource_container.ResourceContainer(
        message_types.VoidMessage,
        id=messages.IntegerField(1, required=True))

    @api_config.api(name='testapicustomurl', version='v3',
                    hostname='example.appspot.com',
                    description='A wonderful API.', base_path='/my/base/path/')
    class TestServiceCustomUrl(remote.Service):

      @api_config.method(test_request,
                         message_types.VoidMessage,
                         http_method='DELETE', path='items/{id}')
      # Silence lint warning about method naming conventions
      # pylint: disable=g-bad-name
      def delete(self, unused_request):
        return message_types.VoidMessage()

    api = json.loads(
        self.generator.pretty_print_config_to_json(TestServiceCustomUrl))

    expected_adapter = {
        'bns': 'https://example.appspot.com/my/base/path',
        'type': 'lily',
        'deadline': 10.0
    }

    test_util.AssertDictEqual(expected_adapter, api['adapter'], self)


class ApiConfigParamsDescriptorTest(unittest.TestCase):

  def setUp(self):
    self.generator = ApiConfigGenerator()

    class OtherRefClass(messages.Message):
      three = messages.BooleanField(1, repeated=True)
      four = messages.FloatField(2, required=True)
      five = messages.IntegerField(3, default=42)
    self.other_ref_class = OtherRefClass

    class RefClass(messages.Message):
      one = messages.StringField(1)
      two = messages.MessageField(OtherRefClass, 2)
      not_two = messages.MessageField(OtherRefClass, 3, required=True)
    self.ref_class = RefClass

    class RefClassForContainer(messages.Message):
      not_two = messages.MessageField(OtherRefClass, 3, required=True)

    ref_class_container = resource_container.ResourceContainer(
        RefClassForContainer,
        one=messages.StringField(1),
        two=messages.MessageField(OtherRefClass, 2))

    @api_config.api(name='root', hostname='example.appspot.com', version='v1')
    class MyService(remote.Service):

      @api_config.method(RefClass, RefClass,
                         name='entries.get',
                         path='/a/{two.three}/{two.four}',
                         http_method='GET')
      def entries_get(self, request):
        return request

      @api_config.method(RefClass, RefClass,
                         name='entries.put',
                         path='/b/{two.three}/{one}',
                         http_method='PUT')
      def entries_put(self, request):
        return request

      # Flatten the fields intended for the put request into only parameters.
      # This would not be a typical use, but is done to adhere to the behavior
      # in the non-ResourceContainer case.
      get_request_container = resource_container.ResourceContainer(
          **{field.name: field for field in
             ref_class_container.combined_message_class.all_fields()})

      @api_config.method(get_request_container, RefClass,
                         name='entries.getContainer',
                         path='/a/container/{two.three}/{two.four}',
                         http_method='GET')
      def entries_get_container(self, request):
        return request

      @api_config.method(ref_class_container, RefClass,
                         name='entries.putContainer',
                         path='/b/container/{two.three}/{one}',
                         http_method='PUT')
      def entries_put_container(self, request):
        return request

    self.api_str = self.generator.pretty_print_config_to_json(MyService)
    self.api = json.loads(self.api_str)

    self.m_field = messages.MessageField(RefClass, 1)
    self.m_field.name = 'm_field'

  def GetPrivateMethod(self, attr_name):
    protected_attr_name = '_ApiConfigGenerator__' + attr_name
    return getattr(self.generator, protected_attr_name)

  def testFieldToSubfieldsSimpleField(self):
    m_field = messages.StringField(1)
    expected = [[m_field]]
    self.assertItemsEqual(expected,
                          self.GetPrivateMethod('field_to_subfields')(m_field))

  def testFieldToSubfieldsSingleMessageField(self):
    class RefClass(messages.Message):
      one = messages.StringField(1)
      two = messages.IntegerField(2)
    m_field = messages.MessageField(RefClass, 1)
    expected = [
        [m_field, RefClass.one],
        [m_field, RefClass.two],
    ]
    self.assertItemsEqual(expected,
                          self.GetPrivateMethod('field_to_subfields')(m_field))

  def testFieldToSubfieldsDifferingDepth(self):
    expected = [
        [self.m_field, self.ref_class.one],
        [self.m_field, self.ref_class.two, self.other_ref_class.three],
        [self.m_field, self.ref_class.two, self.other_ref_class.four],
        [self.m_field, self.ref_class.two, self.other_ref_class.five],
        [self.m_field, self.ref_class.not_two, self.other_ref_class.three],
        [self.m_field, self.ref_class.not_two, self.other_ref_class.four],
        [self.m_field, self.ref_class.not_two, self.other_ref_class.five],
    ]
    self.assertItemsEqual(
        expected, self.GetPrivateMethod('field_to_subfields')(self.m_field))

  def testGetPathParameters(self):
    get_path_parameters = self.GetPrivateMethod('get_path_parameters')
    expected = {
        'c': ['c'],
        'd': ['d.e'],
    }
    test_util.AssertDictEqual(
        expected, get_path_parameters('/a/b/{c}/{d.e}/{}'), self)
    test_util.AssertDictEqual(
        {}, get_path_parameters('/stray{/brackets{in/the}middle'), self)

  def testValidatePathParameters(self):
    # This also tests __validate_simple_subfield indirectly
    validate_path_parameters = self.GetPrivateMethod('validate_path_parameters')
    self.assertRaises(TypeError, validate_path_parameters,
                      self.m_field, ['x'])
    self.assertRaises(TypeError, validate_path_parameters,
                      self.m_field, ['m_field'])
    self.assertRaises(TypeError, validate_path_parameters,
                      self.m_field, ['m_field.one_typo'])
    # This should not fail
    validate_path_parameters(self.m_field, ['m_field.one'])

  def MethodDescriptorTest(self, method_name, path, param_order, parameters):
    method_descriptor = self.api['methods'][method_name]
    self.assertEqual(method_descriptor['path'], path)
    request_descriptor = method_descriptor['request']
    self.assertEqual(param_order, request_descriptor['parameterOrder'])
    self.assertEqual(parameters, request_descriptor['parameters'])

  def testParametersDescriptorEntriesGet(self):
    parameters = {
        'one': {
            'type': 'string',
        },
        'two.three': {
            'repeated': True,
            'required': True,
            'type': 'boolean',
        },
        'two.four': {
            'required': True,
            'type': 'double',
        },
        'two.five': {
            'default': 42,
            'type': 'int64'
        },
        'not_two.three': {
            'repeated': True,
            'type': 'boolean',
        },
        'not_two.four': {
            'required': True,
            'type': 'double',
        },
        'not_two.five': {
            'default': 42,
            'type': 'int64'
        },
    }

    # Without container.
    self.MethodDescriptorTest('root.entries.get', 'a/{two.three}/{two.four}',
                              ['two.three', 'two.four', 'not_two.four'],
                              parameters)
    # With container.
    self.MethodDescriptorTest('root.entries.getContainer',
                              'a/container/{two.three}/{two.four}',
                              # Not parameter order differs because of the way
                              # combined_message_class combines classes. This
                              # is not so big a deal.
                              ['not_two.four', 'two.three', 'two.four'],
                              parameters)

  def testParametersDescriptorEntriesPut(self):
    param_order = ['one', 'two.three']
    parameters = {
        'one': {
            'required': True,
            'type': 'string',
        },
        'two.three': {
            'repeated': True,
            'required': True,
            'type': 'boolean',
        },
        'two.four': {
            'type': 'double',
        },
        'two.five': {
            'default': 42,
            'type': 'int64'
        },
    }

    # Without container.
    self.MethodDescriptorTest('root.entries.put', 'b/{two.three}/{one}',
                              param_order, parameters)
    # With container.
    self.MethodDescriptorTest('root.entries.putContainer',
                              'b/container/{two.three}/{one}',
                              param_order, parameters)


class ApiDecoratorTest(unittest.TestCase):

  def testApiInfoPopulated(self):

    @api_config.api(name='CoolService', version='vX',
                    description='My Cool Service', hostname='myhost.com',
                    canonical_name='Cool Service Name')
    class MyDecoratedService(remote.Service):
      """Describes MyDecoratedService."""
      pass

    api_info = MyDecoratedService.api_info
    self.assertEqual('CoolService', api_info.name)
    self.assertEqual('vX', api_info.version)
    self.assertEqual('My Cool Service', api_info.description)
    self.assertEqual('myhost.com', api_info.hostname)
    self.assertEqual('Cool Service Name', api_info.canonical_name)
    self.assertIsNone(api_info.audiences)
    self.assertEqual([api_config.EMAIL_SCOPE], api_info.scopes)
    self.assertEqual([api_config.API_EXPLORER_CLIENT_ID],
                     api_info.allowed_client_ids)
    self.assertEqual(AUTH_LEVEL.NONE, api_info.auth_level)
    self.assertEqual(None, api_info.resource_name)
    self.assertEqual(None, api_info.path)

  def testApiInfoDefaults(self):

    @api_config.api('CoolService2', 'v2')
    class MyDecoratedService(remote.Service):
      """Describes MyDecoratedService."""
      pass

    api_info = MyDecoratedService.api_info
    self.assertEqual('CoolService2', api_info.name)
    self.assertEqual('v2', api_info.version)
    self.assertEqual(None, api_info.description)
    self.assertEqual(None, api_info.hostname)
    self.assertEqual(None, api_info.canonical_name)
    self.assertEqual(None, api_info.title)
    self.assertEqual(None, api_info.documentation)

  def testGetApiClassesSingle(self):
    """Test that get_api_classes works when one class has been decorated."""
    my_api = api_config.api(name='My Service', version='v1')

    @my_api
    class MyDecoratedService(remote.Service):
      """Describes MyDecoratedService."""

    self.assertEqual([MyDecoratedService], my_api.get_api_classes())

  def testGetApiClassesSingleCollection(self):
    """Test that get_api_classes works with the collection() decorator."""
    my_api = api_config.api(name='My Service', version='v1')

    @my_api.api_class(resource_name='foo')
    class MyDecoratedService(remote.Service):
      """Describes MyDecoratedService."""

    self.assertEqual([MyDecoratedService], my_api.get_api_classes())

  def testGetApiClassesMultiple(self):
    """Test that get_api_classes works with multiple classes."""
    my_api = api_config.api(name='My Service', version='v1')

    @my_api.api_class(resource_name='foo')
    class MyDecoratedService1(remote.Service):
      """Describes MyDecoratedService."""

    @my_api.api_class(resource_name='bar')
    class MyDecoratedService2(remote.Service):
      """Describes MyDecoratedService."""

    @my_api.api_class(resource_name='baz')
    class MyDecoratedService3(remote.Service):
      """Describes MyDecoratedService."""

    self.assertEqual([MyDecoratedService1, MyDecoratedService2,
                      MyDecoratedService3], my_api.get_api_classes())

  def testGetApiClassesMixedStyles(self):
    """Test that get_api_classes works when decorated differently."""
    my_api = api_config.api(name='My Service', version='v1')

    # @my_api is equivalent to @my_api.api_class().  This is allowed, though
    # mixing styles like this shouldn't be encouraged.
    @my_api
    class MyDecoratedService1(remote.Service):
      """Describes MyDecoratedService."""

    @my_api
    class MyDecoratedService2(remote.Service):
      """Describes MyDecoratedService."""

    @my_api.api_class(resource_name='foo')
    class MyDecoratedService3(remote.Service):
      """Describes MyDecoratedService."""

    self.assertEqual([MyDecoratedService1, MyDecoratedService2,
                      MyDecoratedService3], my_api.get_api_classes())


class MethodDecoratorTest(unittest.TestCase):

  def testMethodId(self):

    @api_config.api('foo', 'v2')
    class MyDecoratedService(remote.Service):
      """Describes MyDecoratedService."""

      @api_config.method()
      def get(self):
        pass

      @api_config.method()
      def people(self):
        pass

      @api_config.method()
      def _get(self):
        pass

      @api_config.method()
      def get_(self):
        pass

      @api_config.method()
      def _(self):
        pass

      @api_config.method()
      def _____(self):
        pass

      @api_config.method()
      def people_update(self):
        pass

      @api_config.method()
      def people_search(self):
        pass

      # pylint: disable=g-bad-name
      @api_config.method()
      def _several_underscores__in_various___places__(self):
        pass

    test_cases = [
        ('get', 'foo.get'),
        ('people', 'foo.people'),
        ('_get', 'foo.get'),
        ('get_', 'foo.get_'),
        ('_', 'foo.'),
        ('_____', 'foo.'),
        ('people_update', 'foo.people_update'),
        ('people_search', 'foo.people_search'),
        ('_several_underscores__in_various___places__',
         'foo.several_underscores__in_various___places__')
        ]

    for protorpc_method_name, expected in test_cases:
      method_id = ''
      info = getattr(MyDecoratedService, protorpc_method_name, None)
      self.assertIsNotNone(info)

      method_id = info.method_info.method_id(MyDecoratedService.api_info)
      self.assertEqual(expected, method_id,
                       'unexpected result (%s) for: %s' %
                       (method_id, protorpc_method_name))

  def testMethodInfoPopulated(self):

    @api_config.api(name='CoolService', version='vX',
                    description='My Cool Service', hostname='myhost.com')
    class MyDecoratedService(remote.Service):
      """Describes MyDecoratedService."""

      @api_config.method(request_message=Nested,
                         response_message=AllFields,
                         name='items.operate',
                         path='items',
                         http_method='GET',
                         scopes=['foo'],
                         audiences=['bar'],
                         allowed_client_ids=['baz', 'bim'],
                         auth_level=AUTH_LEVEL.REQUIRED)
      def my_method(self):
        pass

    method_info = MyDecoratedService.my_method.method_info
    protorpc_info = MyDecoratedService.my_method.remote
    self.assertEqual(Nested, protorpc_info.request_type)
    self.assertEqual(AllFields, protorpc_info.response_type)
    self.assertEqual('items.operate', method_info.name)
    self.assertEqual('items', method_info.get_path(MyDecoratedService.api_info))
    self.assertEqual('GET', method_info.http_method)
    self.assertEqual(['foo'], method_info.scopes)
    self.assertEqual(['bar'], method_info.audiences)
    self.assertEqual(['baz', 'bim'], method_info.allowed_client_ids)
    self.assertEqual(AUTH_LEVEL.REQUIRED, method_info.auth_level)

  def testMethodInfoDefaults(self):

    @api_config.api('CoolService2', 'v2')
    class MyDecoratedService(remote.Service):
      """Describes MyDecoratedService."""

      @api_config.method()
      def my_method(self):
        pass

    method_info = MyDecoratedService.my_method.method_info
    protorpc_info = MyDecoratedService.my_method.remote
    self.assertEqual(message_types.VoidMessage, protorpc_info.request_type)
    self.assertEqual(message_types.VoidMessage, protorpc_info.response_type)
    self.assertEqual('my_method', method_info.name)
    self.assertEqual('my_method',
                     method_info.get_path(MyDecoratedService.api_info))
    self.assertEqual('POST', method_info.http_method)
    self.assertEqual(None, method_info.scopes)
    self.assertEqual(None, method_info.audiences)
    self.assertEqual(None, method_info.allowed_client_ids)
    self.assertEqual(None, method_info.auth_level)

  def testMethodInfoPath(self):

    class MyRequest(messages.Message):
      """Documentation for MyRequest."""
      zebra = messages.StringField(1, required=True)
      kitten = messages.StringField(2, required=True)
      dog = messages.StringField(3)
      panda = messages.StringField(4, required=True)

    @api_config.api('CoolService3', 'v3')
    class MyDecoratedService(remote.Service):
      """Describes MyDecoratedService."""

      @api_config.method(MyRequest, message_types.VoidMessage)
      def default_path_method(self):
        pass

      @api_config.method(MyRequest, message_types.VoidMessage,
                         path='zebras/{zebra}/pandas/{panda}/kittens/{kitten}')
      def specified_path_method(self):
        pass

    specified_path_info = MyDecoratedService.specified_path_method.method_info
    specified_protorpc_info = MyDecoratedService.specified_path_method.remote
    self.assertEqual(MyRequest, specified_protorpc_info.request_type)
    self.assertEqual(message_types.VoidMessage,
                     specified_protorpc_info.response_type)
    self.assertEqual('specified_path_method', specified_path_info.name)
    self.assertEqual('zebras/{zebra}/pandas/{panda}/kittens/{kitten}',
                     specified_path_info.get_path(MyDecoratedService.api_info))
    self.assertEqual('POST', specified_path_info.http_method)
    self.assertEqual(None, specified_path_info.scopes)
    self.assertEqual(None, specified_path_info.audiences)
    self.assertEqual(None, specified_path_info.allowed_client_ids)
    self.assertEqual(None, specified_path_info.auth_level)

    default_path_info = MyDecoratedService.default_path_method.method_info
    default_protorpc_info = MyDecoratedService.default_path_method.remote
    self.assertEqual(MyRequest, default_protorpc_info.request_type)
    self.assertEqual(message_types.VoidMessage,
                     default_protorpc_info.response_type)
    self.assertEqual('default_path_method', default_path_info.name)
    self.assertEqual('default_path_method',
                     default_path_info.get_path(MyDecoratedService.api_info))
    self.assertEqual('POST', default_path_info.http_method)
    self.assertEqual(None, default_path_info.scopes)
    self.assertEqual(None, default_path_info.audiences)
    self.assertEqual(None, default_path_info.allowed_client_ids)
    self.assertEqual(None, specified_path_info.auth_level)

  def testInvalidPaths(self):
    for path in ('invalid/mixed{param}',
                 'invalid/{param}mixed',
                 'invalid/mixed{param}mixed',
                 'invalid/{extra}{vars}',
                 'invalid/{}/emptyvar'):

      @api_config.api('root', 'v1')
      class MyDecoratedService(remote.Service):
        """Describes MyDecoratedService."""

        @api_config.method(message_types.VoidMessage, message_types.VoidMessage,
                           path=path)
        def test(self):
          pass

      self.assertRaises(api_exceptions.ApiConfigurationError,
                        MyDecoratedService.test.method_info.get_path,
                        MyDecoratedService.api_info)

  def testMethodAttributeInheritance(self):
    """Test descriptor attributes that can be inherited from the main config."""
    self.TryListAttributeVariations('audiences', 'audiences', None)
    self.TryListAttributeVariations(
        'scopes', 'scopes',
        ['https://www.googleapis.com/auth/userinfo.email'])
    self.TryListAttributeVariations('allowed_client_ids', 'clientIds',
                                    [api_config.API_EXPLORER_CLIENT_ID])

  def TryListAttributeVariations(self, attribute_name, config_name,
                                 default_expected):
    """Test setting an attribute in the API config and method configs.

    The audiences, scopes and allowed_client_ids settings can be set
    in either the main API config or on each of the methods.  This helper
    function tests each variation of one of these (whichever is specified)
    and ensures that the api config has the right values.

    Args:
      attribute_name: Name of the keyword arg to pass to the api or method
        decorator.  Also the name of the attribute used to access that
        variable on api_info or method_info.
      config_name: Name of the variable as it appears in the configuration
        output.
      default_expected: The default expected value if the attribute isn't
        specified on either the api or the method.
    """

    # Try the various combinations of api-level and method-level settings.
    # Test cases are: (api-setting, method-setting, expected)
    test_cases = ((None, ['foo', 'bar'], ['foo', 'bar']),
                  (None, [], None),
                  (['foo', 'bar'], None, ['foo', 'bar']),
                  (['foo', 'bar'], ['foo', 'bar'], ['foo', 'bar']),
                  (['foo', 'bar'], ['foo', 'baz'], ['foo', 'baz']),
                  (['foo', 'bar'], [], None),
                  (['foo', 'bar'], ['abc'], ['abc']),
                  (None, None, default_expected))
    for api_value, method_value, expected_value in test_cases:
      api_kwargs = {attribute_name: api_value}
      method_kwargs = {attribute_name: method_value}

      @api_config.api('AuthService', 'v1', hostname='example.appspot.com',
                      **api_kwargs)
      class AuthServiceImpl(remote.Service):
        """Describes AuthServiceImpl."""

        @api_config.method(**method_kwargs)
        def baz(self):
          pass

      self.assertEqual(api_value if api_value is not None else default_expected,
                       getattr(AuthServiceImpl.api_info, attribute_name))
      self.assertEqual(method_value,
                       getattr(AuthServiceImpl.baz.method_info, attribute_name))

      generator = ApiConfigGenerator()
      api = json.loads(generator.pretty_print_config_to_json(AuthServiceImpl))
      expected = {
          'authService.baz': {
              'httpMethod': 'POST',
              'path': 'baz',
              'request': {'body': 'empty'},
              'response': {'body': 'empty'},
              'rosyMethod': 'AuthServiceImpl.baz',
              'scopes': ['https://www.googleapis.com/auth/userinfo.email'],
              'clientIds': [api_config.API_EXPLORER_CLIENT_ID],
              'authLevel': 'NONE'
              }
          }
      if expected_value:
        expected['authService.baz'][config_name] = expected_value
      elif config_name in expected['authService.baz']:
        del expected['authService.baz'][config_name]

      test_util.AssertDictEqual(expected, api['methods'], self)


if __name__ == '__main__':
  unittest.main()
