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

"""Endpoints-specific implementation of ProtoRPC's ProtoJson class."""

import base64

from protorpc import messages
from protorpc import protojson

# pylint: disable=g-bad-name


__all__ = ['EndpointsProtoJson']


class EndpointsProtoJson(protojson.ProtoJson):
  """Endpoints-specific implementation of ProtoRPC's ProtoJson class.

  We need to adjust the way some types of data are encoded to ensure they're
  consistent with the existing API pipeline.  This class adjusts the JSON
  encoding as needed.

  This may be used in a multithreaded environment, so take care to ensure
  that this class (and its parent, protojson.ProtoJson) remain thread-safe.
  """

  def encode_field(self, field, value):
    """Encode a python field value to a JSON value.

    Args:
      field: A ProtoRPC field instance.
      value: A python value supported by field.

    Returns:
      A JSON serializable value appropriate for field.
    """
    # Override the handling of 64-bit integers, so they're always encoded
    # as strings.
    if (isinstance(field, messages.IntegerField) and
        field.variant in (messages.Variant.INT64,
                          messages.Variant.UINT64,
                          messages.Variant.SINT64)):
      if value not in (None, [], ()):
        # Convert and replace the value.
        if isinstance(value, list):
          value = [str(subvalue) for subvalue in value]
        else:
          value = str(value)
        return value

    return super(EndpointsProtoJson, self).encode_field(field, value)

  @staticmethod
  def __pad_value(value, pad_len_multiple, pad_char):
    """Add padding characters to the value if needed.

    Args:
      value: The string value to be padded.
      pad_len_multiple: Pad the result so its length is a multiple
          of pad_len_multiple.
      pad_char: The character to use for padding.

    Returns:
      The string value with padding characters added.
    """
    assert pad_len_multiple > 0
    assert len(pad_char) == 1
    padding_length = (pad_len_multiple -
                      (len(value) % pad_len_multiple)) % pad_len_multiple
    return value + pad_char * padding_length

  def decode_field(self, field, value):
    """Decode a JSON value to a python value.

    Args:
      field: A ProtoRPC field instance.
      value: A serialized JSON value.

    Returns:
      A Python value compatible with field.
    """
    # Override BytesField handling.  Client libraries typically use a url-safe
    # encoding.  b64decode doesn't handle these gracefully.  urlsafe_b64decode
    # handles both cases safely.  Also add padding if the padding is incorrect.
    if isinstance(field, messages.BytesField):
      try:
        # Need to call str(value) because ProtoRPC likes to pass values
        # as unicode, and urlsafe_b64decode can only handle bytes.
        padded_value = self.__pad_value(str(value), 4, '=')
        return base64.urlsafe_b64decode(padded_value)
      except (TypeError, UnicodeEncodeError), err:
        raise messages.DecodeError('Base64 decoding error: %s' % err)

    return super(EndpointsProtoJson, self).decode_field(field, value)
