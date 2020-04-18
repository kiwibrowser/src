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

"""label_descriptor provides funcs for working with `LabelDescriptor` instances.

:class:`KnownLabels` is an :class:`enum.Enum` that defines the list of known
`LabelDescriptor` instances.  Each enum instance has several fields

- label_name: the name used in the label
- kind: indicates whether the label is system or user label
- value_type: the value type used in the label
- update_label_func: a function to update the labels

"""

from __future__ import absolute_import

import base64
from enum import Enum
from . import messages, USER_AGENT, SERVICE_AGENT

ValueType = messages.LabelDescriptor.ValueTypeValueValuesEnum


class Kind(Enum):
    """Enumerates the known labels."""
    # pylint: disable=too-few-public-methods
    USER = 0
    SYSTEM = 1


_CANONICAL_CODES = {
    200: 0,  # OK
    400: 3,  # INVALID_ARGUMENT
    401: 16,  # UNAUTHENTICATED
    403: 7,  # PERMISSION_DENIED
    404: 5,  # NOT_FOUND
    409: 10,  # ABORTED
    412: 9,  # FAILED_PRECONDITION
    416: 11,  # OUT_OF_RANGE
    429: 8,  # RESOURCE_EXHAUSTED
    499: 1,  # CANCELLED
    500: 13,  # INTERNAL, UNKNOWN
    504: 4,  # DEADLINE_EXCEEDED
    501: 12,  # UNIMPLEMENTED
    503: 14,  # UNAVAILABLE
}


def _canonical_code(http_code):
    mapped_code = _CANONICAL_CODES.get(http_code, 0)
    if mapped_code != 0:
        return mapped_code
    elif 200 <= http_code < 300:
        return 0  # OK
    elif 400 <= http_code < 500:
        return 9  # failed precondition
    elif 500 <= http_code < 600:
        return 13  # internal
    else:
        return 2  # unknown


def set_credential_id(name, info, labels):
    # The rule to set /credential_id is:
    # 1) If api_key is available, set it as apiKey:API-KEY
    # 2) If auth issuer and audience both are available, set it as:
    #    jwtAuth:issuer=base64(issuer)&audience=base64(audience)
    if info.api_key:
        labels[name] = 'apiKey:' + info.api_key
    elif info.auth_issuer:
        value = 'jwtAuth:issuer=' + base64.urlsafe_b64encode(info.auth_issuer)
        if info.auth_audience:
            value += '&audience=' + base64.urlsafe_b64encode(info.auth_audience)
        labels[name] = value


_ERROR_TYPES = tuple('%dxx' % (x,) for x in range(10))


