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

"""Library for generating an API configuration document for a ProtoRPC backend.

The protorpc.remote.Service is inspected and a JSON document describing
the API is returned.

    class MyResponse(messages.Message):
      bool_value = messages.BooleanField(1)
      int32_value = messages.IntegerField(2)

    class MyService(remote.Service):

      @remote.method(message_types.VoidMessage, MyResponse)
      def entries_get(self, request):
        pass

    api = ApiConfigGenerator().pretty_print_config_to_json(MyService)
"""

# pylint: disable=g-bad-name

# pylint: disable=g-statement-before-imports,g-import-not-at-top
import collections
import json
import logging
import re

import api_exceptions
import message_parser

from protorpc import message_types
from protorpc import messages
from protorpc import remote
from protorpc import util

import resource_container
import users_id_token
import util as endpoints_util

from google.appengine.api import app_identity


package = 'google.appengine.endpoints'


__all__ = [
    'API_EXPLORER_CLIENT_ID',
    'ApiAuth',
    'ApiConfigGenerator',
    'ApiFrontEndLimitRule',
    'ApiFrontEndLimits',
    'EMAIL_SCOPE',
    'Issuer',
    'api',
    'method',
    'AUTH_LEVEL',
    'package',
]


API_EXPLORER_CLIENT_ID = '292824132082.apps.googleusercontent.com'
EMAIL_SCOPE = 'https://www.googleapis.com/auth/userinfo.email'
_PATH_VARIABLE_PATTERN = r'{([a-zA-Z_][a-zA-Z_.\d]*)}'

_MULTICLASS_MISMATCH_ERROR_TEMPLATE = (
    'Attempting to implement service %s, version %s, with multiple '
    'classes that aren\'t compatible. See docstring for api() for '
    'examples how to implement a multi-class API.')


Issuer = collections.namedtuple('Issuer', ['issuer', 'jwks_uri'])


def _Enum(docstring, *names):
  """Utility to generate enum classes used by annotations.

  Args:
    docstring: Docstring for the generated enum class.
    *names: Enum names.

  Returns:
    A class that contains enum names as attributes.
  """
  enums = dict(zip(names, range(len(names))))
  reverse = dict((value, key) for key, value in enums.iteritems())
  enums['reverse_mapping'] = reverse
  enums['__doc__'] = docstring
  return type('Enum', (object,), enums)

_AUTH_LEVEL_DOCSTRING = """
  Define the enums used by the auth_level annotation to specify frontend
  authentication requirement.

  Frontend authentication is handled by a Google API server prior to the
  request reaching backends. An early return before hitting the backend can
  happen if the request does not fulfil the requirement specified by the
  auth_level.

  Valid values of auth_level and their meanings are:

  AUTH_LEVEL.REQUIRED: Valid authentication credentials are required. Backend
    will be called only if authentication credentials are present and valid.

  AUTH_LEVEL.OPTIONAL: Authentication is optional. If authentication credentials
    are supplied they must be valid. Backend will be called if the request
    contains valid authentication credentials or no authentication credentials.

  AUTH_LEVEL.OPTIONAL_CONTINUE: Authentication is optional and will be attempted
    if authentication credentials are supplied. Invalid authentication
    credentials will be removed but the request can always reach backend.

  AUTH_LEVEL.NONE: Frontend authentication will be skipped. If authentication is
   desired, it will need to be performed by the backend.
  """

AUTH_LEVEL = _Enum(_AUTH_LEVEL_DOCSTRING, 'REQUIRED', 'OPTIONAL',
                   'OPTIONAL_CONTINUE', 'NONE')


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


def _CheckType(value, check_type, name, allow_none=True):
  """Check that the type of an object is acceptable.

  Args:
    value: The object whose type is to be checked.
    check_type: The type that the object must be an instance of.
    name: Name of the object, to be placed in any error messages.
    allow_none: True if value can be None, false if not.

  Raises:
    TypeError: If value is not an acceptable type.
  """
  if value is None and allow_none:
    return
  if not isinstance(value, check_type):
    raise TypeError('%s type doesn\'t match %s.' % (name, check_type))


def _CheckEnum(value, check_type, name):
  if value is None:
    return
  if value not in check_type.reverse_mapping:
    raise TypeError('%s is not a valid value for %s' % (value, name))


def _CheckAudiences(audiences):
  # Audiences can either be a list of audiences using the google_id_token
  # or a dict mapping auth issuer name to the list of audiences.
  if audiences is None or isinstance(audiences, dict):
    return
  else:
    endpoints_util.check_list_type(audiences, basestring, 'audiences')


