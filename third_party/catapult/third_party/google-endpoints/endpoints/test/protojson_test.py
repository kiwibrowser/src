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

"""Tests for Endpoints-specific ProtoJson class."""

import json
import unittest

import endpoints.protojson as protojson
from protorpc import messages

import test_util


class MyMessage(messages.Message):
  """Test message containing various types."""
  var_int32 = messages.IntegerField(1, variant=messages.Variant.INT32)
  var_int64 = messages.IntegerField(2, variant=messages.Variant.INT64)
  var_repeated_int64 = messages.IntegerField(3, variant=messages.Variant.INT64,
                                             repeated=True)
  var_sint64 = messages.IntegerField(4, variant=messages.Variant.SINT64)
  var_uint64 = messages.IntegerField(5, variant=messages.Variant.UINT64)
  var_bytes = messages.BytesField(6)


class ModuleInterfaceTest(test_util.ModuleInterfaceTest,
                          unittest.TestCase):

  MODULE = protojson


class EndpointsProtoJsonTest(unittest.TestCase):

  def setUp(self):
    self.__protojson = protojson.EndpointsProtoJson()
    super(EndpointsProtoJsonTest, self).setUp()

  def CompareEncoded(self, expected_encoded, actual_encoded):
    """JSON encoding will be laundered to remove string differences."""
    self.assertEquals(json.loads(expected_encoded), json.loads(actual_encoded))

  def testEncodeInt32(self):
    """Make sure int32 values are encoded as integers."""
    encoded_message = self.__protojson.encode_message(MyMessage(var_int32=123))
    expected_encoding = '{"var_int32": 123}'
    self.CompareEncoded(expected_encoding, encoded_message)

  def testEncodeInt64(self):
    """Make sure int64 values are encoded as strings."""
    encoded_message = self.__protojson.encode_message(MyMessage(var_int64=123))
    expected_encoding = '{"var_int64": "123"}'
    self.CompareEncoded(expected_encoding, encoded_message)

  def testEncodeRepeatedInt64(self):
    """Make sure int64 in repeated fields are encoded as strings."""
    encoded_message = self.__protojson.encode_message(
        MyMessage(var_repeated_int64=[1, 2, 3]))
    expected_encoding = '{"var_repeated_int64": ["1", "2", "3"]}'
    self.CompareEncoded(expected_encoding, encoded_message)

  def testEncodeSint64(self):
    """Make sure sint64 values are encoded as strings."""
    encoded_message = self.__protojson.encode_message(MyMessage(var_sint64=-12))
    expected_encoding = '{"var_sint64": "-12"}'
    self.CompareEncoded(expected_encoding, encoded_message)

  def testEncodeUint64(self):
    """Make sure uint64 values are encoded as strings."""
    encoded_message = self.__protojson.encode_message(MyMessage(var_uint64=900))
    expected_encoding = '{"var_uint64": "900"}'
    self.CompareEncoded(expected_encoding, encoded_message)

  def testBytesNormal(self):
    """Verify that bytes encoded with standard b64 encoding are accepted."""
    for encoded, decoded in (('/+==', '\xff'),
                             ('/+/+', '\xff\xef\xfe'),
                             ('YWI+Y2Q/', 'ab>cd?')):
      self.assertEqual(decoded, self.__protojson.decode_field(
          MyMessage.var_bytes, encoded))

  def testBytesUrlSafe(self):
    """Verify that bytes encoded with urlsafe b64 encoding are accepted."""
    for encoded, decoded in (('_-==', '\xff'),
                             ('_-_-', '\xff\xef\xfe'),
                             ('YWI-Y2Q_', 'ab>cd?')):
      self.assertEqual(decoded, self.__protojson.decode_field(
          MyMessage.var_bytes, encoded))

  def testBytesMisalignedEncoding(self):
    """Verify that misaligned BytesField data raises an error."""
    for encoded in ('garbagebb', 'a===', 'a', 'abcde'):
      self.assertRaises(messages.DecodeError,
                        self.__protojson.decode_field,
                        MyMessage.var_bytes, encoded)

  def testBytesUnpaddedEncoding(self):
    """Verify that unpadded BytesField data is accepted."""
    for encoded, decoded in (('YQ', 'a'),
                             ('YWI', 'ab'),
                             ('_-', '\xff'),
                             ('VGVzdGluZyB1bnBhZGRlZCBtZXNzYWdlcw',
                              'Testing unpadded messages')):
      self.assertEqual(decoded, self.__protojson.decode_field(
          MyMessage.var_bytes, encoded))

  def testBytesInvalidChars(self):
    """Verify that invalid characters are ignored in BytesField encodings."""
    for encoded in ('\x00\x01\x02\x03', '\xff==='):
      self.assertEqual('', self.__protojson.decode_field(MyMessage.var_bytes,
                                                         encoded))

if __name__ == '__main__':
  unittest.main()
