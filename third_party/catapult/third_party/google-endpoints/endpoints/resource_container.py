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

"""Module for a class that contains a request body resource and parameters."""

from protorpc import message_types
from protorpc import messages


class ResourceContainer(object):
  """Container for a request body resource combined with parameters.

  Used for API methods which may also have path or query parameters in addition
  to a request body.

  Attributes:
    body_message_class: A message class to represent a request body.
    parameters_message_class: A placeholder message class for request
        parameters.
  """

  __remote_info_cache = {}  # pylint: disable=g-bad-name

  __combined_message_class = None  # pylint: disable=invalid-name

  def __init__(self, _body_message_class=message_types.VoidMessage, **kwargs):
    """Constructor for ResourceContainer.

    Stores a request body message class and attempts to create one from the
    keyword arguments passed in.

    Args:
      _body_message_class: A keyword argument to be treated like a positional
          argument. This will not conflict with the potential names of fields
          since they can't begin with underscore. We make this a keyword
          argument since the default VoidMessage is a very common choice given
          the prevalence of GET methods.
      **kwargs: Keyword arguments specifying field names (the named arguments)
          and instances of ProtoRPC fields as the values.
    """
    self.body_message_class = _body_message_class
    self.parameters_message_class = type('ParameterContainer',
                                         (messages.Message,), kwargs)

  @property
  def combined_message_class(self):
    """A ProtoRPC message class with both request and parameters fields.

    Caches the result in a local private variable. Uses _CopyField to create
    copies of the fields from the existing request and parameters classes since
    those fields are "owned" by the message classes.

    Raises:
      TypeError: If a field name is used in both the request message and the
        parameters but the two fields do not represent the same type.

    Returns:
      Value of combined message class for this property.
    """
    if self.__combined_message_class is not None:
      return self.__combined_message_class

    fields = {}
    # We don't need to preserve field.number since this combined class is only
    # used for the protorpc remote.method and is not needed for the API config.
    # The only place field.number matters is in parameterOrder, but this is set
    # based on container.parameters_message_class which will use the field
    # numbers originally passed in.

    # Counter for fields.
    field_number = 1
    for field in self.body_message_class.all_fields():
      fields[field.name] = _CopyField(field, number=field_number)
      field_number += 1
    for field in self.parameters_message_class.all_fields():
      if field.name in fields:
        if not _CompareFields(field, fields[field.name]):
          raise TypeError('Field %r contained in both parameters and request '
                          'body, but the fields differ.' % (field.name,))
        else:
          # Skip a field that's already there.
          continue
      fields[field.name] = _CopyField(field, number=field_number)
      field_number += 1

    self.__combined_message_class = type('CombinedContainer',
                                         (messages.Message,), fields)
    return self.__combined_message_class

  @classmethod
  def add_to_cache(cls, remote_info, container):  # pylint: disable=g-bad-name
    """Adds a ResourceContainer to a cache tying it to a protorpc method.

    Args:
      remote_info: Instance of protorpc.remote._RemoteMethodInfo corresponding
          to a method.
      container: An instance of ResourceContainer.

    Raises:
      TypeError: if the container is not an instance of cls.
      KeyError: if the remote method has been reference by a container before.
          This created remote method should never occur because a remote method
          is created once.
    """
    if not isinstance(container, cls):
      raise TypeError('%r not an instance of %r, could not be added to cache.' %
                      (container, cls))
    if remote_info in cls.__remote_info_cache:
      raise KeyError('Cache has collision but should not.')
    cls.__remote_info_cache[remote_info] = container

  @classmethod
  def get_request_message(cls, remote_info):  # pylint: disable=g-bad-name
    """Gets request message or container from remote info.

    Args:
      remote_info: Instance of protorpc.remote._RemoteMethodInfo corresponding
          to a method.

    Returns:
      Either an instance of the request type from the remote or the
          ResourceContainer that was cached with the remote method.
    """
    if remote_info in cls.__remote_info_cache:
      return cls.__remote_info_cache[remote_info]
    else:
      return remote_info.request_type()


def _GetFieldAttributes(field):
  """Decomposes field into the needed arguments to pass to the constructor.

  This can be used to create copies of the field or to compare if two fields
  are "equal" (since __eq__ is not implemented on messages.Field).

  Args:
    field: A ProtoRPC message field (potentially to be copied).

  Raises:
    TypeError: If the field is not an instance of messages.Field.

  Returns:
    A pair of relevant arguments to be passed to the constructor for the field
      type. The first element is a list of positional arguments for the
      constructor and the second is a dictionary of keyword arguments.
  """
  if not isinstance(field, messages.Field):
    raise TypeError('Field %r to be copied not a ProtoRPC field.' % (field,))

  positional_args = []
  kwargs = {
      'required': field.required,
      'repeated': field.repeated,
      'variant': field.variant,
      'default': field._Field__default,  # pylint: disable=protected-access
  }

  if isinstance(field, messages.MessageField):
    # Message fields can't have a default
    kwargs.pop('default')
    if not isinstance(field, message_types.DateTimeField):
      positional_args.insert(0, field.message_type)
  elif isinstance(field, messages.EnumField):
    positional_args.insert(0, field.type)

  return positional_args, kwargs


def _CompareFields(field, other_field):
  """Checks if two ProtoRPC fields are "equal".

  Compares the arguments, rather than the id of the elements (which is
  the default __eq__ behavior) as well as the class of the fields.

  Args:
    field: A ProtoRPC message field to be compared.
    other_field: A ProtoRPC message field to be compared.

  Returns:
    Boolean indicating whether the fields are equal.
  """
  field_attrs = _GetFieldAttributes(field)
  other_field_attrs = _GetFieldAttributes(other_field)
  if field_attrs != other_field_attrs:
    return False
  return field.__class__ == other_field.__class__


def _CopyField(field, number=None):
  """Copies a (potentially) owned ProtoRPC field instance into a new copy.

  Args:
    field: A ProtoRPC message field to be copied.
    number: An integer for the field to override the number of the field.
        Defaults to None.

  Raises:
    TypeError: If the field is not an instance of messages.Field.

  Returns:
    A copy of the ProtoRPC message field.
  """
  positional_args, kwargs = _GetFieldAttributes(field)
  number = number or field.number
  positional_args.append(number)
  return field.__class__(*positional_args, **kwargs)