# pylint: disable=g-bad-name
class _ApiInfo(object):
  """Configurable attributes of an API.

  A structured data object used to store API information associated with each
  remote.Service-derived class that implements an API.  This stores properties
  that could be different for each class (such as the path or
  collection/resource name), as well as properties common to all classes in
  the API (such as API name and version).
  """

  @util.positional(2)
  def __init__(self, common_info, resource_name=None, path=None, audiences=None,
               scopes=None, allowed_client_ids=None, auth_level=None,
               api_key_required=None):
    """Constructor for _ApiInfo.

    Args:
      common_info: _ApiDecorator.__ApiCommonInfo, Information that's common for
        all classes that implement an API.
      resource_name: string, The collection that the annotated class will
        implement in the API. (Default: None)
      path: string, Base request path for all methods in this API.
        (Default: None)
      audiences: list of strings, Acceptable audiences for authentication.
        (Default: None)
      scopes: list of strings, Acceptable scopes for authentication.
        (Default: None)
      allowed_client_ids: list of strings, Acceptable client IDs for auth.
        (Default: None)
      auth_level: enum from AUTH_LEVEL, Frontend authentication level.
        (Default: None)
      api_key_required: bool, whether a key is required to call this API.
    """
    _CheckType(resource_name, basestring, 'resource_name')
    _CheckType(path, basestring, 'path')
    endpoints_util.check_list_type(audiences, basestring, 'audiences')
    endpoints_util.check_list_type(scopes, basestring, 'scopes')
    endpoints_util.check_list_type(allowed_client_ids, basestring,
                                   'allowed_client_ids')
    _CheckEnum(auth_level, AUTH_LEVEL, 'auth_level')
    _CheckType(api_key_required, bool, 'api_key_required')

    self.__common_info = common_info
    self.__resource_name = resource_name
    self.__path = path
    self.__audiences = audiences
    self.__scopes = scopes
    self.__allowed_client_ids = allowed_client_ids
    self.__auth_level = auth_level
    self.__api_key_required = api_key_required

  def is_same_api(self, other):
    """Check if this implements the same API as another _ApiInfo instance."""
    if not isinstance(other, _ApiInfo):
      return False
    # pylint: disable=protected-access
    return self.__common_info is other.__common_info

  @property
  def name(self):
    """Name of the API."""
    return self.__common_info.name

  @property
  def version(self):
    """Version of the API."""
    return self.__common_info.version

  @property
  def description(self):
    """Description of the API."""
    return self.__common_info.description

  @property
  def hostname(self):
    """Hostname for the API."""
    return self.__common_info.hostname

  @property
  def audiences(self):
    """List of audiences accepted for the API, overriding the defaults."""
    if self.__audiences is not None:
      return self.__audiences
    return self.__common_info.audiences

  @property
  def scopes(self):
    """List of scopes accepted for the API, overriding the defaults."""
    if self.__scopes is not None:
      return self.__scopes
    return self.__common_info.scopes

  @property
  def allowed_client_ids(self):
    """List of client IDs accepted for the API, overriding the defaults."""
    if self.__allowed_client_ids is not None:
      return self.__allowed_client_ids
    return self.__common_info.allowed_client_ids

  @property
  def issuers(self):
    """List of auth issuers for the API."""
    return self.__common_info.issuers

  @property
  def auth_level(self):
    """Enum from AUTH_LEVEL specifying the frontend authentication level."""
    if self.__auth_level is not None:
      return self.__auth_level
    return self.__common_info.auth_level

  @property
  def api_key_required(self):
    """bool specifying whether a key is required to call into this API."""
    if self.__api_key_required is not None:
      return self.__api_key_required
    return self.__common_info.api_key_required

  @property
  def canonical_name(self):
    """Canonical name for the API."""
    return self.__common_info.canonical_name

  @property
  def auth(self):
    """Authentication configuration information for this API."""
    return self.__common_info.auth

  @property
  def owner_domain(self):
    """Domain of the owner of this API."""
    return self.__common_info.owner_domain

  @property
  def owner_name(self):
    """Name of the owner of this API."""
    return self.__common_info.owner_name

  @property
  def package_path(self):
    """Package this API belongs to, '/' delimited.  Used by client libs."""
    return self.__common_info.package_path

  @property
  def frontend_limits(self):
    """Optional query limits for unregistered developers."""
    return self.__common_info.frontend_limits

  @property
  def title(self):
    """Human readable name of this API."""
    return self.__common_info.title

  @property
  def documentation(self):
    """Link to the documentation for this version of the API."""
    return self.__common_info.documentation

  @property
  def resource_name(self):
    """Resource name for the class this decorates."""
    return self.__resource_name

  @property
  def path(self):
    """Base path prepended to any method paths in the class this decorates."""
    return self.__path

  @property
  def base_path(self):
    """Base path for the entire API prepended before the path property."""
    return self.__common_info.base_path


