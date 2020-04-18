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

"""A library for converting service configs to discovery docs."""

import collections
import json
import logging
import re

import api_exceptions
import message_parser
from protorpc import message_types
from protorpc import messages
from protorpc import remote
import resource_container
import util


_PATH_VARIABLE_PATTERN = r'{([a-zA-Z_][a-zA-Z_.\d]*)}'

_MULTICLASS_MISMATCH_ERROR_TEMPLATE = (
    'Attempting to implement service %s, version %s, with multiple '
    'classes that are not compatible. See docstring for api() for '
    'examples how to implement a multi-class API.')

_INVALID_AUTH_ISSUER = 'No auth issuer named %s defined in this Endpoints API.'

_API_KEY = 'api_key'
_API_KEY_PARAM = 'key'

CUSTOM_VARIANT_MAP = {
    messages.Variant.DOUBLE: ('number', 'double'),
    messages.Variant.FLOAT: ('number', 'float'),
    messages.Variant.INT64: ('string', 'int64'),
    messages.Variant.SINT64: ('string', 'int64'),
    messages.Variant.UINT64: ('string', 'uint64'),
    messages.Variant.INT32: ('integer', 'int32'),
    messages.Variant.SINT32: ('integer', 'int32'),
    messages.Variant.UINT32: ('integer', 'uint32'),
    messages.Variant.BOOL: ('boolean', None),
    messages.Variant.STRING: ('string', None),
    messages.Variant.BYTES: ('string', 'byte'),
    messages.Variant.ENUM: ('string', None),
}



