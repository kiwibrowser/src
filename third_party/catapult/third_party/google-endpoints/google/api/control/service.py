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

"""service provides funcs for working with ``Service`` instances.

:func:`extract_report_spec` obtains objects used to determine what metrics,
labels and logs are included in a report request.

:class:`MethodRegistry` obtains a registry of `MethodInfo` instances from the
data within a `Service` which can then be used to determine which methods get
tracked.

:class:`Loaders` enumerates the different ways in which to obtain a usable
``Service`` instance

"""

from __future__ import absolute_import

import collections
import logging
import os


from apitools.base.py import encoding
from enum import Enum

from . import label_descriptor, metric_descriptor, messages, path_template
from google.api.config import service_config


logger = logging.getLogger(__name__)


CONFIG_VAR = 'ENDPOINTS_SERVICE_CONFIG_FILE'


def _load_from_well_known_env():
    if CONFIG_VAR not in os.environ:
        logger.info('did not load service; no environ var %s', CONFIG_VAR)
        return None
    config_file = os.environ[CONFIG_VAR]
    if not os.path.exists(os.environ[CONFIG_VAR]):
        logger.warn('did not load service; missing config file %s', config_file)
        return None
    try:
        with open(config_file) as f:
            return encoding.JsonToMessage(messages.Service, f.read())
    except ValueError:
        logger.warn('did not load service; bad json config file %s', config_file)
        return None


_SIMPLE_CONFIG = """
{
    "name": "allow-all",
    "http": {
        "rules": [{
            "selector": "allow-all.GET",
            "get": "**"
        }, {
            "selector": "allow-all.POST",
            "post": "**"
        }]
    },
    "usage": {
        "rules": [{
            "selector" : "allow-all.GET",
            "allowUnregisteredCalls" : true
        }, {
            "selector" : "allow-all.POST",
            "allowUnregisteredCalls" : true
        }]
    }
}
"""
_SIMPLE_CORE = encoding.JsonToMessage(messages.Service, _SIMPLE_CONFIG)


def _load_simple():
    return encoding.CopyProtoMessage(_SIMPLE_CORE)


class Loaders(Enum):
    """Enumerates the functions used to load service configs."""
    # pylint: disable=too-few-public-methods
    ENVIRONMENT = (_load_from_well_known_env,)
    SIMPLE = (_load_simple,)
    FROM_SERVICE_MANAGEMENT = (service_config.fetch_service_config,)

    def __init__(self, load_func):
        """Constructor.

        load_func is used to load a service config
        """
        self._load_func = load_func

    def load(self, **kw):
        return self._load_func(**kw)