class _ApiDecorator(object):
  """Decorator for single- or multi-class APIs.

  An instance of this class can be used directly as a decorator for a
  single-class API.  Or call the api_class() method to decorate a multi-class
  API.
  """

  @util.positional(3)
  def __init__(self, name, version, description=None, hostname=None,
               audiences=None, scopes=None, allowed_client_ids=None,
               canonical_name=None, auth=None, owner_domain=None,
               owner_name=None, package_path=None, frontend_limits=None,
               title=None, documentation=None, auth_level=None, issuers=None,
               api_key_required=None, base_path=None):
    """Constructor for _ApiDecorator.

    Args:
      name: string, Name of the API.
      version: string, Version of the API.
      description: string, Short description of the API (Default: None)
      hostname: string, Hostname of the API (Default: app engine default host)
      audiences: list of strings, Acceptable audiences for authentication.
      scopes: list of strings, Acceptable scopes for authentication.
      allowed_client_ids: list of strings, Acceptable client IDs for auth.
      canonical_name: string, the canonical name for the API, a more human
        readable version of the name.
      auth: ApiAuth instance, the authentication configuration information
        for this API.
      owner_domain: string, the domain of the person or company that owns
        this API.  Along with owner_name, this provides hints to properly
        name client libraries for this API.
      owner_name: string, the name of the owner of this API.  Along with
        owner_domain, this provides hints to properly name client libraries
        for this API.
      package_path: string, the "package" this API belongs to.  This '/'
        delimited value specifies logical groupings of APIs.  This is used by
        client libraries of this API.
      frontend_limits: ApiFrontEndLimits, optional query limits for unregistered
        developers.
      title: string, the human readable title of your API. It is exposed in the
        discovery service.
      documentation: string, a URL where users can find documentation about this
        version of the API. This will be surfaced in the API Explorer and GPE
        plugin to allow users to learn about your service.
      auth_level: enum from AUTH_LEVEL, Frontend authentication level.
      issuers: list of endpoints.Issuer objects, auth issuers for this API.
      api_key_required: bool, whether a key is required to call this API.
      base_path: string, the base path for all endpoints in this API.
    """
    self.__common_info = self.__ApiCommonInfo(
        name, version, description=description, hostname=hostname,
        audiences=audiences, scopes=scopes,
        allowed_client_ids=allowed_client_ids,
        canonical_name=canonical_name, auth=auth, owner_domain=owner_domain,
        owner_name=owner_name, package_path=package_path,
        frontend_limits=frontend_limits, title=title,
        documentation=documentation, auth_level=auth_level, issuers=issuers,
        api_key_required=api_key_required, base_path=base_path)
    self.__classes = []

  class __ApiCommonInfo(object):
    """API information that's common among all classes that implement an API.

    When a remote.Service-derived class implements part of an API, there is
    some common information that remains constant across all such classes
    that implement the same API.  This includes things like name, version,
    hostname, and so on.  __ApiComminInfo stores that common information, and
    a single __ApiCommonInfo instance is shared among all classes that
    implement the same API, guaranteeing that they share the same common
    information.

    Some of these values can be overridden (such as audiences and scopes),
    while some can't and remain the same for all classes that implement
    the API (such as name and version).
    """

    @util.positional(3)
    def __init__(self, name, version, description=None, hostname=None,
                 audiences=None, scopes=None, allowed_client_ids=None,
                 canonical_name=None, auth=None, owner_domain=None,
                 owner_name=None, package_path=None, frontend_limits=None,
                 title=None, documentation=None, auth_level=None, issuers=None,
                 api_key_required=None, base_path=None):
      """Constructor for _ApiCommonInfo.

      Args:
        name: string, Name of the API.
        version: string, Version of the API.
        description: string, Short description of the API (Default: None)
        hostname: string, Hostname of the API (Default: app engine default host)
        audiences: list of strings, Acceptable audiences for authentication.
        scopes: list of strings, Acceptable scopes for authentication.
        allowed_client_ids: list of strings, Acceptable client IDs for auth.
        canonical_name: string, the canonical name for the API, a more human
          readable version of the name.
        auth: ApiAuth instance, the authentication configuration information
          for this API.
        owner_domain: string, the domain of the person or company that owns
          this API.  Along with owner_name, this provides hints to properly
          name client libraries for this API.
        owner_name: string, the name of the owner of this API.  Along with
          owner_domain, this provides hints to properly name client libraries
          for this API.
        package_path: string, the "package" this API belongs to.  This '/'
          delimited value specifies logical groupings of APIs.  This is used by
          client libraries of this API.
        frontend_limits: ApiFrontEndLimits, optional query limits for
          unregistered developers.
        title: string, the human readable title of your API. It is exposed in
          the discovery service.
        documentation: string, a URL where users can find documentation about
          this version of the API. This will be surfaced in the API Explorer and
          GPE plugin to allow users to learn about your service.
        auth_level: enum from AUTH_LEVEL, Frontend authentication level.
        issuers: dict, mapping auth issuer names to endpoints.Issuer objects.
        api_key_required: bool, whether a key is required to call into this API.
        base_path: string, the base path for all endpoints in this API.
      """
      _CheckType(name, basestring, 'name', allow_none=False)
      _CheckType(version, basestring, 'version', allow_none=False)
      _CheckType(description, basestring, 'description')
      _CheckType(hostname, basestring, 'hostname')
      endpoints_util.check_list_type(scopes, basestring, 'scopes')
      endpoints_util.check_list_type(allowed_client_ids, basestring,
                                     'allowed_client_ids')
      _CheckType(canonical_name, basestring, 'canonical_name')
      _CheckType(auth, ApiAuth, 'auth')
      _CheckType(owner_domain, basestring, 'owner_domain')
      _CheckType(owner_name, basestring, 'owner_name')
      _CheckType(package_path, basestring, 'package_path')
      _CheckType(frontend_limits, ApiFrontEndLimits, 'frontend_limits')
      _CheckType(title, basestring, 'title')
      _CheckType(documentation, basestring, 'documentation')
      _CheckEnum(auth_level, AUTH_LEVEL, 'auth_level')
      _CheckType(api_key_required, bool, 'api_key_required')
      _CheckType(base_path, basestring, 'base_path')

      _CheckType(issuers, dict, 'issuers')
      if issuers:
        for issuer_name, issuer_value in issuers.items():
          _CheckType(issuer_name, basestring, 'issuer %s' % issuer_name)
          _CheckType(issuer_value, Issuer, 'issuer value for %s' % issuer_name)

      _CheckAudiences(audiences)

      if hostname is None:
        hostname = app_identity.get_default_version_hostname()
      if scopes is None:
        scopes = [EMAIL_SCOPE]
      if allowed_client_ids is None:
        allowed_client_ids = [API_EXPLORER_CLIENT_ID]
      if auth_level is None:
        auth_level = AUTH_LEVEL.NONE
      if api_key_required is None:
        api_key_required = False
      if base_path is None:
        base_path = '/_ah/api/'

      self.__name = name
      self.__version = version
      self.__description = description
      self.__hostname = hostname
      self.__audiences = audiences
      self.__scopes = scopes
      self.__allowed_client_ids = allowed_client_ids
      self.__canonical_name = canonical_name
      self.__auth = auth
      self.__owner_domain = owner_domain
      self.__owner_name = owner_name
      self.__package_path = package_path
      self.__frontend_limits = frontend_limits
      self.__title = title
      self.__documentation = documentation
      self.__auth_level = auth_level
      self.__issuers = issuers
      self.__api_key_required = api_key_required
      self.__base_path = base_path

    @property
    def name(self):
      """Name of the API."""
      return self.__name

    @property
    def version(self):
      """Version of the API."""
      return self.__version

    @property
    def description(self):
      """Description of the API."""
      return self.__description

    @property
    def hostname(self):
      """Hostname for the API."""
      return self.__hostname

    @property
    def audiences(self):
      """List of audiences accepted by default for the API."""
      return self.__audiences

    @property
    def scopes(self):
      """List of scopes accepted by default for the API."""
      return self.__scopes

    @property
    def allowed_client_ids(self):
      """List of client IDs accepted by default for the API."""
      return self.__allowed_client_ids

    @property
    def issuers(self):
      """List of auth issuers for the API."""
      return self.__issuers

    @property
    def auth_level(self):
      """Enum from AUTH_LEVEL specifying default frontend auth level."""
      return self.__auth_level

    @property
    def canonical_name(self):
      """Canonical name for the API."""
      return self.__canonical_name

    @property
    def auth(self):
      """Authentication configuration for this API."""
      return self.__auth

    @property
    def api_key_required(self):
      """Whether a key is required to call into this API."""
      return self.__api_key_required

    @property
    def owner_domain(self):
      """Domain of the owner of this API."""
      return self.__owner_domain

    @property
    def owner_name(self):
      """Name of the owner of this API."""
      return self.__owner_name

    @property
    def package_path(self):
      """Package this API belongs to, '/' delimited.  Used by client libs."""
      return self.__package_path

    @property
    def frontend_limits(self):
      """Optional query limits for unregistered developers."""
      return self.__frontend_limits

    @property
    def title(self):
      """Human readable name of this API."""
      return self.__title

    @property
    def documentation(self):
      """Link to the documentation for this version of the API."""
      return self.__documentation

    @property
    def base_path(self):
      """The base path for all endpoints in this API."""
      return self.__base_path

  def __call__(self, service_class):
    """Decorator for ProtoRPC class that configures Google's API server.

    Args:
      service_class: remote.Service class, ProtoRPC service class being wrapped.

    Returns:
      Same class with API attributes assigned in api_info.
    """
    return self.api_class()(service_class)

  def api_class(self, resource_name=None, path=None, audiences=None,
                scopes=None, allowed_client_ids=None, auth_level=None,
                api_key_required=None):
    """Get a decorator for a class that implements an API.

    This can be used for single-class or multi-class implementations.  It's
    used implicitly in simple single-class APIs that only use @api directly.

    Args:
      resource_name: string, Resource name for the class this decorates.
        (Default: None)
      path: string, Base path prepended to any method paths in the class this
        decorates. (Default: None)
      audiences: list of strings, Acceptable audiences for authentication.
        (Default: None)
      scopes: list of strings, Acceptable scopes for authentication.
        (Default: None)
      allowed_client_ids: list of strings, Acceptable client IDs for auth.
        (Default: None)
      auth_level: enum from AUTH_LEVEL, Frontend authentication level.
        (Default: None)
      api_key_required: bool, Whether a key is required to call into this API.
        (Default: None)

    Returns:
      A decorator function to decorate a class that implements an API.
    """

    def apiserving_api_decorator(api_class):
      """Decorator for ProtoRPC class that configures Google's API server.

      Args:
        api_class: remote.Service class, ProtoRPC service class being wrapped.

      Returns:
        Same class with API attributes assigned in api_info.
      """
      self.__classes.append(api_class)
      api_class.api_info = _ApiInfo(
          self.__common_info, resource_name=resource_name,
          path=path, audiences=audiences, scopes=scopes,
          allowed_client_ids=allowed_client_ids, auth_level=auth_level,
          api_key_required=api_key_required)
      return api_class

    return apiserving_api_decorator

  def get_api_classes(self):
    """Get the list of remote.Service classes that implement this API."""
    return self.__classes


