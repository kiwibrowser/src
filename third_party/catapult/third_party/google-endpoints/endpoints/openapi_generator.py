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

"""A library for converting service configs to OpenAPI (Swagger) specs."""

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
    'classes that aren\'t compatible. See docstring for api() for '
    'examples how to implement a multi-class API.')

_INVALID_AUTH_ISSUER = 'No auth issuer named %s defined in this Endpoints API.'

_API_KEY = 'api_key'
_API_KEY_PARAM = 'key'


class OpenApiGenerator(object):
  """Generates an OpenAPI spec from a ProtoRPC service.

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

    api_config = OpenApiGenerator().pretty_print_config_to_json(HelloService)

  The resulting api_config will be a JSON OpenAPI document describing the API
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

  def _add_def_paths(self, prop_dict):
    """Recursive method to add relative paths for any $ref objects.

    Args:
      prop_dict: The property dict to alter.

    Side Effects:
      Alters prop_dict in-place.
    """
    for prop_key, prop_value in prop_dict.iteritems():
      if prop_key == '$ref' and not 'prop_value'.startswith('#'):
        prop_dict[prop_key] = '#/definitions/' + prop_dict[prop_key]
      elif isinstance(prop_value, dict):
        self._add_def_paths(prop_value)

  def _construct_operation_id(self, service_name, protorpc_method_name):
    """Return an operation id for a service method.

    Args:
      service_name: The name of the service.
      protorpc_method_name: The ProtoRPC method name.

    Returns:
      A string representing the operation id.
    """

    # camelCase the ProtoRPC method name
    method_name_camel = util.snake_case_to_headless_camel_case(
        protorpc_method_name)

    return '{0}_{1}'.format(service_name, method_name_camel)

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
      raise TypeError('A message variant can\'t be used in a parameter.')

    # Note that the 64-bit integers are marked as strings -- this is to
    # accommodate JavaScript, which would otherwise demote them to 32-bit
    # integers.

    custom_variant_map = {
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
    return custom_variant_map.get(variant) or (variant.name.lower(), None)

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

    descriptor['name'] = param.name

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
    enum_descriptor = self.__parameter_enum(param)
    if enum_descriptor is not None:
      descriptor['enum'] = enum_descriptor

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
      descriptor = self.__parameter_descriptor(param)
      descriptor['in'] = 'path' if param.name in path_parameters else 'query'
    else:
      # If a subfield of a MessageField is found in the path, build a descriptor
      # for the path parameter.
      for subfield_list in self.__field_to_subfields(param):
        qualified_name = '.'.join(subfield.name for subfield in subfield_list)
        if qualified_name in path_parameters:
          descriptor = self.__parameter_descriptor(subfield_list[-1])
          descriptor['required'] = True
          descriptor['in'] = 'path'

    if descriptor:
      params.append(descriptor)

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
    params = []

    path_parameter_dict = self.__get_path_parameters(path)
    for field in sorted(message_type.all_fields(), key=lambda f: f.number):
      matched_path_parameters = path_parameter_dict.get(field.name, [])
      self.__validate_path_parameters(field, matched_path_parameters)
      if matched_path_parameters or request_kind == self.__NO_BODY:
        self.__add_parameter(field, matched_path_parameters, params)

    return params

  def __params_descriptor(self, message_type, request_kind, path, method_id):
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

    Returns:
      A tuple (dict, list of string): Descriptor of the parameters, Order of the
        parameters.
    """
    path_parameter_dict = self.__get_path_parameters(path)

    if not isinstance(message_type, resource_container.ResourceContainer):
      if path_parameter_dict:
        logging.warning('Method %s specifies path parameters but you are not '
                        'using a ResourceContainer. This will fail in future '
                        'releases; please switch to using ResourceContainer as '
                        'soon as possible.', method_id)
      return self.__params_descriptor_without_container(
          message_type, request_kind, path)

    # From here, we can assume message_type is a ResourceContainer.
    message_type = message_type.parameters_message_class()

    params = []

    # Make sure all path parameters are covered.
    for field_name, matched_path_parameters in path_parameter_dict.iteritems():
      field = message_type.field_by_name(field_name)
      self.__validate_path_parameters(field, matched_path_parameters)

    # Add all fields, sort by field.number since we have parameterOrder.
    for field in sorted(message_type.all_fields(), key=lambda f: f.number):
      matched_path_parameters = path_parameter_dict.get(field.name, [])
      self.__add_parameter(field, matched_path_parameters, params)

    return params

  def __request_message_descriptor(self, request_kind, message_type, method_id,
                                   path):
    """Describes the parameters and body of the request.

    Args:
      request_kind: The type of request being made.
      message_type: messages.Message or ResourceContainer class. The message to
          describe.
      method_id: string, Unique method identifier (e.g. 'myapi.items.method')
      path: string, HTTP path to method.

    Returns:
      Dictionary describing the request.

    Raises:
      ValueError: if the method path and request required fields do not match
    """
    params = self.__params_descriptor(message_type, request_kind, path,
                                      method_id)

    if isinstance(message_type, resource_container.ResourceContainer):
      message_type = message_type.body_message_class()

    if (request_kind != self.__NO_BODY and
        message_type != message_types.VoidMessage()):
      self.__request_schema[method_id] = self.__parser.add_message(
          message_type.__class__)

    return params

  def __definitions_descriptor(self):
    """Describes the definitions section of the OpenAPI spec.

    Returns:
      Dictionary describing the definitions of the spec.
    """
    # Filter out any keys that aren't 'properties' or 'type'
    result = {}
    for def_key, def_value in self.__parser.schemas().iteritems():
      if 'properties' in def_value or 'type' in def_value:
        key_result = {}
        required_keys = set()
        if 'type' in def_value:
          key_result['type'] = def_value['type']
        if 'properties' in def_value:
          for prop_key, prop_value in def_value['properties'].items():
            if isinstance(prop_value, dict) and 'required' in prop_value:
              required_keys.add(prop_key)
              del prop_value['required']
          key_result['properties'] = def_value['properties']
        # Add in the required fields, if any
        if required_keys:
          key_result['required'] = sorted(required_keys)
        result[def_key] = key_result

    # Add 'type': 'object' to all object properties
    # Also, recursively add relative path to all $ref values
    for def_value in result.itervalues():
      for prop_value in def_value.itervalues():
        if isinstance(prop_value, dict):
          if '$ref' in prop_value:
            prop_value['type'] = 'object'
          self._add_def_paths(prop_value)

    return result

  def __response_message_descriptor(self, message_type, method_id):
    """Describes the response.

    Args:
      message_type: messages.Message class, The message to describe.
      method_id: string, Unique method identifier (e.g. 'myapi.items.method')

    Returns:
      Dictionary describing the response.
    """

    # Skeleton response descriptor, common to all response objects
    descriptor = {'200': {'description': 'A successful response'}}

    if message_type != message_types.VoidMessage():
      self.__parser.add_message(message_type.__class__)
      self.__response_schema[method_id] = self.__parser.ref_for_message_type(
          message_type.__class__)
      descriptor['200']['schema'] = {'$ref': '#/definitions/{0}'.format(
          self.__response_schema[method_id])}

    return dict(descriptor)

  def __method_descriptor(self, service, method_info, operation_id,
                          protorpc_method_info, security_definitions):
    """Describes a method.

    Args:
      service: endpoints.Service, Implementation of the API as a service.
      method_info: _MethodInfo, Configuration for the method.
      operation_id: string, Operation ID of the method
      protorpc_method_info: protorpc.remote._RemoteMethodInfo, ProtoRPC
        description of the method.
      security_definitions: list of dicts, security definitions for the API.

    Returns:
      Dictionary describing the method.
    """
    descriptor = {}

    request_message_type = (resource_container.ResourceContainer.
                            get_request_message(protorpc_method_info.remote))
    request_kind = self.__get_request_kind(method_info)
    remote_method = protorpc_method_info.remote

    path = method_info.get_path(service.api_info)

    descriptor['parameters'] = self.__request_message_descriptor(
        request_kind, request_message_type,
        method_info.method_id(service.api_info),
        path)
    descriptor['responses'] = self.__response_message_descriptor(
        remote_method.response_type(), method_info.method_id(service.api_info))
    descriptor['operationId'] = operation_id

    # Insert the auth audiences, if any
    api_key_required = method_info.is_api_key_required(service.api_info)
    if method_info.audiences is not None:
      descriptor['x-security'] = self.__x_security_descriptor(
          method_info.audiences, security_definitions)
      descriptor['security'] = self.__security_descriptor(
          method_info.audiences, security_definitions,
          api_key_required=api_key_required)
    elif service.api_info.audiences is not None or api_key_required:
      if service.api_info.audiences:
        descriptor['x-security'] = self.__x_security_descriptor(
            service.api_info.audiences, security_definitions)
      descriptor['security'] = self.__security_descriptor(
          service.api_info.audiences, security_definitions,
          api_key_required=api_key_required)

    return descriptor

  def __security_descriptor(self, audiences, security_definitions,
                            api_key_required=False):
    if not audiences and not api_key_required:
      return []

    result_dict = {
        issuer_name: [] for issuer_name in security_definitions.keys()
    }

    if api_key_required:
      result_dict[_API_KEY] = []
      # Remove the unnecessary implicit google_id_token issuer
      result_dict.pop('google_id_token', None)
    else:
      # If the API key is not required, remove the issuer for it
      result_dict.pop('api_key', None)

    return [result_dict]

  def __x_security_descriptor(self, audiences, security_definitions):
    default_auth_issuer = 'google_id_token'
    if isinstance(audiences, list):
      if default_auth_issuer not in security_definitions:
        raise api_exceptions.ApiConfigurationError(
            _INVALID_AUTH_ISSUER % default_auth_issuer)
      return [
          {
              default_auth_issuer: {
                  'audiences': audiences,
              }
          }
      ]
    elif isinstance(audiences, dict):
      descriptor = list()
      for audience_key, audience_value in audiences.items():
        if audience_key not in security_definitions:
          raise api_exceptions.ApiConfigurationError(
              _INVALID_AUTH_ISSUER % audience_key)
        descriptor.append({audience_key: {'audiences': audience_value}})
      return descriptor

  def __security_definitions_descriptor(self, issuers):
    """Create a descriptor for the security definitions.

    Args:
      issuers: dict, mapping issuer names to Issuer tuples

    Returns:
      The dict representing the security definitions descriptor.
    """
    if not issuers:
      return {
          'google_id_token': {
              'authorizationUrl': '',
              'flow': 'implicit',
              'type': 'oauth2',
              'x-issuer': 'accounts.google.com',
              'x-jwks_uri': 'https://www.googleapis.com/oauth2/v1/certs',
          }
      }

    result = {}

    for issuer_key, issuer_value in issuers.items():
      result[issuer_key] = {
          'authorizationUrl': '',
          'flow': 'implicit',
          'type': 'oauth2',
          'x-issuer': issuer_value.issuer,
      }

      # If jwks_uri is omitted, the auth library will use OpenID discovery
      # to find it. Otherwise, include it in the descriptor explicitly.
      if issuer_value.jwks_uri:
        result[issuer_key]['x-jwks_uri'] = issuer_value.jwks_uri

    return result

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

  def __api_openapi_descriptor(self, services, hostname=None):
    """Builds an OpenAPI description of an API.

    Args:
      services: List of protorpc.remote.Service instances implementing an
        api/version.
      hostname: string, Hostname of the API, to override the value set on the
        current service. Defaults to None.

    Returns:
      A dictionary that can be deserialized into JSON and stored as an API
      description document in OpenAPI format.

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
      descriptor['info']['description'] = description

    security_definitions = self.__security_definitions_descriptor(
        merged_api_info.issuers)

    method_map = {}
    method_collision_tracker = {}
    rest_collision_tracker = {}

    for service in services:
      remote_methods = service.all_remote_methods()

      for protorpc_meth_name, protorpc_meth_info in remote_methods.iteritems():
        method_info = getattr(protorpc_meth_info, 'method_info', None)
        # Skip methods that are not decorated with @method
        if method_info is None:
          continue
        method_id = method_info.method_id(service.api_info)
        is_api_key_required = method_info.is_api_key_required(service.api_info)
        path = '/{0}/{1}/{2}'.format(merged_api_info.name,
                                     merged_api_info.version,
                                     method_info.get_path(service.api_info))
        verb = method_info.http_method.lower()

        if path not in method_map:
          method_map[path] = {}

        # If an API key is required and the security definitions don't already
        # have the apiKey issuer, add the appropriate notation now
        if is_api_key_required and _API_KEY not in security_definitions:
          security_definitions[_API_KEY] = {
              'type': 'apiKey',
              'name': _API_KEY_PARAM,
              'in': 'query'
          }

        # Derive an OperationId from the method name data
        operation_id = self._construct_operation_id(
            service.__name__, protorpc_meth_name)

        method_map[path][verb] = self.__method_descriptor(
            service, method_info, operation_id, protorpc_meth_info,
            security_definitions)

        # Make sure the same method name isn't repeated.
        if method_id in method_collision_tracker:
          raise api_exceptions.ApiConfigurationError(
              'Method %s used multiple times, in classes %s and %s' %
              (method_id, method_collision_tracker[method_id],
               service.__name__))
        else:
          method_collision_tracker[method_id] = service.__name__

        # Make sure the same HTTP method & path aren't repeated.
        rest_identifier = (method_info.http_method,
                           method_info.get_path(service.api_info))
        if rest_identifier in rest_collision_tracker:
          raise api_exceptions.ApiConfigurationError(
              '%s path "%s" used multiple times, in classes %s and %s' %
              (method_info.http_method, method_info.get_path(service.api_info),
               rest_collision_tracker[rest_identifier],
               service.__name__))
        else:
          rest_collision_tracker[rest_identifier] = service.__name__

    if method_map:
      descriptor['paths'] = method_map

    # Add request and/or response definitions, if any
    definitions = self.__definitions_descriptor()
    if definitions:
      descriptor['definitions'] = definitions

    descriptor['securityDefinitions'] = security_definitions

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
    defaults = {
        'swagger': '2.0',
        'info': {
            'version': api_info.version,
            'title': api_info.name
        },
        'host': hostname,
        'consumes': ['application/json'],
        'produces': ['application/json'],
        'schemes': [protocol],
        'basePath': api_info.base_path.rstrip('/'),
    }

    return defaults

  def get_openapi_dict(self, services, hostname=None):
    """JSON dict description of a protorpc.remote.Service in OpenAPI format.

    Args:
      services: Either a single protorpc.remote.Service or a list of them
        that implements an api/version.
      hostname: string, Hostname of the API, to override the value set on the
        current service. Defaults to None.

    Returns:
      dict, The OpenAPI descriptor document as a JSON dict.
    """

    if not isinstance(services, (tuple, list)):
      services = [services]

    # The type of a class that inherits from remote.Service is actually
    # remote._ServiceClass, thanks to metaclass strangeness.
    # pylint: disable=protected-access
    util.check_list_type(services, remote._ServiceClass, 'services',
                         allow_none=False)

    return self.__api_openapi_descriptor(services, hostname=hostname)

  def pretty_print_config_to_json(self, services, hostname=None):
    """JSON string description of a protorpc.remote.Service in OpenAPI format.

    Args:
      services: Either a single protorpc.remote.Service or a list of them
        that implements an api/version.
      hostname: string, Hostname of the API, to override the value set on the
        current service. Defaults to None.

    Returns:
      string, The OpenAPI descriptor document as a JSON string.
    """
    descriptor = self.get_openapi_dict(services, hostname)
    return json.dumps(descriptor, sort_keys=True, indent=2,
                      separators=(',', ': '))