class MethodRegistry(object):
    """Provides a registry of the api methods defined by a ``Service``.

    During construction, ``MethodInfo`` instances are extracted from a
    ``Service``.  The are subsequently accessible via the :func:`lookup` method.

    """
    # pylint: disable=too-few-public-methods
    _OPTIONS = 'OPTIONS'

    def __init__(self, service):
        """Constructor.

        Args:
          service (:class:`google.api.gen.servicecontrol_v1_messages.Service`):
            a service instance
        """
        if not isinstance(service, messages.Service):
            raise ValueError('service should be an instance of Service')
        if not service.name:
            raise ValueError('Bad service: the name is missing')

        self._service = service  # the service that provides the methods
        self._extracted_methods = {}  # tracks all extracted_methods by selector

        self._auth_infos = self._extract_auth_config()

        # tracks urls templates
        self._templates_method_infos = collections.defaultdict(list)
        self._extract_methods()

    def lookup(self, http_method, path):
        http_method = http_method.lower()
        if path.startswith('/'):
            path = path[1:]
        tmi = self._templates_method_infos.get(http_method)
        if not tmi:
            logger.debug('No methods for http method %s in %s',
                         http_method,
                         self._templates_method_infos.keys())
            return None

        # pylint: disable=fixme
        # TODO: speed this up if it proves to be bottleneck.
        #
        # There is sophisticated trie-based solution in esp, something similar
        # could be built around the path_template implementation
        for template, method_info in tmi:
            logger.debug('trying %s with template %s', path, template)
            try:
                template.match(path)
                logger.debug('%s matched template %s', path, template)
                return method_info
            except path_template.ValidationException:
                logger.debug('%s did not match template %s', path, template)
                continue

        return None

    def _extract_auth_config(self):
        """Obtains the authentication configurations."""

        service = self._service
        if not service.authentication:
            return {}

        auth_infos = {}
        for auth_rule in service.authentication.rules:
            selector = auth_rule.selector
            provider_ids_to_audiences = {}
            for requirement in auth_rule.requirements:
                provider_id = requirement.providerId
                if provider_id and requirement.audiences:
                    audiences = requirement.audiences.split(",")
                    provider_ids_to_audiences[provider_id] = audiences
            auth_infos[selector] = AuthInfo(provider_ids_to_audiences)
        return auth_infos

    def _extract_methods(self):
        """Obtains the methods used in the service."""
        service = self._service
        all_urls = set()
        urls_with_options = set()
        if not service.http:
            return
        for rule in service.http.rules:
            http_method, url = _detect_pattern_option(rule)
            if not url or not http_method or not rule.selector:
                logger.error('invalid HTTP binding encountered')
                continue

            # Obtain the method info
            method_info = self._get_or_create_method_info(rule.selector)
            if rule.body:
                method_info.body_field_path = rule.body
            if not self._register(http_method, url, method_info):
                continue  # detected an invalid url
            all_urls.add(url)

            if http_method == self._OPTIONS:
                urls_with_options.add(url)

        self._add_cors_options_selectors(all_urls - urls_with_options)
        self._update_usage()
        self._update_system_parameters()

    def _register(self, http_method, url, method_info):
        try:
            http_method = http_method.lower()
            template = path_template.PathTemplate(url)
            self._templates_method_infos[http_method].append((template, method_info))
            logger.debug('Registered template %s under method %s',
                         template,
                         http_method)
            return True
        except path_template.ValidationException:
            logger.error('invalid HTTP template provided: %s', url)
            return False

    def _update_usage(self):
        extracted_methods = self._extracted_methods
        service = self._service
        if not service.usage:
            return
        for rule in service.usage.rules:
            selector = rule.selector
            method = extracted_methods.get(selector)
            if method:
                method.allow_unregistered_calls = rule.allowUnregisteredCalls
            else:
                logger.error('bad usage selector: No HTTP rule for %s', selector)

    def _get_or_create_method_info(self, selector):
        extracted_methods = self._extracted_methods
        info = self._extracted_methods.get(selector)
        if info:
            return info

        auth_infos = self._auth_infos
        auth_info = auth_infos[selector] if selector in auth_infos else None

        info = MethodInfo(selector, auth_info)
        extracted_methods[selector] = info
        return info

    def _add_cors_options_selectors(self, urls):
        extracted_methods = self._extracted_methods
        base_selector = '%s.%s' % (self._service.name, self._OPTIONS)

        # ensure that no existing options selector is being used
        options_selector = base_selector
        n = 0
        while extracted_methods.get(options_selector) is not None:
            n += 1
            options_selector = '%s.%d' % (base_selector, n)
        method_info = self._get_or_create_method_info(options_selector)
        method_info.allow_unregistered_calls = True
        for u in urls:
            self._register(self._OPTIONS, u, method_info)

    def _update_system_parameters(self):
        extracted_methods = self._extracted_methods
        service = self._service
        if not service.systemParameters:
            return
        rules = service.systemParameters.rules
        for rule in rules:
            selector = rule.selector
            method = extracted_methods.get(selector)
            if not method:
                logger.error('bad system parameter: No HTTP rule for %s',
                             selector)
                continue

            for parameter in rule.parameters:
                name = parameter.name
                if not name:
                    logger.error('bad system parameter: no parameter name %s',
                                 selector)
                    continue

                if parameter.httpHeader:
                    method.add_header_param(name, parameter.httpHeader)
                if parameter.urlQueryParameter:
                    method.add_url_query_param(name, parameter.urlQueryParameter)


class AuthInfo(object):
    """Consolidates auth information about methods defined in a ``Service``."""

    def __init__(self, provider_ids_to_audiences):
        """Construct an AuthInfo instance.

        Args:
          provider_ids_to_audiences: a dictionary that maps from provider ids
            to allowed audiences.
        """
        self._provider_ids_to_audiences = provider_ids_to_audiences

    def is_provider_allowed(self, provider_id):
        return provider_id in self._provider_ids_to_audiences

    def get_allowed_audiences(self, provider_id):
        return self._provider_ids_to_audiences.get(provider_id, [])


class MethodInfo(object):
    """Consolidates information about methods defined in a ``Service``."""
    API_KEY_NAME = 'api_key'
    # pylint: disable=too-many-instance-attributes

    def __init__(self, selector, auth_info):
        self.selector = selector
        self.auth_info = auth_info
        self.allow_unregistered_calls = False
        self.backend_address = ''
        self.body_field_path = ''
        self._url_query_parameters = collections.defaultdict(list)
        self._header_parameters = collections.defaultdict(list)

    def add_url_query_param(self, name, parameter):
        self._url_query_parameters[name].append(parameter)

    def add_header_param(self, name, parameter):
        self._header_parameters[name].append(parameter)

    def url_query_param(self, name):
        return tuple(self._url_query_parameters[name])

    def header_param(self, name):
        return tuple(self._header_parameters[name])

    @property
    def api_key_http_header(self):
        return self.header_param(self.API_KEY_NAME)

    @property
    def api_key_url_query_params(self):
        return self.url_query_param(self.API_KEY_NAME)