class ApiAuth(object):
  """Optional authorization configuration information for an API."""

  def __init__(self, allow_cookie_auth=None, blocked_regions=None):
    """Constructor for ApiAuth, authentication information for an API.

    Args:
      allow_cookie_auth: boolean, whether cooking auth is allowed. By
        default, API methods do not allow cookie authentication, and
        require the use of OAuth2 or ID tokens. Setting this field to
        True will allow cookies to be used to access the API, with
        potentially dangerous results. Please be very cautious in enabling
        this setting, and make sure to require appropriate XSRF tokens to
        protect your API.
      blocked_regions: list of Strings, a list of 2-letter ISO region codes
        to block.
    """
    _CheckType(allow_cookie_auth, bool, 'allow_cookie_auth')
    endpoints_util.check_list_type(blocked_regions, basestring,
                                   'blocked_regions')

    self.__allow_cookie_auth = allow_cookie_auth
    self.__blocked_regions = blocked_regions

  @property
  def allow_cookie_auth(self):
    """Whether cookie authentication is allowed for this API."""
    return self.__allow_cookie_auth

  @property
  def blocked_regions(self):
    """List of 2-letter ISO region codes to block."""
    return self.__blocked_regions


class ApiFrontEndLimitRule(object):
  """Custom rule to limit unregistered traffic."""

  def __init__(self, match=None, qps=None, user_qps=None, daily=None,
               analytics_id=None):
    """Constructor for ApiFrontEndLimitRule.

    Args:
      match: string, the matching rule that defines this traffic segment.
      qps: int, the aggregate QPS for this segment.
      user_qps: int, the per-end-user QPS for this segment.
      daily: int, the aggregate daily maximum for this segment.
      analytics_id: string, the project ID under which traffic for this segment
        will be logged.
    """
    _CheckType(match, basestring, 'match')
    _CheckType(qps, int, 'qps')
    _CheckType(user_qps, int, 'user_qps')
    _CheckType(daily, int, 'daily')
    _CheckType(analytics_id, basestring, 'analytics_id')

    self.__match = match
    self.__qps = qps
    self.__user_qps = user_qps
    self.__daily = daily
    self.__analytics_id = analytics_id

  @property
  def match(self):
    """The matching rule that defines this traffic segment."""
    return self.__match

  @property
  def qps(self):
    """The aggregate QPS for this segment."""
    return self.__qps

  @property
  def user_qps(self):
    """The per-end-user QPS for this segment."""
    return self.__user_qps

  @property
  def daily(self):
    """The aggregate daily maximum for this segment."""
    return self.__daily

  @property
  def analytics_id(self):
    """Project ID under which traffic for this segment will be logged."""
    return self.__analytics_id


class ApiFrontEndLimits(object):
  """Optional front end limit information for an API."""

  def __init__(self, unregistered_user_qps=None, unregistered_qps=None,
               unregistered_daily=None, rules=None):
    """Constructor for ApiFrontEndLimits, front end limit info for an API.

    Args:
      unregistered_user_qps: int, the per-end-user QPS.  Users are identified
        by their IP address. A value of 0 will block unregistered requests.
      unregistered_qps: int, an aggregate QPS upper-bound for all unregistered
        traffic. A value of 0 currently means unlimited, though it might change
        in the future. To block unregistered requests, use unregistered_user_qps
        or unregistered_daily instead.
      unregistered_daily: int, an aggregate daily upper-bound for all
        unregistered traffic. A value of 0 will block unregistered requests.
      rules: A list or tuple of ApiFrontEndLimitRule instances: custom rules
        used to apply limits to unregistered traffic.
    """
    _CheckType(unregistered_user_qps, int, 'unregistered_user_qps')
    _CheckType(unregistered_qps, int, 'unregistered_qps')
    _CheckType(unregistered_daily, int, 'unregistered_daily')
    endpoints_util.check_list_type(rules, ApiFrontEndLimitRule, 'rules')

    self.__unregistered_user_qps = unregistered_user_qps
    self.__unregistered_qps = unregistered_qps
    self.__unregistered_daily = unregistered_daily
    self.__rules = rules

  @property
  def unregistered_user_qps(self):
    """Per-end-user QPS limit."""
    return self.__unregistered_user_qps

  @property
  def unregistered_qps(self):
    """Aggregate QPS upper-bound for all unregistered traffic."""
    return self.__unregistered_qps

  @property
  def unregistered_daily(self):
    """Aggregate daily upper-bound for all unregistered traffic."""
    return self.__unregistered_daily

  @property
  def rules(self):
    """Custom rules used to apply limits to unregistered traffic."""
    return self.__rules


@util.positional(2)
def api(name, version, description=None, hostname=None, audiences=None,
        scopes=None, allowed_client_ids=None, canonical_name=None,
        auth=None, owner_domain=None, owner_name=None, package_path=None,
        frontend_limits=None, title=None, documentation=None, auth_level=None,
        issuers=None, api_key_required=None, base_path=None):
  """Decorate a ProtoRPC Service class for use by the framework above.

  This decorator can be used to specify an API name, version, description, and
  hostname for your API.

  Sample usage (python 2.7):
    @endpoints.api(name='guestbook', version='v0.2',
                   description='Guestbook API')
    class PostService(remote.Service):
      ...

  Sample usage (python 2.5):
    class PostService(remote.Service):
      ...
    endpoints.api(name='guestbook', version='v0.2',
                  description='Guestbook API')(PostService)

  Sample usage if multiple classes implement one API:
    api_root = endpoints.api(name='library', version='v1.0')

    @api_root.api_class(resource_name='shelves')
    class Shelves(remote.Service):
      ...

    @api_root.api_class(resource_name='books', path='books')
    class Books(remote.Service):
      ...

  Args:
    name: string, Name of the API.
    version: string, Version of the API.
    description: string, Short description of the API (Default: None)
    hostname: string, Hostname of the API (Default: app engine default host)
    audiences: list of strings, Acceptable audiences for authentication.
    scopes: list of strings, Acceptable scopes for authentication.
    allowed_client_ids: list of strings, Acceptable client IDs for auth.
    canonical_name: string, the canonical name for the API, a more human
      readable version of the name.
    auth: ApiAuth instance, the authentication configuration information
      for this API.
    owner_domain: string, the domain of the person or company that owns
      this API.  Along with owner_name, this provides hints to properly
      name client libraries for this API.
    owner_name: string, the name of the owner of this API.  Along with
      owner_domain, this provides hints to properly name client libraries
      for this API.
    package_path: string, the "package" this API belongs to.  This '/'
      delimited value specifies logical groupings of APIs.  This is used by
      client libraries of this API.
    frontend_limits: ApiFrontEndLimits, optional query limits for unregistered
      developers.
    title: string, the human readable title of your API. It is exposed in the
      discovery service.
    documentation: string, a URL where users can find documentation about this
      version of the API. This will be surfaced in the API Explorer and GPE
      plugin to allow users to learn about your service.
    auth_level: enum from AUTH_LEVEL, frontend authentication level.
    issuers: list of endpoints.Issuer objects, auth issuers for this API.
    api_key_required: bool, whether a key is required to call into this API.
    base_path: string, the base path for all endpoints in this API.

  Returns:
    Class decorated with api_info attribute, an instance of ApiInfo.
  """

  return _ApiDecorator(name, version, description=description,
                       hostname=hostname, audiences=audiences, scopes=scopes,
                       allowed_client_ids=allowed_client_ids,
                       canonical_name=canonical_name, auth=auth,
                       owner_domain=owner_domain, owner_name=owner_name,
                       package_path=package_path,
                       frontend_limits=frontend_limits, title=title,
                       documentation=documentation, auth_level=auth_level,
                       issuers=issuers, api_key_required=api_key_required,
                       base_path=base_path)