class DiscoveryGenerator(object):
  """Generates a discovery doc from a ProtoRPC service.

  Example:

    class HelloRequest(messages.Message):
      my_name = messages.StringField(1, required=True)

    class HelloResponse(messages.Message):
      hello = messages.StringField(1, required=True)

    class HelloService(remote.Service):

      @remote.method(HelloRequest, HelloResponse)
      def hello(self, request):
        return HelloResponse(hello='Hello there, %s!' %
                             request.my_name)

    api_config = DiscoveryGenerator().pretty_print_config_to_json(HelloService)

  The resulting api_config will be a JSON discovery document describing the API
  implemented by HelloService.
  """

  # Constants for categorizing a request method.
  # __NO_BODY - Request without a request body, such as GET and DELETE methods.
  # __HAS_BODY - Request (such as POST/PUT/PATCH) with info in the request body.
  __NO_BODY = 1  # pylint: disable=invalid-name
  __HAS_BODY = 2  # pylint: disable=invalid-name

  def __init__(self):
    self.__parser = message_parser.MessageTypeToJsonSchema()

    # Maps method id to the request schema id.
    self.__request_schema = {}

    # Maps method id to the response schema id.
    self.__response_schema = {}

  def _get_resource_path(self, method_id):
    """Return the resource path for a method or an empty array if none."""
    return method_id.split('.')[1:-1]

  def _get_canonical_method_id(self, method_id):
    return method_id.split('.')[-1]

  def __get_request_kind(self, method_info):
    """Categorize the type of the request.

    Args:
      method_info: _MethodInfo, method information.

    Returns:
      The kind of request.
    """
    if method_info.http_method in ('GET', 'DELETE'):
      return self.__NO_BODY
    else:
      return self.__HAS_BODY

  def __field_to_subfields(self, field):
    """Fully describes data represented by field, including the nested case.

    In the case that the field is not a message field, we have no fields nested
    within a message definition, so we can simply return that field. However, in
    the nested case, we can't simply describe the data with one field or even
    with one chain of fields.

    For example, if we have a message field

      m_field = messages.MessageField(RefClass, 1)

    which references a class with two fields:

      class RefClass(messages.Message):
        one = messages.StringField(1)
        two = messages.IntegerField(2)

    then we would need to include both one and two to represent all the
    data contained.

    Calling __field_to_subfields(m_field) would return:
    [
      [<MessageField "m_field">, <StringField "one">],
      [<MessageField "m_field">, <StringField "two">],
    ]

    If the second field was instead a message field

      class RefClass(messages.Message):
        one = messages.StringField(1)
        two = messages.MessageField(OtherRefClass, 2)

    referencing another class with two fields

      class OtherRefClass(messages.Message):
        three = messages.BooleanField(1)
        four = messages.FloatField(2)

    then we would need to recurse one level deeper for two.

    With this change, calling __field_to_subfields(m_field) would return:
    [
      [<MessageField "m_field">, <StringField "one">],
      [<MessageField "m_field">, <StringField "two">, <StringField "three">],
      [<MessageField "m_field">, <StringField "two">, <StringField "four">],
    ]

    Args:
      field: An instance of a subclass of messages.Field.

    Returns:
      A list of lists, where each sublist is a list of fields.
    """
    # Termination condition
    if not isinstance(field, messages.MessageField):
      return [[field]]

    result = []
    for subfield in sorted(field.message_type.all_fields(),
                           key=lambda f: f.number):
      subfield_results = self.__field_to_subfields(subfield)
      for subfields_list in subfield_results:
        subfields_list.insert(0, field)
        result.append(subfields_list)
    return result

  def __field_to_parameter_type_and_format(self, field):
    """Converts the field variant type into a tuple describing the parameter.

    Args:
      field: An instance of a subclass of messages.Field.

    Returns:
      A tuple with the type and format of the field, respectively.

    Raises:
      TypeError: if the field variant is a message variant.
    """
    # We use lowercase values for types (e.g. 'string' instead of 'STRING').
    variant = field.variant
    if variant == messages.Variant.MESSAGE:
      raise TypeError('A message variant cannot be used in a parameter.')

    # Note that the 64-bit integers are marked as strings -- this is to
    # accommodate JavaScript, which would otherwise demote them to 32-bit
    # integers.

    return CUSTOM_VARIANT_MAP.get(variant) or (variant.name.lower(), None)

  def __get_path_parameters(self, path):
    """Parses path paremeters from a URI path and organizes them by parameter.

    Some of the parameters may correspond to message fields, and so will be
    represented as segments corresponding to each subfield; e.g. first.second if
    the field "second" in the message field "first" is pulled from the path.

    The resulting dictionary uses the first segments as keys and each key has as
    value the list of full parameter values with first segment equal to the key.

    If the match path parameter is null, that part of the path template is
    ignored; this occurs if '{}' is used in a template.

    Args:
      path: String; a URI path, potentially with some parameters.

    Returns:
      A dictionary with strings as keys and list of strings as values.
    """
    path_parameters_by_segment = {}
    for format_var_name in re.findall(_PATH_VARIABLE_PATTERN, path):
      first_segment = format_var_name.split('.', 1)[0]
      matches = path_parameters_by_segment.setdefault(first_segment, [])
      matches.append(format_var_name)

    return path_parameters_by_segment

  def __validate_simple_subfield(self, parameter, field, segment_list,
                                 segment_index=0):
    """Verifies that a proposed subfield actually exists and is a simple field.

    Here, simple means it is not a MessageField (nested).

    Args:
      parameter: String; the '.' delimited name of the current field being
          considered. This is relative to some root.
      field: An instance of a subclass of messages.Field. Corresponds to the
          previous segment in the path (previous relative to _segment_index),
          since this field should be a message field with the current segment
          as a field in the message class.
      segment_list: The full list of segments from the '.' delimited subfield
          being validated.
      segment_index: Integer; used to hold the position of current segment so
          that segment_list can be passed as a reference instead of having to
          copy using segment_list[1:] at each step.

    Raises:
      TypeError: If the final subfield (indicated by _segment_index relative
        to the length of segment_list) is a MessageField.
      TypeError: If at any stage the lookup at a segment fails, e.g if a.b
        exists but a.b.c does not exist. This can happen either if a.b is not
        a message field or if a.b.c is not a property on the message class from
        a.b.
    """
    if segment_index >= len(segment_list):
      # In this case, the field is the final one, so should be simple type
      if isinstance(field, messages.MessageField):
        field_class = field.__class__.__name__
        raise TypeError('Can\'t use messages in path. Subfield %r was '
                        'included but is a %s.' % (parameter, field_class))
      return

    segment = segment_list[segment_index]
    parameter += '.' + segment
    try:
      field = field.type.field_by_name(segment)
    except (AttributeError, KeyError):
      raise TypeError('Subfield %r from path does not exist.' % (parameter,))

    self.__validate_simple_subfield(parameter, field, segment_list,
                                    segment_index=segment_index + 1)

  def __validate_path_parameters(self, field, path_parameters):
    """Verifies that all path parameters correspond to an existing subfield.

    Args:
      field: An instance of a subclass of messages.Field. Should be the root
          level property name in each path parameter in path_parameters. For
          example, if the field is called 'foo', then each path parameter should
          begin with 'foo.'.
      path_parameters: A list of Strings representing URI parameter variables.

    Raises:
      TypeError: If one of the path parameters does not start with field.name.
    """
    for param in path_parameters:
      segment_list = param.split('.')
      if segment_list[0] != field.name:
        raise TypeError('Subfield %r can\'t come from field %r.'
                        % (param, field.name))
      self.__validate_simple_subfield(field.name, field, segment_list[1:])

  def __parameter_default(self, field):
    """Returns default value of field if it has one.

    Args:
      field: A simple field.

    Returns:
      The default value of the field, if any exists, with the exception of an
          enum field, which will have its value cast to a string.
    """
    if field.default:
      if isinstance(field, messages.EnumField):
        return field.default.name
      else:
        return field.default

  def __parameter_enum(self, param):
    """Returns enum descriptor of a parameter if it is an enum.

    An enum descriptor is a list of keys.

    Args:
      param: A simple field.

    Returns:
      The enum descriptor for the field, if it's an enum descriptor, else
          returns None.
    """
    if isinstance(param, messages.EnumField):
      return [enum_entry[0] for enum_entry in sorted(
          param.type.to_dict().items(), key=lambda v: v[1])]

  def __parameter_descriptor(self, param):
    """Creates descriptor for a parameter.

    Args:
      param: The parameter to be described.

    Returns:
      Dictionary containing a descriptor for the parameter.
    """
    descriptor = {}

    param_type, param_format = self.__field_to_parameter_type_and_format(param)

    # Required
    if param.required:
      descriptor['required'] = True

    # Type
    descriptor['type'] = param_type

    # Format (optional)
    if param_format:
      descriptor['format'] = param_format

    # Default
    default = self.__parameter_default(param)
    if default is not None:
      descriptor['default'] = default

    # Repeated
    if param.repeated:
      descriptor['repeated'] = True

    # Enum
    # Note that enumDescriptions are not currently supported using the
    # framework's annotations, so just insert blank strings.
    enum_descriptor = self.__parameter_enum(param)
    if enum_descriptor is not None:
      descriptor['enum'] = enum_descriptor
      descriptor['enumDescriptions'] = [''] * len(enum_descriptor)

    return descriptor

  def __add_parameter(self, param, path_parameters, params):
    """Adds all parameters in a field to a method parameters descriptor.

    Simple fields will only have one parameter, but a message field 'x' that
    corresponds to a message class with fields 'y' and 'z' will result in
    parameters 'x.y' and 'x.z', for example. The mapping from field to
    parameters is mostly handled by __field_to_subfields.

    Args:
      param: Parameter to be added to the descriptor.
      path_parameters: A list of parameters matched from a path for this field.
         For example for the hypothetical 'x' from above if the path was
         '/a/{x.z}/b/{other}' then this list would contain only the element
         'x.z' since 'other' does not match to this field.
      params: List of parameters. Each parameter in the field.
    """
    # If this is a simple field, just build the descriptor and append it.
    # Otherwise, build a schema and assign it to this descriptor
    descriptor = None
    if not isinstance(param, messages.MessageField):
      name = param.name
      descriptor = self.__parameter_descriptor(param)
      descriptor['location'] = 'path' if name in path_parameters else 'query'

      if descriptor:
        params[name] = descriptor
    else:
      for subfield_list in self.__field_to_subfields(param):
        name = '.'.join(subfield.name for subfield in subfield_list)
        descriptor = self.__parameter_descriptor(subfield_list[-1])
        if name in path_parameters:
          descriptor['required'] = True
          descriptor['location'] = 'path'
        else:
          descriptor.pop('required', None)
          descriptor['location'] = 'query'

        if descriptor:
          params[name] = descriptor


  def __params_descriptor_without_container(self, message_type,
                                            request_kind, path):
    """Describe parameters of a method which does not use a ResourceContainer.

    Makes sure that the path parameters are included in the message definition
    and adds any required fields and URL query parameters.

    This method is to preserve backwards compatibility and will be removed in
    a future release.

    Args:
      message_type: messages.Message class, Message with parameters to describe.
      request_kind: The type of request being made.
      path: string, HTTP path to method.

    Returns:
      A list of dicts: Descriptors of the parameters
    """
    params = {}

    path_parameter_dict = self.__get_path_parameters(path)
    for field in sorted(message_type.all_fields(), key=lambda f: f.number):
      matched_path_parameters = path_parameter_dict.get(field.name, [])
      self.__validate_path_parameters(field, matched_path_parameters)
      if matched_path_parameters or request_kind == self.__NO_BODY:
        self.__add_parameter(field, matched_path_parameters, params)

    return params

  def __params_descriptor(self, message_type, request_kind, path, method_id,
                          request_params_class):
    """Describe the parameters of a method.

    If the message_type is not a ResourceContainer, will fall back to
    __params_descriptor_without_container (which will eventually be deprecated).

    If the message type is a ResourceContainer, then all path/query parameters
    will come from the ResourceContainer. This method will also make sure all
    path parameters are covered by the message fields.

    Args:
      message_type: messages.Message or ResourceContainer class, Message with
        parameters to describe.
      request_kind: The type of request being made.
      path: string, HTTP path to method.
      method_id: string, Unique method identifier (e.g. 'myapi.items.method')
      request_params_class: messages.Message, the original params message when
        using a ResourceContainer. Otherwise, this should be null.

    Returns:
      A tuple (dict, list of string): Descriptor of the parameters, Order of the
        parameters.
    """
    path_parameter_dict = self.__get_path_parameters(path)

    if request_params_class is None:
      if path_parameter_dict:
        logging.warning('Method %s specifies path parameters but you are not '
                        'using a ResourceContainer. This will fail in future '
                        'releases; please switch to using ResourceContainer as '
                        'soon as possible.', method_id)
      return self.__params_descriptor_without_container(
          message_type, request_kind, path)

    # From here, we can assume message_type is from a ResourceContainer.
    message_type = request_params_class

    params = {}

    # Make sure all path parameters are covered.
    for field_name, matched_path_parameters in path_parameter_dict.iteritems():
      field = message_type.field_by_name(field_name)
      self.__validate_path_parameters(field, matched_path_parameters)

    # Add all fields, sort by field.number since we have parameterOrder.
    for field in sorted(message_type.all_fields(), key=lambda f: f.number):
      matched_path_parameters = path_parameter_dict.get(field.name, [])
      self.__add_parameter(field, matched_path_parameters, params)

    return params

  def __params_order_descriptor(self, message_type, path):
    """Describe the order of path parameters.

    Args:
      message_type: messages.Message class, Message with parameters to describe.
      path: string, HTTP path to method.

    Returns:
      Descriptor list for the parameter order.
    """
    descriptor = []
    path_parameter_dict = self.__get_path_parameters(path)

    for field in sorted(message_type.all_fields(), key=lambda f: f.number):
      matched_path_parameters = path_parameter_dict.get(field.name, [])
      if not isinstance(field, messages.MessageField):
        name = field.name
        if name in matched_path_parameters:
          descriptor.append(name)
      else:
        for subfield_list in self.__field_to_subfields(field):
          name = '.'.join(subfield.name for subfield in subfield_list)
          if name in matched_path_parameters:
            descriptor.append(name)

    return descriptor

  def __schemas_descriptor(self):
    """Describes the schemas section of the discovery document.

    Returns:
      Dictionary describing the schemas of the document.
    """
    # Filter out any keys that aren't 'properties', 'type', or 'id'
    result = {}
    for schema_key, schema_value in self.__parser.schemas().iteritems():
      field_keys = schema_value.keys()
      key_result = {}

      # Some special processing for the properties value
      if 'properties' in field_keys:
        key_result['properties'] = schema_value['properties'].copy()
        # Add in enumDescriptions for any enum properties and strip out
        # the required tag for consistency with Java framework
        for prop_key, prop_value in schema_value['properties'].iteritems():
          if 'enum' in prop_value:
            num_enums = len(prop_value['enum'])
            key_result['properties'][prop_key]['enumDescriptions'] = (
                [''] * num_enums)
          key_result['properties'][prop_key].pop('required', None)

      for key in ('type', 'id', 'description'):
        if key in field_keys:
          key_result[key] = schema_value[key]

      if key_result:
        result[schema_key] = key_result

    # Add 'type': 'object' to all object properties
    for schema_value in result.itervalues():
      for field_value in schema_value.itervalues():
        if isinstance(field_value, dict):
          if '$ref' in field_value:
            field_value['type'] = 'object'

    return result

  def __request_message_descriptor(self, request_kind, message_type, method_id,
                                   request_body_class):
    """Describes the parameters and body of the request.

    Args:
      request_kind: The type of request being made.
      message_type: messages.Message or ResourceContainer class. The message to
          describe.
      method_id: string, Unique method identifier (e.g. 'myapi.items.method')
      request_body_class: messages.Message of the original body when using
          a ResourceContainer. Otherwise, this should be null.

    Returns:
      Dictionary describing the request.

    Raises:
      ValueError: if the method path and request required fields do not match
    """
    if request_body_class:
      message_type = request_body_class

    if (request_kind != self.__NO_BODY and
        message_type != message_types.VoidMessage()):
      self.__request_schema[method_id] = self.__parser.add_message(
          message_type.__class__)
      return {
          '$ref': self.__request_schema[method_id],
          'parameterName': 'resource',
      }

  def __response_message_descriptor(self, message_type, method_id):
    """Describes the response.

    Args:
      message_type: messages.Message class, The message to describe.
      method_id: string, Unique method identifier (e.g. 'myapi.items.method')

    Returns:
      Dictionary describing the response.
    """
    if message_type != message_types.VoidMessage():
      self.__parser.add_message(message_type.__class__)
      self.__response_schema[method_id] = self.__parser.ref_for_message_type(
          message_type.__class__)
      return {'$ref': self.__response_schema[method_id]}
    else:
      return None

  def __method_descriptor(self, service, method_info,
                          protorpc_method_info):
    """Describes a method.

    Args:
      service: endpoints.Service, Implementation of the API as a service.
      method_info: _MethodInfo, Configuration for the method.
      protorpc_method_info: protorpc.remote._RemoteMethodInfo, ProtoRPC
        description of the method.

    Returns:
      Dictionary describing the method.
    """
    descriptor = {}

    request_message_type = (resource_container.ResourceContainer.
                            get_request_message(protorpc_method_info.remote))
    request_kind = self.__get_request_kind(method_info)
    remote_method = protorpc_method_info.remote

    method_id = method_info.method_id(service.api_info)

    path = method_info.get_path(service.api_info)

    description = protorpc_method_info.remote.method.__doc__

    descriptor['id'] = method_id
    descriptor['path'] = path
    descriptor['httpMethod'] = method_info.http_method

    if description:
      descriptor['description'] = description

    descriptor['scopes'] = [
        'https://www.googleapis.com/auth/userinfo.email'
    ]

    parameters = self.__params_descriptor(
        request_message_type, request_kind, path, method_id,
        method_info.request_params_class)
    if parameters:
      descriptor['parameters'] = parameters

    if method_info.request_params_class:
      parameter_order = self.__params_order_descriptor(
        method_info.request_params_class, path)
    else:
      parameter_order = self.__params_order_descriptor(
        request_message_type, path)
    if parameter_order:
      descriptor['parameterOrder'] = parameter_order

    request_descriptor = self.__request_message_descriptor(
        request_kind, request_message_type, method_id,
        method_info.request_body_class)
    if request_descriptor is not None:
      descriptor['request'] = request_descriptor

    response_descriptor = self.__response_message_descriptor(
        remote_method.response_type(), method_info.method_id(service.api_info))
    if response_descriptor is not None:
      descriptor['response'] = response_descriptor

    return descriptor

  def __resource_descriptor(self, resource_path, methods):
    """Describes a resource.

    Args:
      resource_path: string, the path of the resource (e.g., 'entries.items')
      methods: list of tuples of type
        (endpoints.Service, protorpc.remote._RemoteMethodInfo), the methods
        that serve this resource.

    Returns:
      Dictionary describing the resource.
    """
    descriptor = {}
    method_map = {}
    sub_resource_index = collections.defaultdict(list)
    sub_resource_map = {}

    resource_path_tokens = resource_path.split('.')
    for service, protorpc_meth_info in methods:
      method_info = getattr(protorpc_meth_info, 'method_info', None)
      path = method_info.get_path(service.api_info)
      method_id = method_info.method_id(service.api_info)
      canonical_method_id = self._get_canonical_method_id(method_id)

      current_resource_path = self._get_resource_path(method_id)

      # Sanity-check that this method belongs to the resource path
      if (current_resource_path[:len(resource_path_tokens)] !=
          resource_path_tokens):
        raise api_exceptions.ToolError(
            'Internal consistency error in resource path {0}'.format(
                current_resource_path))

      # Remove the portion of the current method's resource path that's already
      # part of the resource path at this level.
      effective_resource_path = current_resource_path[
          len(resource_path_tokens):]

      # If this method is part of a sub-resource, note it and skip it for now
      if effective_resource_path:
        sub_resource_name = effective_resource_path[0]
        new_resource_path = '.'.join([resource_path, sub_resource_name])
        sub_resource_index[new_resource_path].append(
            (service, protorpc_meth_info))
      else:
        method_map[canonical_method_id] = self.__method_descriptor(
            service, method_info, protorpc_meth_info)

    # Process any sub-resources
    for sub_resource, sub_resource_methods in sub_resource_index.items():
      sub_resource_name = sub_resource.split('.')[-1]
      sub_resource_map[sub_resource_name] = self.__resource_descriptor(
          sub_resource, sub_resource_methods)

    if method_map:
      descriptor['methods'] = method_map

    if sub_resource_map:
      descriptor['resources'] = sub_resource_map

    return descriptor

  def __standard_parameters_descriptor(self):
    return {
        'alt': {
            'type': 'string',
            'description': 'Data format for the response.',
            'default': 'json',
            'enum': ['json'],
            'enumDescriptions': [
                'Responses with Content-Type of application/json'
            ],
            'location': 'query',
        },
        'fields': {
          'type': 'string',
          'description': 'Selector specifying which fields to include in a '
                         'partial response.',
          'location': 'query',
        },
        'key': {
            'type': 'string',
            'description': 'API key. Your API key identifies your project and '
                           'provides you with API access, quota, and reports. '
                           'Required unless you provide an OAuth 2.0 token.',
            'location': 'query',
        },
        'oauth_token': {
            'type': 'string',
            'description': 'OAuth 2.0 token for the current user.',
            'location': 'query',
        },
        'prettyPrint': {
            'type': 'boolean',
            'description': 'Returns response with indentations and line '
                           'breaks.',
            'default': 'true',
            'location': 'query',
        },
        'quotaUser': {
            'type': 'string',
            'description': 'Available to use for quota purposes for '
                           'server-side applications. Can be any arbitrary '
                           'string assigned to a user, but should not exceed '
                           '40 characters. Overrides userIp if both are '
                           'provided.',
            'location': 'query',
        },
        'userIp': {
            'type': 'string',
            'description': 'IP address of the site where the request '
                           'originates. Use this if you want to enforce '
                           'per-user limits.',
            'location': 'query',
        },
    }

  def __standard_auth_descriptor(self):
    return {
        'oauth2': {
            'scopes': {
                'https://www.googleapis.com/auth/userinfo.email': {
                    'description': 'View your email address'
                }
            }
        }
    }

  def __get_merged_api_info(self, services):
    """Builds a description of an API.

    Args:
      services: List of protorpc.remote.Service instances implementing an
        api/version.

    Returns:
      The _ApiInfo object to use for the API that the given services implement.

    Raises:
      ApiConfigurationError: If there's something wrong with the API
        configuration, such as a multiclass API decorated with different API
        descriptors (see the docstring for api()).
    """
    merged_api_info = services[0].api_info

    # Verify that, if there are multiple classes here, they're allowed to
    # implement the same API.
    for service in services[1:]:
      if not merged_api_info.is_same_api(service.api_info):
        raise api_exceptions.ApiConfigurationError(
            _MULTICLASS_MISMATCH_ERROR_TEMPLATE % (service.api_info.name,
                                                   service.api_info.version))

    return merged_api_info

  def __discovery_doc_descriptor(self, services, hostname=None):
    """Builds a discovery doc for an API.

    Args:
      services: List of protorpc.remote.Service instances implementing an
        api/version.
      hostname: string, Hostname of the API, to override the value set on the
        current service. Defaults to None.

    Returns:
      A dictionary that can be deserialized into JSON in discovery doc format.

    Raises:
      ApiConfigurationError: If there's something wrong with the API
        configuration, such as a multiclass API decorated with different API
        descriptors (see the docstring for api()), or a repeated method
        signature.
    """
    merged_api_info = self.__get_merged_api_info(services)
    descriptor = self.get_descriptor_defaults(merged_api_info,
                                              hostname=hostname)

    description = merged_api_info.description
    if not description and len(services) == 1:
      description = services[0].__doc__
    if description:
      descriptor['description'] = description

    descriptor['parameters'] = self.__standard_parameters_descriptor()
    descriptor['auth'] = self.__standard_auth_descriptor()

    method_map = {}
    method_collision_tracker = {}
    rest_collision_tracker = {}

    resource_index = collections.defaultdict(list)
    resource_map = {}

    # For the first pass, only process top-level methods (that is, those methods
    # that are unattached to a resource).
    for service in services:
      remote_methods = service.all_remote_methods()

      for protorpc_meth_name, protorpc_meth_info in remote_methods.iteritems():
        method_info = getattr(protorpc_meth_info, 'method_info', None)
        # Skip methods that are not decorated with @method
        if method_info is None:
          continue
        path = method_info.get_path(service.api_info)
        method_id = method_info.method_id(service.api_info)
        canonical_method_id = self._get_canonical_method_id(method_id)
        resource_path = self._get_resource_path(method_id)

        # Make sure the same method name isn't repeated.
        if method_id in method_collision_tracker:
          raise api_exceptions.ApiConfigurationError(
              'Method %s used multiple times, in classes %s and %s' %
              (method_id, method_collision_tracker[method_id],
               service.__name__))
        else:
          method_collision_tracker[method_id] = service.__name__

        # Make sure the same HTTP method & path aren't repeated.
        rest_identifier = (method_info.http_method, path)
        if rest_identifier in rest_collision_tracker:
          raise api_exceptions.ApiConfigurationError(
              '%s path "%s" used multiple times, in classes %s and %s' %
              (method_info.http_method, path,
               rest_collision_tracker[rest_identifier],
               service.__name__))
        else:
          rest_collision_tracker[rest_identifier] = service.__name__

        # If this method is part of a resource, note it and skip it for now
        if resource_path:
          resource_index[resource_path[0]].append((service, protorpc_meth_info))
        else:
          method_map[canonical_method_id] = self.__method_descriptor(
              service, method_info, protorpc_meth_info)

    # Do another pass for methods attached to resources
    for resource, resource_methods in resource_index.items():
      resource_map[resource] = self.__resource_descriptor(resource,
          resource_methods)

    if method_map:
      descriptor['methods'] = method_map

    if resource_map:
      descriptor['resources'] = resource_map

    # Add schemas, if any
    schemas = self.__schemas_descriptor()
    if schemas:
      descriptor['schemas'] = schemas

    return descriptor

  def get_descriptor_defaults(self, api_info, hostname=None):
    """Gets a default configuration for a service.

    Args:
      api_info: _ApiInfo object for this service.
      hostname: string, Hostname of the API, to override the value set on the
        current service. Defaults to None.

    Returns:
      A dictionary with the default configuration.
    """
    hostname = (hostname or util.get_app_hostname() or
                api_info.hostname)
    protocol = 'http' if ((hostname and hostname.startswith('localhost')) or
                          util.is_running_on_devserver()) else 'https'
    full_base_path = '{0}{1}/{2}/'.format(api_info.base_path,
                                          api_info.name,
                                          api_info.version)
    base_url = '{0}://{1}{2}'.format(protocol, hostname, full_base_path)
    root_url = '{0}://{1}{2}'.format(protocol, hostname, api_info.base_path)
    defaults = {
        'kind': 'discovery#restDescription',
        'discoveryVersion': 'v1',
        'id': '{0}:{1}'.format(api_info.name, api_info.version),
        'name': api_info.name,
        'version': api_info.version,
        'icons': {
            'x16': 'http://www.google.com/images/icons/product/search-16.gif',
            'x32': 'http://www.google.com/images/icons/product/search-32.gif'
        },
        'protocol': 'rest',
        'servicePath': '{0}/{1}/'.format(api_info.name, api_info.version),
        'batchPath': 'batch',
        'basePath': full_base_path,
        'rootUrl': root_url,
        'baseUrl': base_url,
    }

    return defaults

  def get_discovery_doc(self, services, hostname=None):
    """JSON dict description of a protorpc.remote.Service in discovery format.

    Args:
      services: Either a single protorpc.remote.Service or a list of them
        that implements an api/version.
      hostname: string, Hostname of the API, to override the value set on the
        current service. Defaults to None.

    Returns:
      dict, The discovery document as a JSON dict.
    """

    if not isinstance(services, (tuple, list)):
      services = [services]

    # The type of a class that inherits from remote.Service is actually
    # remote._ServiceClass, thanks to metaclass strangeness.
    # pylint: disable=protected-access
    util.check_list_type(services, remote._ServiceClass, 'services',
                         allow_none=False)

    return self.__discovery_doc_descriptor(services, hostname=hostname)

  def pretty_print_config_to_json(self, services, hostname=None):
    """JSON string description of a protorpc.remote.Service in a discovery doc.

    Args:
      services: Either a single protorpc.remote.Service or a list of them
        that implements an api/version.
      hostname: string, Hostname of the API, to override the value set on the
        current service. Defaults to None.

    Returns:
      string, The discovery doc descriptor document as a JSON string.
    """
    descriptor = self.get_discovery_doc(services, hostname)
    return json.dumps(descriptor, sort_keys=True, indent=2,
                      separators=(',', ': '))
