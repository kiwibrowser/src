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

"""Tests for message_parser."""

import difflib
import json
import unittest

import endpoints.message_parser as message_parser
from protorpc import message_types
from protorpc import messages

import test_util


package = 'TestPackage'


class ModuleInterfaceTest(test_util.ModuleInterfaceTest,
                          unittest.TestCase):

  MODULE = message_parser


def _assertSchemaEqual(expected, actual, testcase):
  """Utility method to dump diffs if the schema aren't equal.

  Args:
    expected: object, the expected results.
    actual: object, the actual results.
    testcase: unittest.TestCase, the test case this assertion is used within.
  """
  if expected != actual:
    expected_text = json.dumps(expected, indent=2, sort_keys=True)
    actual_text = json.dumps(actual, indent=2, sort_keys=True)
    diff = difflib.unified_diff(expected_text.splitlines(True),
                                actual_text.splitlines(True),
                                fromfile='expected.schema',
                                tofile='actual.schema')
    diff_text = ''.join(list(diff))
    testcase.fail('Schema differs from expected:\n%s' % diff_text)


class SelfReference(messages.Message):
  """This must be at top level to be found by MessageField."""
  self = messages.MessageField('SelfReference', 1)


class MessageTypeToJsonSchemaTest(unittest.TestCase):

  def testSelfReferenceMessageField(self):
    """MessageFields should be recursively parsed."""

    parser = message_parser.MessageTypeToJsonSchema()
    parser.add_message(SelfReference)
    schemas = parser.schemas()
    self.assertEquals(1, len(schemas))
    self.assertTrue(package + 'SelfReference' in schemas)

  def testRecursiveDescent(self):
    """MessageFields should be recursively parsed."""

    class C(messages.Message):
      text = messages.StringField(1, required=True)

    class B(messages.Message):
      c = messages.MessageField(C, 1)

    class A(messages.Message):
      b = messages.MessageField(B, 1, repeated=True)

    parser = message_parser.MessageTypeToJsonSchema()
    parser.add_message(A)
    schemas = parser.schemas()
    self.assertEquals(3, len(schemas))
    self.assertTrue(package + 'A' in schemas)
    self.assertTrue(package + 'B' in schemas)
    self.assertTrue(package + 'C' in schemas)

  def testRepeatedAndRequired(self):
    """Repeated and required fields should show up as such in the schema."""

    class AllFields(messages.Message):
      """Documentation for AllFields."""
      string = messages.StringField(1)
      string_required = messages.StringField(2, required=True)
      string_default_required = messages.StringField(3, required=True,
                                                     default='Foo')
      string_repeated = messages.StringField(4, repeated=True)

      class SimpleEnum(messages.Enum):
        """Simple enumeration type."""
        VAL1 = 1
        VAL2 = 2

      enum_value = messages.EnumField(SimpleEnum, 5, default=SimpleEnum.VAL2)

    parser = message_parser.MessageTypeToJsonSchema()
    parser.add_message(AllFields)
    schemas = parser.schemas()

    expected = {
        package + 'AllFields': {
            'type': 'object',
            'id': package + 'AllFields',
            'description': 'Documentation for AllFields.',
            'properties': {
                'string': {
                    'type': 'string'
                    },
                'string_required': {
                    'type': 'string',
                    'required': True
                    },
                'string_default_required': {
                    'type': 'string',
                    'required': True,
                    'default': 'Foo'
                    },
                'string_repeated': {
                    'items': {
                        'type': 'string',
                        },
                    'type': 'array'
                    },
                'enum_value': {
                    'default': 'VAL2',
                    'type': 'string',
                    'enum': ['VAL1', 'VAL2']
                    },
                }
            }
        }
    _assertSchemaEqual(expected, schemas, self)

  def testAllFieldTypes(self):
    """Test all the Field types that ProtoRPC supports."""

    class AllTypes(messages.Message):
      """Contains all field types."""

      class SimpleEnum(messages.Enum):
        """Simple enumeration type."""
        VAL1 = 1
        VAL2 = 2

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
      int_value = messages.IntegerField(11)  # Default variant is INT64.
      datetime_value = message_types.DateTimeField(12)
      repeated_datetime_value = message_types.DateTimeField(13, repeated=True)

    parser = message_parser.MessageTypeToJsonSchema()
    parser.add_message(AllTypes)
    schemas = parser.schemas()

    expected = {
        package + 'AllTypes': {
            'type': 'object',
            'id': package + 'AllTypes',
            'description': 'Contains all field types.',
            'properties': {
                'bool_value': {'type': 'boolean'},
                'bytes_value': {'type': 'string', 'format': 'byte'},
                'double_value': {'type': 'number', 'format': 'double'},
                'enum_value': {'type': 'string', 'enum': ['VAL1', 'VAL2']},
                'float_value': {'type': 'number', 'format': 'float'},
                'int32_value': {'type': 'integer', 'format': 'int32'},
                'int64_value': {'type': 'string', 'format': 'int64'},
                'string_value': {'type': 'string'},
                'uint32_value': {'type': 'integer', 'format': 'uint32'},
                'uint64_value': {'type': 'string', 'format': 'uint64'},
                'int_value': {'type': 'string', 'format': 'int64'},
                'datetime_value': {'type': 'string', 'format': 'date-time'},
                'repeated_datetime_value':
                {'items': {'type': 'string', 'format': 'date-time'},
                 'type': 'array'}
                }
            }
        }

    _assertSchemaEqual(expected, schemas, self)

  def testLargeEnum(self):
    """Test that an enum with lots of values works."""

    class MyMessage(messages.Message):
      """Documentation for MyMessage."""

      class LargeEnum(messages.Enum):
        """Large enumeration type, in a strange order."""
        ALL = 1000
        AND = 1050
        BAR = 4
        BIND = 3141
        DARKNESS = 2123
        FOO = 3
        IN = 1200
        ONE = 5
        RING = 6
        RULE = 8
        THE = 1500
        THEM1 = 9
        THEM2 = 10000
        TO = 7
        VAL1 = 1
        VAL2 = 2

      enum_value = messages.EnumField(LargeEnum, 1)

    parser = message_parser.MessageTypeToJsonSchema()
    parser.add_message(MyMessage)
    schemas = parser.schemas()

    expected = {
        package + 'MyMessage': {
            'type': 'object',
            'id': package + 'MyMessage',
            'description': 'Documentation for MyMessage.',
            'properties': {
                'enum_value': {
                    'type': 'string',
                    'enum': ['VAL1', 'VAL2', 'FOO', 'BAR',
                             'ONE', 'RING', 'TO', 'RULE', 'THEM1', 'ALL',
                             'AND', 'IN', 'THE', 'DARKNESS', 'BIND', 'THEM2']
                    },
                }
            }
        }
    _assertSchemaEqual(expected, schemas, self)

  def testEmptyMessage(self):
    """Test the empty edge case."""

    class NoFields(messages.Message):
      pass

    parser = message_parser.MessageTypeToJsonSchema()
    parser.add_message(NoFields)
    schemas = parser.schemas()

    expected = {
        package + 'NoFields': {
            'type': 'object',
            'id': package + 'NoFields',
            'properties': {
                }
            }
        }

    _assertSchemaEqual(expected, schemas, self)

  def testRefForMessage(self):

    class NoFields(messages.Message):
      pass

    parser = message_parser.MessageTypeToJsonSchema()

    self.assertRaises(KeyError, parser.ref_for_message_type, NoFields)

    parser.add_message(NoFields)
    self.assertEqual(package + 'NoFields',
                     parser.ref_for_message_type(NoFields))

  def testMessageFieldDocsAndArrayRef(self):
    """Descriptions for MessageFields and a reference in an array."""

    class B(messages.Message):
      """A description of B."""
      pass

    class A(messages.Message):
      b = messages.MessageField(B, 1, repeated=True)

    parser = message_parser.MessageTypeToJsonSchema()
    parser.add_message(A)
    schemas = parser.schemas()

    expected = {
        package + 'A': {
            'type': 'object',
            'id': package + 'A',
            'properties': {
                'b': {
                    'type': 'array',
                    'description': 'A description of B.',
                    'items': {
                        '$ref': package + 'B'
                        }
                    }
                }
            },
        package + 'B': {
            'type': 'object',
            'id': package + 'B',
            'description': 'A description of B.',
            'properties': {}
            }
        }

    _assertSchemaEqual(expected, schemas, self)

  def testNormalizeSchemaName(self):

    class _1_lower_case_name_(messages.Message):
      pass

    parser = message_parser.MessageTypeToJsonSchema()
    # Test _, numbers, and case fixing.
    self.assertEqual(
        package + '1LowerCaseName',
        parser.add_message(_1_lower_case_name_))

  def testNormalizeSchemaNameCollision(self):

    class A(messages.Message):
      pass

    class A_(messages.Message):
      pass

    parser = message_parser.MessageTypeToJsonSchema()
    parser.add_message(A)
    self.assertRaises(KeyError, parser.add_message, A_)


if __name__ == '__main__':
  unittest.main()