class _MethodInfo(object):
  """Configurable attributes of an API method.

  Consolidates settings from @method decorator and/or any settings that were
  calculating from the ProtoRPC method name, so they only need to be calculated
  once.
  """

  @util.positional(1)
  def __init__(self, name=None, path=None, http_method=None,
               scopes=None, audiences=None, allowed_client_ids=None,
               auth_level=None, api_key_required=None, request_body_class=None,
               request_params_class=None):
    """Constructor.

    Args:
      name: string, Name of the method, prepended with <apiname>. to make it
        unique.
      path: string, Path portion of the URL to the method, for RESTful methods.
      http_method: string, HTTP method supported by the method.
      scopes: list of string, OAuth2 token must contain one of these scopes.
      audiences: list of string, IdToken must contain one of these audiences.
      allowed_client_ids: list of string, Client IDs allowed to call the method.
      auth_level: enum from AUTH_LEVEL, Frontend auth level for the method.
      api_key_required: bool, whether a key is required to call the method.
      request_body_class: The type for the request body when using a
        ResourceContainer. Otherwise, null.
      request_params_class: The type for the request parameters when using a
        ResourceContainer. Otherwise, null.
    """
    self.__name = name
    self.__path = path
    self.__http_method = http_method
    self.__scopes = scopes
    self.__audiences = audiences
    self.__allowed_client_ids = allowed_client_ids
    self.__auth_level = auth_level
    self.__api_key_required = api_key_required
    self.__request_body_class = request_body_class
    self.__request_params_class = request_params_class

  def __safe_name(self, method_name):
    """Restrict method name to a-zA-Z0-9_, first char lowercase."""
    # Endpoints backend restricts what chars are allowed in a method name.
    safe_name = re.sub(r'[^\.a-zA-Z0-9_]', '', method_name)

    # Strip any number of leading underscores.
    safe_name = safe_name.lstrip('_')

    # Ensure the first character is lowercase.
    # Slice from 0:1 rather than indexing [0] in case safe_name is length 0.
    return safe_name[0:1].lower() + safe_name[1:]

  @property
  def name(self):
    """Method name as specified in decorator or derived."""
    return self.__name

  def get_path(self, api_info):
    """Get the path portion of the URL to the method (for RESTful methods).

    Request path can be specified in the method, and it could have a base
    path prepended to it.

    Args:
      api_info: API information for this API, possibly including a base path.
        This is the api_info property on the class that's been annotated for
        this API.

    Returns:
      This method's request path (not including the http://.../{base_path}
      prefix).

    Raises:
      ApiConfigurationError: If the path isn't properly formatted.
    """
    path = self.__path or ''
    if path and path[0] == '/':
      # Absolute path, ignoring any prefixes.  Just strip off the leading /.
      path = path[1:]
    else:
      # Relative path.
      if api_info.path:
        path = '%s%s%s' % (api_info.path, '/' if path else '', path)

    # Verify that the path seems valid.
    for part in path.split('/'):
      if part and '{' in part and '}' in part:
        if re.match('^{[^{}]+}$', part) is None:
          raise api_exceptions.ApiConfigurationError(
              'Invalid path segment: %s (part of %s)' % (part, path))
    return path

  @property
  def http_method(self):
    """HTTP method supported by the method (e.g. GET, POST)."""
    return self.__http_method

  @property
  def scopes(self):
    """List of scopes for the API method."""
    return self.__scopes

  @property
  def audiences(self):
    """List of audiences for the API method."""
    return self.__audiences

  @property
  def allowed_client_ids(self):
    """List of allowed client IDs for the API method."""
    return self.__allowed_client_ids

  @property
  def auth_level(self):
    """Enum from AUTH_LEVEL specifying default frontend auth level."""
    return self.__auth_level

  @property
  def api_key_required(self):
    """bool whether a key is required to call the API method."""
    return self.__api_key_required

  @property
  def request_body_class(self):
    """Type of request body when using a ResourceContainer."""
    return self.__request_body_class

  @property
  def request_params_class(self):
    """Type of request parameter message when using a ResourceContainer."""
    return self.__request_params_class

  def is_api_key_required(self, api_info):
    if self.api_key_required is not None:
      return self.api_key_required
    else:
      return api_info.api_key_required

  def method_id(self, api_info):
    """Computed method name."""
    # This is done here for now because at __init__ time, the method is known
    # but not the api, and thus not the api name.  Later, in
    # ApiConfigGenerator.__method_descriptor, the api name is known.
    if api_info.resource_name:
      resource_part = '.%s' % self.__safe_name(api_info.resource_name)
    else:
      resource_part = ''
    return '%s%s.%s' % (self.__safe_name(api_info.name), resource_part,
                        self.__safe_name(self.name))