def set_error_type(name, info, labels):
    if info.response_code > 0:
        code = (info.response_code // 100) % 10
        if code < len(_ERROR_TYPES):
            labels[name] = _ERROR_TYPES[code]


def set_protocol(name, info, labels):
    labels[name] = info.protocol.name


def set_referer(name, info, labels):
    if info.referer:
        labels[name] = info.referer


def set_response_code(name, info, labels):
    labels[name] = '%d' % (info.response_code,)


def set_response_code_class(name, info, labels):
    if info.response_code > 0:
        code = (info.response_code // 100) % 10
        if code < len(_ERROR_TYPES):
            labels[name] = _ERROR_TYPES[code]


def set_status_code(name, info, labels):
    if info.response_code > 0:
        labels[name] = '%d' % (_canonical_code(info.response_code),)


def set_location(name, info, labels):
    if info.location:
        labels[name] = info.location


def set_api_method(name, info, labels):
    if info.api_method:
        labels[name] = info.api_method


def set_api_version(name, info, labels):
    if info.api_version:
        labels[name] = info.api_version


def set_platform(name, info, labels):
    labels[name] = info.platform.name


def set_service_agent(name, dummy_info, labels):
    labels[name] = SERVICE_AGENT


def set_user_agent(name, dummy_info, labels):
    labels[name] = USER_AGENT


class KnownLabels(Enum):
    """Enumerates the known labels."""

    CREDENTIAL_ID = (
        '/credential_id', ValueType.STRING, Kind.USER, set_credential_id)
    END_USER = ('/end_user', ValueType.STRING, Kind.USER, None)
    END_USER_COUNTRY = ('/end_user_country', ValueType.STRING, Kind.USER, None)
    ERROR_TYPE = ('/error_type', ValueType.STRING, Kind.USER,
                  set_error_type)
    PROTOCOL = ('/protocol', ValueType.STRING, Kind.USER,
                set_protocol)
    REFERER = ('/referer', ValueType.STRING, Kind.USER,
               set_referer)
    RESPONSE_CODE = ('/response_code', ValueType.STRING, Kind.USER,
                     set_response_code)
    RESPONSE_CODE_CLASS = ('/response_code_class', ValueType.STRING, Kind.USER,
                           set_response_code_class)
    STATUS_CODE = ('/status_code', ValueType.STRING, Kind.USER,
                   set_status_code)
    GAE_CLONE_ID = (
        'appengine.googleapis.com/clone_id', ValueType.STRING, Kind.USER, None)
    GAE_MODULE_ID = (
        'appengine.googleapis.com/module_id', ValueType.STRING, Kind.USER, None)
    GAE_REPLICA_INDEX = (
        'appengine.googleapis.com/replica_index', ValueType.STRING, Kind.USER,
        None)
    GAE_VERSION_ID = (
        'appengine.googleapis.com/version_id', ValueType.STRING, Kind.USER, None)
    GCP_LOCATION = (
        'cloud.googleapis.com/location', ValueType.STRING, Kind.SYSTEM,
        set_location)
    GCP_PROJECT = (
        'cloud.googleapis.com/project', ValueType.STRING, Kind.SYSTEM, None)
    GCP_REGION = (
        'cloud.googleapis.com/region', ValueType.STRING, Kind.SYSTEM, None)
    GCP_RESOURCE_ID = (
        'cloud.googleapis.com/resource_id', ValueType.STRING, Kind.USER, None)
    GCP_RESOURCE_TYPE = (
        'cloud.googleapis.com/resource_type', ValueType.STRING, Kind.USER, None)
    GCP_SERVICE = (
        'cloud.googleapis.com/service', ValueType.STRING, Kind.SYSTEM, None)
    GCP_ZONE = (
        'cloud.googleapis.com/zone', ValueType.STRING, Kind.SYSTEM, None)
    GCP_UID = (
        'cloud.googleapis.com/uid', ValueType.STRING, Kind.SYSTEM, None)
    GCP_API_METHOD = (
        'serviceruntime.googleapis.com/api_method', ValueType.STRING, Kind.USER,
        set_api_method)
    GCP_API_VERSION = (
        'serviceruntime.googleapis.com/api_version', ValueType.STRING, Kind.USER,
        set_api_version)
    SCC_CALLER_IP = (
        'servicecontrol.googleapis.com/caller_ip', ValueType.STRING, Kind.SYSTEM, None)
    SCC_PLATFORM = (
        'servicecontrol.googleapis.com/platform', ValueType.STRING, Kind.SYSTEM,
        set_platform)
    SCC_REFERER = (
        'servicecontrol.googleapis.com/referer', ValueType.STRING, Kind.SYSTEM, None)
    SCC_SERVICE_AGENT = (
        'servicecontrol.googleapis.com/service_agent', ValueType.STRING, Kind.SYSTEM,
        set_service_agent)
    SCC_USER_AGENT = (
        'servicecontrol.googleapis.com/user_agent', ValueType.STRING, Kind.SYSTEM,
        set_user_agent)

    def __init__(self, label_name, value_type, kind, update_label_func):
        """Constructor.

        update_label_func is used when updating a label in an `Operation` from a
        `ReportRequestInfo`.

        Args:
           label_name (str): the name of the label descriptor
           value_type (:class:`ValueType`): the `value type` of the described metric
           kind (:class:`Kind`): the ``kind`` of the described metric
           update_op_func (function): the func to update an operation

        """
        self.label_name = label_name
        self.kind = kind
        self.update_label_func = update_label_func
        self.value_type = value_type

    def matches(self, desc):
        """Determines if a given label descriptor matches this enum instance

        Args:
           desc (:class:`google.api.gen.servicecontrol_v1_messages.LabelDescriptor`): the
              instance to test

        Return:
           `True` if desc is supported, otherwise `False`

        """
        desc_value_type = desc.valueType or ValueType.STRING  # default not parsed
        return (self.label_name == desc.key and
                self.value_type == desc_value_type)

    def do_labels_update(self, info, labels):
        """Updates a dictionary of labels using the assigned update_op_func

        Args:
           info (:class:`google.api.control.report_request.Info`): the
              info instance to update
           labels (dict[string[string]]): the labels dictionary

        Return:
           `True` if desc is supported, otherwise `False`

        """
        if self.update_label_func:
            self.update_label_func(self.label_name, info, labels)

    @classmethod
    def is_supported(cls, desc):
        """Determines if the given label descriptor is supported.

        Args:
           desc (:class:`google.api.gen.servicecontrol_v1_messages.LabelDescriptor`): the
             label descriptor to test

        Return:
           `True` if desc is supported, otherwise `False`

        """
        for l in cls:
            if l.matches(desc):
                return True
        return False