def extract_report_spec(
        service,
        label_is_supported=label_descriptor.KnownLabels.is_supported,
        metric_is_supported=metric_descriptor.KnownMetrics.is_supported):
    """Obtains the used logs, metrics and labels from a service.

    label_is_supported and metric_is_supported are filter functions used to
    determine if label_descriptors or metric_descriptors found in the service
    are supported.

    Args:
       service (:class:`google.api.gen.servicecontrol_v1_messages.Service`):
          a service instance
       label_is_supported (:func): determines if a given label is supported
       metric_is_supported (:func): determines if a given metric is supported

    Return:
       tuple: (
         logs (set[string}), # the logs to report to
         metrics (list[string]), # the metrics to use
         labels (list[string]) # the labels to add
       )
    """
    resource_descs = service.monitoredResources
    labels_dict = {}
    logs = set()
    if service.logging:
        logs = _add_logging_destinations(
            service.logging.producerDestinations,
            resource_descs,
            service.logs,
            labels_dict,
            label_is_supported
        )
    metrics_dict = {}
    monitoring = service.monitoring
    if monitoring:
        for destinations in (monitoring.consumerDestinations,
                             monitoring.producerDestinations):
            _add_monitoring_destinations(destinations,
                                         resource_descs,
                                         service.metrics,
                                         metrics_dict,
                                         metric_is_supported,
                                         labels_dict,
                                         label_is_supported)
    return logs, metrics_dict.keys(), labels_dict.keys()


def _add_logging_destinations(destinations,
                              resource_descs,
                              log_descs,
                              labels_dict,
                              is_supported):
    all_logs = set()
    for d in destinations:
        if not _add_labels_for_a_monitored_resource(resource_descs,
                                                    d.monitoredResource,
                                                    labels_dict,
                                                    is_supported):
            continue  # skip bad monitored resources
        for log in d.logs:
            if _add_labels_for_a_log(log_descs, log, labels_dict, is_supported):
                all_logs.add(log)  # only add correctly configured logs
    return all_logs


def _add_monitoring_destinations(destinations,
                                 resource_descs,
                                 metric_descs,
                                 metrics_dict,
                                 metric_is_supported,
                                 labels_dict,
                                 label_is_supported):
    # pylint: disable=too-many-arguments
    for d in destinations:
        if not _add_labels_for_a_monitored_resource(resource_descs,
                                                    d.monitoredResource,
                                                    labels_dict,
                                                    label_is_supported):
            continue  # skip bad monitored resources
        for metric_name in d.metrics:
            metric_desc = _find_metric_descriptor(metric_descs, metric_name,
                                                  metric_is_supported)
            if not metric_desc:
                continue  # skip unrecognized or unsupported metric
            if not _add_labels_from_descriptors(metric_desc.labels, labels_dict,
                                                label_is_supported):
                continue  # skip metrics with bad labels
            metrics_dict[metric_name] = metric_desc


def _add_labels_from_descriptors(descs, labels_dict, is_supported):
    # only add labels if there are no conflicts
    for desc in descs:
        existing = labels_dict.get(desc.key)
        if existing and existing.valueType != desc.valueType:
            logger.warn('halted label scan: conflicting label in %s', desc.key)
            return False
    # Update labels_dict
    for desc in descs:
        if is_supported(desc):
            labels_dict[desc.key] = desc
    return True


def _add_labels_for_a_log(logging_descs, log_name, labels_dict, is_supported):
    for d in logging_descs:
        if d.name == log_name:
            _add_labels_from_descriptors(d.labels, labels_dict, is_supported)
            return True
    logger.warn('bad log label scan: log not found %s', log_name)
    return False


def _add_labels_for_a_monitored_resource(resource_descs,
                                         resource_name,
                                         labels_dict,
                                         is_supported):
    for d in resource_descs:
        if d.type == resource_name:
            _add_labels_from_descriptors(d.labels, labels_dict, is_supported)
            return True
    logger.warn('bad monitored resource label scan: resource not found %s',
                resource_name)
    return False


def _find_metric_descriptor(metric_descs, name, metric_is_supported):
    for d in metric_descs:
        if name != d.name:
            continue
        if metric_is_supported(d):
            return d
        else:
            return None
    return None


# This is derived from the oneof choices of the HttpRule message's pattern
# field in google/api/http.proto, and should be kept in sync with that
_HTTP_RULE_ONE_OF_FIELDS = (
    'get', 'put', 'post', 'delete', 'patch', 'custom')


def _detect_pattern_option(http_rule):
    for f in _HTTP_RULE_ONE_OF_FIELDS:
        value = http_rule.get_assigned_value(f)
        if value is not None:
            if f == 'custom':
                return value.kind, value.path
            else:
                return f, value
    return None, None