@util.positional(2)
def method(request_message=message_types.VoidMessage,
           response_message=message_types.VoidMessage,
           name=None,
           path=None,
           http_method='POST',
           scopes=None,
           audiences=None,
           allowed_client_ids=None,
           auth_level=None,
           api_key_required=None):
  """Decorate a ProtoRPC Method for use by the framework above.

  This decorator can be used to specify a method name, path, http method,
  scopes, audiences, client ids and auth_level.

  Sample usage:
    @api_config.method(RequestMessage, ResponseMessage,
                       name='insert', http_method='PUT')
    def greeting_insert(request):
      ...
      return response

  Args:
    request_message: Message type of expected request.
    response_message: Message type of expected response.
    name: string, Name of the method, prepended with <apiname>. to make it
      unique. (Default: python method name)
    path: string, Path portion of the URL to the method, for RESTful methods.
    http_method: string, HTTP method supported by the method. (Default: POST)
    scopes: list of string, OAuth2 token must contain one of these scopes.
    audiences: list of string, IdToken must contain one of these audiences.
    allowed_client_ids: list of string, Client IDs allowed to call the method.
      If None and auth_level is REQUIRED, no calls will be allowed.
    auth_level: enum from AUTH_LEVEL, Frontend auth level for the method.
    api_key_required: bool, whether a key is required to call the method

  Returns:
    'apiserving_method_wrapper' function.

  Raises:
    TypeError: if the request_type or response_type parameters are not
      proper subclasses of messages.Message.
  """

  # Default HTTP method if one is not specified.
  DEFAULT_HTTP_METHOD = 'POST'

  def apiserving_method_decorator(api_method):
    """Decorator for ProtoRPC method that configures Google's API server.

    Args:
      api_method: Original method being wrapped.

    Returns:
      Function responsible for actual invocation.
      Assigns the following attributes to invocation function:
        remote: Instance of RemoteInfo, contains remote method information.
        remote.request_type: Expected request type for remote method.
        remote.response_type: Response type returned from remote method.
        method_info: Instance of _MethodInfo, api method configuration.
      It is also assigned attributes corresponding to the aforementioned kwargs.

    Raises:
      TypeError: if the request_type or response_type parameters are not
        proper subclasses of messages.Message.
      KeyError: if the request_message is a ResourceContainer and the newly
          created remote method has been reference by the container before. This
          should never occur because a remote method is created once.
    """
    request_body_class = None
    request_params_class = None
    if isinstance(request_message, resource_container.ResourceContainer):
      remote_decorator = remote.method(request_message.combined_message_class,
                                       response_message)
      request_body_class = request_message.body_message_class()
      request_params_class = request_message.parameters_message_class()
    else:
      remote_decorator = remote.method(request_message, response_message)
    remote_method = remote_decorator(api_method)

    def invoke_remote(service_instance, request):
      # If the server didn't specify any auth information, build it now.
      # pylint: disable=protected-access
      users_id_token._maybe_set_current_user_vars(
          invoke_remote, api_info=getattr(service_instance, 'api_info', None),
          request=request)
      # pylint: enable=protected-access
      return remote_method(service_instance, request)

    invoke_remote.remote = remote_method.remote
    if isinstance(request_message, resource_container.ResourceContainer):
      resource_container.ResourceContainer.add_to_cache(
          invoke_remote.remote, request_message)

    invoke_remote.method_info = _MethodInfo(
        name=name or api_method.__name__, path=path or api_method.__name__,
        http_method=http_method or DEFAULT_HTTP_METHOD,
        scopes=scopes, audiences=audiences,
        allowed_client_ids=allowed_client_ids, auth_level=auth_level,
        api_key_required=api_key_required,
        request_body_class=request_body_class,
        request_params_class=request_params_class)
    invoke_remote.__name__ = invoke_remote.method_info.name
    return invoke_remote

  endpoints_util.check_list_type(scopes, basestring, 'scopes')
  endpoints_util.check_list_type(allowed_client_ids, basestring,
                                 'allowed_client_ids')
  _CheckEnum(auth_level, AUTH_LEVEL, 'auth_level')

  _CheckAudiences(audiences)

  return apiserving_method_decorator


class ApiConfigGenerator(object):
  """Generates an API configuration from a ProtoRPC service.

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

    api_config = ApiConfigGenerator().pretty_print_config_to_json(HelloService)

  The resulting api_config will be a JSON document describing the API
  implemented by HelloService.
  """

  # Constants for categorizing a request method.
  # __NO_BODY - Request without a request body, such as GET and DELETE methods.
  # __HAS_BODY - Request (such as POST/PUT/PATCH) with info in the request body.
  __NO_BODY = 1
  __HAS_BODY = 2

  def __init__(self):
    self.__parser = message_parser.MessageTypeToJsonSchema()

    # Maps method id to the request schema id.
    self.__request_schema = {}

    # Maps method id to the response schema id.
    self.__response_schema = {}

    # Maps from ProtoRPC name to method id.
    self.__id_from_name = {}

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

  # TODO(dhermes): Support all the parameter types
  # Currently missing DATE and ETAG
  def __field_to_parameter_type(self, field):
    """Converts the field variant type into a string describing the parameter.

    Args:
      field: An instance of a subclass of messages.Field.

    Returns:
      A string corresponding to the variant enum of the field, with a few
        exceptions. In the case of signed ints, the 's' is dropped; for the BOOL
        variant, 'boolean' is used; and for the ENUM variant, 'string' is used.

    Raises:
      TypeError: if the field variant is a message variant.
    """
    # We use lowercase values for types (e.g. 'string' instead of 'STRING').
    variant = field.variant
    if variant == messages.Variant.MESSAGE:
      raise TypeError('A message variant can\'t be used in a parameter.')

    custom_variant_map = {
        messages.Variant.SINT32: 'int32',
        messages.Variant.SINT64: 'int64',
        messages.Variant.BOOL: 'boolean',
        messages.Variant.ENUM: 'string',
    }
    return custom_variant_map.get(variant) or variant.name.lower()

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
                                 _segment_index=0):
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
      _segment_index: Integer; used to hold the position of current segment so
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
    if _segment_index >= len(segment_list):
      # In this case, the field is the final one, so should be simple type
      if isinstance(field, messages.MessageField):
        field_class = field.__class__.__name__
        raise TypeError('Can\'t use messages in path. Subfield %r was '
                        'included but is a %s.' % (parameter, field_class))
      return

    segment = segment_list[_segment_index]
    parameter += '.' + segment
    try:
      field = field.type.field_by_name(segment)
    except (AttributeError, KeyError):
      raise TypeError('Subfield %r from path does not exist.' % (parameter,))

    self.__validate_simple_subfield(parameter, field, segment_list,
                                    _segment_index=_segment_index + 1)

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

  def __parameter_default(self, final_subfield):
    """Returns default value of final subfield if it has one.

    If this subfield comes from a field list returned from __field_to_subfields,
    none of the fields in the subfield list can have a default except the final
    one since they all must be message fields.

    Args:
      final_subfield: A simple field from the end of a subfield list.

    Returns:
      The default value of the subfield, if any exists, with the exception of an
          enum field, which will have its value cast to a string.
    """
    if final_subfield.default:
      if isinstance(final_subfield, messages.EnumField):
        return final_subfield.default.name
      else:
        return final_subfield.default

  def __parameter_enum(self, final_subfield):
    """Returns enum descriptor of final subfield if it is an enum.

    An enum descriptor is a dictionary with keys as the names from the enum and
    each value is a dictionary with a single key "backendValue" and value equal
    to the same enum name used to stored it in the descriptor.

    The key "description" can also be used next to "backendValue", but protorpc
    Enum classes have no way of supporting a description for each value.

    Args:
      final_subfield: A simple field from the end of a subfield list.

    Returns:
      The enum descriptor for the field, if it's an enum descriptor, else
          returns None.
    """
    if isinstance(final_subfield, messages.EnumField):
      enum_descriptor = {}
      for enum_value in final_subfield.type.to_dict().keys():
        enum_descriptor[enum_value] = {'backendValue': enum_value}
      return enum_descriptor

  def __parameter_descriptor(self, subfield_list):
    """Creates descriptor for a parameter using the subfields that define it.

    Each parameter is defined by a list of fields, with all but the last being
    a message field and the final being a simple (non-message) field.

    Many of the fields in the descriptor are determined solely by the simple
    field at the end, though some (such as repeated and required) take the whole
    chain of fields into consideration.

    Args:
      subfield_list: List of fields describing the parameter.

    Returns:
      Dictionary containing a descriptor for the parameter described by the list
          of fields.
    """
    descriptor = {}
    final_subfield = subfield_list[-1]

    # Required
    if all(subfield.required for subfield in subfield_list):
      descriptor['required'] = True

    # Type
    descriptor['type'] = self.__field_to_parameter_type(final_subfield)

    # Default
    default = self.__parameter_default(final_subfield)
    if default is not None:
      descriptor['default'] = default

    # Repeated
    if any(subfield.repeated for subfield in subfield_list):
      descriptor['repeated'] = True

    # Enum
    enum_descriptor = self.__parameter_enum(final_subfield)
    if enum_descriptor is not None:
      descriptor['enum'] = enum_descriptor

    return descriptor

  def __add_parameters_from_field(self, field, path_parameters,
                                  params, param_order):
    """Adds all parameters in a field to a method parameters descriptor.

    Simple fields will only have one parameter, but a message field 'x' that
    corresponds to a message class with fields 'y' and 'z' will result in
    parameters 'x.y' and 'x.z', for example. The mapping from field to
    parameters is mostly handled by __field_to_subfields.

    Args:
      field: Field from which parameters will be added to the method descriptor.
      path_parameters: A list of parameters matched from a path for this field.
         For example for the hypothetical 'x' from above if the path was
         '/a/{x.z}/b/{other}' then this list would contain only the element
         'x.z' since 'other' does not match to this field.
      params: Dictionary with parameter names as keys and parameter descriptors
          as values. This will be updated for each parameter in the field.
      param_order: List of required parameter names to give them an order in the
          descriptor. All required parameters in the field will be added to this
          list.
    """
    for subfield_list in self.__field_to_subfields(field):
      descriptor = self.__parameter_descriptor(subfield_list)

      qualified_name = '.'.join(subfield.name for subfield in subfield_list)
      in_path = qualified_name in path_parameters
      if descriptor.get('required', in_path):
        descriptor['required'] = True
        param_order.append(qualified_name)

      params[qualified_name] = descriptor

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
      A tuple (dict, list of string): Descriptor of the parameters, Order of the
        parameters.
    """
    params = {}
    param_order = []

    path_parameter_dict = self.__get_path_parameters(path)
    for field in sorted(message_type.all_fields(), key=lambda f: f.number):
      matched_path_parameters = path_parameter_dict.get(field.name, [])
      self.__validate_path_parameters(field, matched_path_parameters)
      if matched_path_parameters or request_kind == self.__NO_BODY:
        self.__add_parameters_from_field(field, matched_path_parameters,
                                         params, param_order)

    return params, param_order

  # TODO(user): request_kind is only used by
  #              __params_descriptor_without_container so can be removed
  #              once that method is fully deprecated.
  def __params_descriptor(self, message_type, request_kind, path, method_id):
    """Describe the parameters of a method.

    If the message_type is not a ResourceContainer, will fall back to
    __params_descriptor_without_container (which will eventually be deprecated).

    If the message type is a ResourceContainer, then all path/query parameters
    will come from the ResourceContainer This method will also make sure all
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
                        'using a ResourceContainer This will fail in future '
                        'releases; please switch to using ResourceContainer as '
                        'soon as possible.', method_id)
      return self.__params_descriptor_without_container(
          message_type, request_kind, path)

    # From here, we can assume message_type is a ResourceContainer
    message_type = message_type.parameters_message_class()

    params = {}
    param_order = []

    # Make sure all path parameters are covered.
    for field_name, matched_path_parameters in path_parameter_dict.iteritems():
      field = message_type.field_by_name(field_name)
      self.__validate_path_parameters(field, matched_path_parameters)

    # Add all fields, sort by field.number since we have parameterOrder.
    for field in sorted(message_type.all_fields(), key=lambda f: f.number):
      matched_path_parameters = path_parameter_dict.get(field.name, [])
      self.__add_parameters_from_field(field, matched_path_parameters,
                                       params, param_order)

    return params, param_order

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
    descriptor = {}

    params, param_order = self.__params_descriptor(message_type, request_kind,
                                                   path, method_id)

    if isinstance(message_type, resource_container.ResourceContainer):
      message_type = message_type.body_message_class()

    if (request_kind == self.__NO_BODY or
        message_type == message_types.VoidMessage()):
      descriptor['body'] = 'empty'
    else:
      descriptor['body'] = 'autoTemplate(backendRequest)'
      descriptor['bodyName'] = 'resource'
      self.__request_schema[method_id] = self.__parser.add_message(
          message_type.__class__)

    if params:
      descriptor['parameters'] = params

    if param_order:
      descriptor['parameterOrder'] = param_order

    return descriptor

  def __response_message_descriptor(self, message_type, method_id):
    """Describes the response.

    Args:
      message_type: messages.Message class, The message to describe.
      method_id: string, Unique method identifier (e.g. 'myapi.items.method')

    Returns:
      Dictionary describing the response.
    """
    descriptor = {}

    self.__parser.add_message(message_type.__class__)
    if message_type == message_types.VoidMessage():
      descriptor['body'] = 'empty'
    else:
      descriptor['body'] = 'autoTemplate(backendResponse)'
      descriptor['bodyName'] = 'resource'
      self.__response_schema[method_id] = self.__parser.ref_for_message_type(
          message_type.__class__)

    return descriptor

  def __method_descriptor(self, service, method_info,
                          rosy_method, protorpc_method_info):
    """Describes a method.

    Args:
      service: endpoints.Service, Implementation of the API as a service.
      method_info: _MethodInfo, Configuration for the method.
      rosy_method: string, ProtoRPC method name prefixed with the
        name of the service.
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

    descriptor['path'] = method_info.get_path(service.api_info)
    descriptor['httpMethod'] = method_info.http_method
    descriptor['rosyMethod'] = rosy_method
    descriptor['request'] = self.__request_message_descriptor(
        request_kind, request_message_type,
        method_info.method_id(service.api_info),
        descriptor['path'])
    descriptor['response'] = self.__response_message_descriptor(
        remote_method.response_type(), method_info.method_id(service.api_info))

    # Audiences, scopes, allowed_client_ids and auth_level could be set at
    # either the method level or the API level.  Allow an empty list at the
    # method level to override the setting at the API level.
    scopes = (method_info.scopes
              if method_info.scopes is not None
              else service.api_info.scopes)
    if scopes:
      descriptor['scopes'] = scopes
    audiences = (method_info.audiences
                 if method_info.audiences is not None
                 else service.api_info.audiences)
    if audiences:
      descriptor['audiences'] = audiences
    allowed_client_ids = (method_info.allowed_client_ids
                          if method_info.allowed_client_ids is not None
                          else service.api_info.allowed_client_ids)
    if allowed_client_ids:
      descriptor['clientIds'] = allowed_client_ids

    if remote_method.method.__doc__:
      descriptor['description'] = remote_method.method.__doc__

    auth_level = (method_info.auth_level
                  if method_info.auth_level is not None
                  else service.api_info.auth_level)
    if auth_level is not None:
      descriptor['authLevel'] = AUTH_LEVEL.reverse_mapping[auth_level]

    return descriptor

  def __schema_descriptor(self, services):
    """Descriptor for the all the JSON Schema used.

    Args:
      services: List of protorpc.remote.Service instances implementing an
        api/version.

    Returns:
      Dictionary containing all the JSON Schema used in the service.
    """
    methods_desc = {}

    for service in services:
      protorpc_methods = service.all_remote_methods()
      for protorpc_method_name in protorpc_methods.iterkeys():
        rosy_method = '%s.%s' % (service.__name__, protorpc_method_name)
        method_id = self.__id_from_name[rosy_method]

        request_response = {}

        request_schema_id = self.__request_schema.get(method_id)
        if request_schema_id:
          request_response['request'] = {
              '$ref': request_schema_id
              }

        response_schema_id = self.__response_schema.get(method_id)
        if response_schema_id:
          request_response['response'] = {
              '$ref': response_schema_id
              }

        methods_desc[rosy_method] = request_response

    descriptor = {
        'methods': methods_desc,
        'schemas': self.__parser.schemas(),
        }

    return descriptor

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

  def __auth_descriptor(self, api_info):
    """Builds an auth descriptor from API info.

    Args:
      api_info: An _ApiInfo object.

    Returns:
      A dictionary with 'allowCookieAuth' and/or 'blockedRegions' keys.
    """
    if api_info.auth is None:
      return None

    auth_descriptor = {}
    if api_info.auth.allow_cookie_auth is not None:
      auth_descriptor['allowCookieAuth'] = api_info.auth.allow_cookie_auth
    if api_info.auth.blocked_regions:
      auth_descriptor['blockedRegions'] = api_info.auth.blocked_regions

    return auth_descriptor

  def __frontend_limit_descriptor(self, api_info):
    """Builds a frontend limit descriptor from API info.

    Args:
      api_info: An _ApiInfo object.

    Returns:
      A dictionary with frontend limit information.
    """
    if api_info.frontend_limits is None:
      return None

    descriptor = {}
    for propname, descname in (('unregistered_user_qps', 'unregisteredUserQps'),
                               ('unregistered_qps', 'unregisteredQps'),
                               ('unregistered_daily', 'unregisteredDaily')):
      if getattr(api_info.frontend_limits, propname) is not None:
        descriptor[descname] = getattr(api_info.frontend_limits, propname)

    rules = self.__frontend_limit_rules_descriptor(api_info)
    if rules:
      descriptor['rules'] = rules

    return descriptor

  def __frontend_limit_rules_descriptor(self, api_info):
    """Builds a frontend limit rules descriptor from API info.

    Args:
      api_info: An _ApiInfo object.

    Returns:
      A list of dictionaries with frontend limit rules information.
    """
    if not api_info.frontend_limits.rules:
      return None

    rules = []
    for rule in api_info.frontend_limits.rules:
      descriptor = {}
      for propname, descname in (('match', 'match'),
                                 ('qps', 'qps'),
                                 ('user_qps', 'userQps'),
                                 ('daily', 'daily'),
                                 ('analytics_id', 'analyticsId')):
        if getattr(rule, propname) is not None:
          descriptor[descname] = getattr(rule, propname)
      if descriptor:
        rules.append(descriptor)

    return rules

  def __api_descriptor(self, services, hostname=None):
    """Builds a description of an API.

    Args:
      services: List of protorpc.remote.Service instances implementing an
        api/version.
      hostname: string, Hostname of the API, to override the value set on the
        current service. Defaults to None.

    Returns:
      A dictionary that can be deserialized into JSON and stored as an API
      description document.

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

    auth_descriptor = self.__auth_descriptor(merged_api_info)
    if auth_descriptor:
      descriptor['auth'] = auth_descriptor

    frontend_limit_descriptor = self.__frontend_limit_descriptor(
        merged_api_info)
    if frontend_limit_descriptor:
      descriptor['frontendLimits'] = frontend_limit_descriptor

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
        rosy_method = '%s.%s' % (service.__name__, protorpc_meth_name)
        self.__id_from_name[rosy_method] = method_id
        method_map[method_id] = self.__method_descriptor(
            service, method_info, rosy_method, protorpc_meth_info)

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
      descriptor['methods'] = method_map
      descriptor['descriptor'] = self.__schema_descriptor(services)

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
    hostname = (hostname or endpoints_util.get_app_hostname() or
                api_info.hostname)
    protocol = 'http' if ((hostname and hostname.startswith('localhost')) or
                          endpoints_util.is_running_on_devserver()) else 'https'
    base_path = api_info.base_path.strip('/')
    defaults = {
        'extends': 'thirdParty.api',
        'root': '{0}://{1}/{2}'.format(protocol, hostname, base_path),
        'name': api_info.name,
        'version': api_info.version,
        'defaultVersion': True,
        'abstract': False,
        'adapter': {
            'bns': '{0}://{1}/{2}'.format(protocol, hostname, base_path),
            'type': 'lily',
            'deadline': 10.0
        }
    }
    if api_info.canonical_name:
      defaults['canonicalName'] = api_info.canonical_name
    if api_info.owner_domain:
      defaults['ownerDomain'] = api_info.owner_domain
    if api_info.owner_name:
      defaults['ownerName'] = api_info.owner_name
    if api_info.package_path:
      defaults['packagePath'] = api_info.package_path
    if api_info.title:
      defaults['title'] = api_info.title
    if api_info.documentation:
      defaults['documentation'] = api_info.documentation
    return defaults

  def get_config_dict(self, services, hostname=None):
    """JSON dict description of a protorpc.remote.Service in API format.

    Args:
      services: Either a single protorpc.remote.Service or a list of them
        that implements an api/version.
      hostname: string, Hostname of the API, to override the value set on the
        current service. Defaults to None.

    Returns:
      dict, The API descriptor document as a JSON dict.
    """
    if not isinstance(services, (tuple, list)):
      services = [services]
    # The type of a class that inherits from remote.Service is actually
    # remote._ServiceClass, thanks to metaclass strangeness.
    # pylint: disable=protected-access
    endpoints_util.check_list_type(services, remote._ServiceClass, 'services',
                                   allow_none=False)

    return self.__api_descriptor(services, hostname=hostname)

  def pretty_print_config_to_json(self, services, hostname=None):
    """JSON string description of a protorpc.remote.Service in API format.

    Args:
      services: Either a single protorpc.remote.Service or a list of them
        that implements an api/version.
      hostname: string, Hostname of the API, to override the value set on the
        current service. Defaults to None.

    Returns:
      string, The API descriptor document as a JSON string.
    """
    descriptor = self.get_config_dict(services, hostname)
    return json.dumps(descriptor, sort_keys=True, indent=2,
                      separators=(',', ': '))
