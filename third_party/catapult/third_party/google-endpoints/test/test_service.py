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

from __future__ import absolute_import

import datetime
import json
import os
import tempfile
import unittest2

from apitools.base.py import encoding
from expects import be_false, be_none, be_true, expect, equal, raise_error

from google.api.control import messages, service


_LOGGING_DESTINATIONS_INPUT = """
{
  "logs": [{
    "name": "endpoints-log",
    "labels": [{
      "key": "supported/endpoints-log-label"
    }, {
      "key": "unsupported/endpoints-log-label"
    }]
  }, {
    "name": "unreferenced-log",
    "labels": [{
      "key": "supported/unreferenced-log-label"
    }]
  }],

  "monitoredResources": [{
    "type": "endpoints.googleapis.com/endpoints",
    "labels": [{
      "key": "unsupported/endpoints"
    }, {
      "key": "supported/endpoints"
    }]
  }],

  "logging": {
    "producerDestinations": [{
      "monitoredResource": "bad-monitored-resource",
      "logs": [
        "bad-monitored-resource-log"
      ]
    }, {
      "monitoredResource": "endpoints.googleapis.com/endpoints",
      "logs": [
        "bad-endpoints-log",
        "endpoints-log"
      ]
    }]
  }
}

"""


class _JsonServiceBase(object):

    def setUp(self):
        self._subject = encoding.JsonToMessage(messages.Service, self._INPUT)

    def _extract(self):
        return service.extract_report_spec(
            self._subject,
            label_is_supported=fake_is_label_supported,
            metric_is_supported=fake_is_metric_supported
        )

    def _get_registry(self):
        return service.MethodRegistry(self._subject)


class TestLoggingDestinations(_JsonServiceBase, unittest2.TestCase):
    _INPUT = _LOGGING_DESTINATIONS_INPUT
    _WANTED_LABELS = [
        'supported/endpoints-log-label',
        'supported/endpoints'
    ]

    def test_should_access_the_valid_referenced_log(self):
        logs, _metrics, _labels =  self._extract()
        expect(logs).to(equal(set(['endpoints-log'])))

    def test_should_not_specify_any_metrics(self):
        _logs, metrics, _labels =  self._extract()
        expect(metrics).to(equal([]))

    def test_should_specify_the_labels_associated_with_the_valid_log(self):
        _logs, _metrics, labels =  self._extract()
        expect(set(labels)).to(equal(set(self._WANTED_LABELS)))

    def test_should_drop_conflicting_log_labels(self):
        conflicting_label = messages.LabelDescriptor(
            key='supported/endpoints-log-label',
            valueType=messages.LabelDescriptor.ValueTypeValueValuesEnum.BOOL
        )
        bad_log_desc = messages.LogDescriptor(
            name='bad-endpoints-log',
            labels=[conflicting_label]
        )
        self._subject.logs.append(bad_log_desc)
        _logs, _metrics, labels =  self._extract()
        expect(set(labels)).to(equal(set(self._WANTED_LABELS)))



_METRIC_DESTINATIONS_INPUTS = """
{
  "metrics": [{
    "name": "supported/endpoints-metric",
    "labels": [{
      "key": "supported/endpoints-metric-label"
    }, {
      "key": "unsupported/endpoints-metric-label"
    }]
  }, {
    "name": "unsupported/unsupported-endpoints-metric",
    "labels": [{
      "key": "supported/unreferenced-metric-label"
    }]
  }, {
    "name": "supported/non-existent-resource-metric",
    "labels": [{
      "key": "supported/non-existent-resource-metric-label"
    }]
  }],

  "monitoredResources": {
    "type": "endpoints.googleapis.com/endpoints",
    "labels": [{
      "key": "unsupported/endpoints"
    }, {
      "key": "supported/endpoints"
    }]
  },

  "monitoring": {
    "consumerDestinations": [{
      "monitoredResource": "endpoints.googleapis.com/endpoints",
      "metrics": [
        "supported/endpoints-metric",
        "unsupported/unsupported-endpoints-metric",
        "supported/unknown-metric"
      ]
    }, {
      "monitoredResource": "endpoints.googleapis.com/non-existent",
      "metrics": [
         "supported/endpoints-metric",
         "unsupported/unsupported-endpoints-metric",
         "supported/unknown-metric",
         "supported/non-existent-resource-metric"
      ]
    }]
  }
}

"""

class TestMetricDestinations(_JsonServiceBase, unittest2.TestCase):
    _INPUT = _METRIC_DESTINATIONS_INPUTS
    _WANTED_METRICS = [
        'supported/endpoints-metric'
    ]
    _WANTED_LABELS = [
        'supported/endpoints-metric-label',
        'supported/endpoints'
    ]

    def test_should_not_load_any_logs(self):
        logs, _metrics, _labels =  self._extract()
        expect(logs).to(equal(set()))

    def test_should_specify_some_metrics(self):
        _logs, metrics, _labels =  self._extract()
        expect(metrics).to(equal(self._WANTED_METRICS))

    def test_should_specify_the_labels_associated_with_the_metrics(self):
        _logs, _metrics, labels =  self._extract()
        expect(set(labels)).to(equal(set(self._WANTED_LABELS)))


_NOT_SUPPORTED_PREFIX = 'unsupported/'


_COMBINED_LOG_METRIC_LABEL_INPUTS = """
{
  "logs": {
    "name": "endpoints-log",
    "labels": [{
      "key": "supported/endpoints-log-label"
    }, {
      "key": "unsupported/endpoints-log-label"
    }]
  },

  "metrics": [{
    "name": "supported/endpoints-metric",
    "labels": [{
      "key": "supported/endpoints-metric-label"
    }, {
      "key": "unsupported/endpoints-metric-label"
    }]
  }, {
    "name": "supported/endpoints-consumer-metric",
    "labels": [{
      "key": "supported/endpoints-metric-label"
    }, {
      "key": "supported/endpoints-consumer-metric-label"
    }]
  }, {
    "name": "supported/endpoints-producer-metric",
    "labels": [{
      "key": "supported/endpoints-metric-label"
    }, {
      "key": "supported/endpoints-producer-metric-label"
    }]
  }],

  "monitoredResources": {
    "type": "endpoints.googleapis.com/endpoints",
    "labels": [{
      "key": "unsupported/endpoints"
    }, {
      "key": "supported/endpoints"
    }]
  },

  "logging": {
    "producerDestinations": [{
      "monitoredResource": "endpoints.googleapis.com/endpoints",
      "logs": ["endpoints-log"]
    }]
  },

  "monitoring": {
    "consumerDestinations": [{
      "monitoredResource": "endpoints.googleapis.com/endpoints",
      "metrics": [
         "supported/endpoints-consumer-metric",
         "supported/endpoints-metric"
      ]
    }],

    "producerDestinations": [{
      "monitoredResource": "endpoints.googleapis.com/endpoints",
      "metrics": [
         "supported/endpoints-producer-metric",
         "supported/endpoints-metric"
      ]
    }]
  }
}

"""

class TestCombinedExtraction(_JsonServiceBase, unittest2.TestCase):
    _INPUT = _COMBINED_LOG_METRIC_LABEL_INPUTS
    _WANTED_METRICS = [
        "supported/endpoints-metric",
        "supported/endpoints-consumer-metric",
        "supported/endpoints-producer-metric"
    ]
    _WANTED_LABELS = [
        "supported/endpoints",  # from monitored resource
        "supported/endpoints-log-label",  # from log
        "supported/endpoints-metric-label",  # from both metrics
        "supported/endpoints-consumer-metric-label",  # from consumer metric
        "supported/endpoints-producer-metric-label"  # from producer metric
    ]

    def test_should_load_the_specified_logs(self):
        logs, _metrics, _labels =  self._extract()
        expect(logs).to(equal(set(['endpoints-log'])))

    def test_should_load_the_specified_metrics(self):
        _logs, metrics, _labels =  self._extract()
        expect(set(metrics)).to(equal(set(self._WANTED_METRICS)))

    def test_should_load_the_specified_metrics(self):
        _logs, _metrics, labels =  self._extract()
        expect(set(labels)).to(equal(set(self._WANTED_LABELS)))


def fake_is_label_supported(label_desc):
    return not label_desc.key.startswith(_NOT_SUPPORTED_PREFIX)


def fake_is_metric_supported(metric_desc):
    return not metric_desc.name.startswith(_NOT_SUPPORTED_PREFIX)


_NO_NAME_SERVICE_CONFIG_TEST = """
{
    "name": ""
}
"""

class TestBadServiceConfig(_JsonServiceBase, unittest2.TestCase):
    _INPUT = _NO_NAME_SERVICE_CONFIG_TEST

    def test_should_fail_if_service_is_bad(self):
        testfs = [
            lambda: self._get_registry(),
            lambda: service.MethodRegistry(None),
            lambda: service.MethodRegistry(object()),
        ]
        for f in testfs:
            expect(f).to(raise_error(ValueError))


_EMPTY_SERVICE_CONFIG_TEST = """
{
    "name": "empty"
}
"""

class TestEmptyServiceConfig(_JsonServiceBase, unittest2.TestCase):
    _INPUT = _EMPTY_SERVICE_CONFIG_TEST

    def test_should_obtain_a_registry(self):
        expect(self._get_registry()).not_to(be_none)

    def test_lookup_should_return_none(self):
        test_verbs = ('GET', 'POST')
        registry = self._get_registry()
        for v in test_verbs:
            info = registry.lookup('GET', 'any_url')
            expect(info).to(be_none)


_BAD_HTTP_RULE_CONFIG_TEST = """
{
    "name": "bad-http-rule",
    "http": {
        "rules": [{
            "selector": "Uvw.BadRule.DoubleWildCard",
            "get": "/uvw/not_present/**/**"
        }, {
            "selector": "Uvw.BadRule.NoMethod"
        }, {
            "get": "/uvv/bad_rule/no_selector"
        },{
            "selector": "Uvw.OkRule",
            "get": "/uvw/ok_rule/*"
        }]
    }
}
"""

class TestBadHttpRuleServiceConfig(_JsonServiceBase, unittest2.TestCase):
    _INPUT = _BAD_HTTP_RULE_CONFIG_TEST

    def test_lookup_should_return_none_for_unknown_uri(self):
        test_verbs = ('GET', 'POST')
        registry = self._get_registry()
        for v in test_verbs:
            expect(registry.lookup(v, 'uvw/unknown_url')).to(be_none)

    def test_lookup_should_return_none_for_potential_bad_url_match(self):
        registry = self._get_registry()
        expect(registry.lookup('GET', '/uvw/not_present/is_bad')).to(be_none)
        expect(registry.lookup('GET', '/uvw/bad_rule/no_selector')).to(be_none)
        expect(registry.lookup('GET', 'uvw/not_present/is_bad')).to(be_none)
        expect(registry.lookup('GET', 'uvw/not_present/is_bad')).to(be_none)


_USAGE_CONFIG_TEST = """
{
    "name": "usage-config",
    "usage": {
        "rules": [{
            "selector" : "Uvw.Method1",
            "allowUnregisteredCalls" : true
        }, {
            "selector" : "Uvw.Method2",
            "allowUnregisteredCalls" : false
        }, {
            "selector" : "Uvw.IgnoredMethod",
            "allowUnregisteredCalls" : false
        }]
    },
    "http": {
        "rules": [{
            "selector": "Uvw.Method1",
            "get": "/uvw/method1/*"
        }, {
            "selector": "Uvw.Method2",
            "get": "/uvw/method2/*"
        }, {
            "selector": "Uvw.DefaultUsage",
            "get": "/uvw/default_usage"
        }]
    }
}
"""

class TestMethodRegistryUsageConfig(_JsonServiceBase, unittest2.TestCase):
    _INPUT = _USAGE_CONFIG_TEST

    def test_should_detect_with_unregistered_calls_are_allowed(self):
        registry = self._get_registry()
        info = registry.lookup('GET', 'uvw/method1/abc')
        expect(info).not_to(be_none)
        expect(info.selector).to(equal('Uvw.Method1'))
        expect(info.allow_unregistered_calls).to(be_true)

    def test_should_detect_when_unregistered_calls_are_not_allowed(self):
        registry = self._get_registry()
        info = registry.lookup('GET', 'uvw/method2/abc')
        expect(info).not_to(be_none)
        expect(info.selector).to(equal('Uvw.Method2'))
        expect(info.allow_unregistered_calls).to(be_false)

    def test_should_default_to_disallowing_unregistered_calls(self):
        registry = self._get_registry()
        info = registry.lookup('GET', 'uvw/default_usage')
        expect(info).not_to(be_none)
        expect(info.selector).to(equal('Uvw.DefaultUsage'))
        expect(info.allow_unregistered_calls).to(be_false)


_SYSTEM_PARAMETER_CONFIG_TEST = """
{
    "name": "system-parameter-config",
    "systemParameters": {
       "rules": [{
         "selector": "Uvw.Method1",
         "parameters": [{
            "name": "name1",
            "httpHeader": "Header-Key1",
            "urlQueryParameter": "param_key1"
         }, {
            "name": "name2",
            "httpHeader": "Header-Key2",
            "urlQueryParameter": "param_key2"
         }, {
            "name": "api_key",
            "httpHeader": "ApiKeyHeader",
            "urlQueryParameter": "ApiKeyParam"
         }, {
            "httpHeader": "Ignored-NoName-Key3",
            "urlQueryParameter": "Ignored-NoName-key3"
         }]
       }, {
         "selector": "Bad.NotConfigured",
         "parameters": [{
            "name": "neverUsed",
            "httpHeader": "NeverUsed-Key1",
            "urlQueryParameter": "NeverUsed_key1"
         }]
       }]
    },
    "http": {
        "rules": [{
            "selector": "Uvw.Method1",
            "get": "/uvw/method1/*"
        }, {
            "selector": "Uvw.DefaultParameters",
            "get": "/uvw/default_parameters"
        }]
    }
}
"""


class TestMethodRegistrySystemParameterConfig(_JsonServiceBase, unittest2.TestCase):
    _INPUT = _SYSTEM_PARAMETER_CONFIG_TEST

    def test_should_detect_registered_system_parameters(self):
        registry = self._get_registry()
        info = registry.lookup('GET', 'uvw/method1/abc')
        expect(info).not_to(be_none)
        expect(info.selector).to(equal('Uvw.Method1'))
        expect(info.url_query_param('name1')).to(equal(('param_key1',)))
        expect(info.url_query_param('name2')).to(equal(('param_key2',)))
        expect(info.header_param('name1')).to(equal(('Header-Key1',)))
        expect(info.header_param('name2')).to(equal(('Header-Key2',)))

    def test_should_detect_default_api_key(self):
        registry = self._get_registry()
        info = registry.lookup('GET', 'uvw/method1/abc')
        expect(info).not_to(be_none)
        expect(info.api_key_http_header).to(equal(('ApiKeyHeader',)))
        expect(info.api_key_url_query_params).to(equal(('ApiKeyParam',)))

    def test_should_find_nothing_for_unregistered_params(self):
        registry = self._get_registry()
        info = registry.lookup('GET', 'uvw/method1/abc')
        expect(info.url_query_param('name3')).to(equal(tuple()))
        expect(info.header_param('name3')).to(equal(tuple()))

    def test_should_detect_nothing_for_methods_with_no_registration(self):
        registry = self._get_registry()
        info = registry.lookup('GET', 'uvw/default_parameters')
        expect(info).not_to(be_none)
        expect(info.selector).to(equal('Uvw.DefaultParameters'))
        expect(info.url_query_param('name1')).to(equal(tuple()))
        expect(info.url_query_param('name2')).to(equal(tuple()))
        expect(info.header_param('name1')).to(equal(tuple()))
        expect(info.header_param('name2')).to(equal(tuple()))


_BOOKSTORE_CONFIG_TEST = """
{
    "name": "bookstore-http-api",
    "http": {
        "rules": [{
            "selector": "Bookstore.ListShelves",
            "get": "/shelves"
        }, {
            "selector": "Bookstore.CorsShelves",
            "custom": {
               "kind": "OPTIONS",
               "path": "shelves"
            }
        }, {
            "selector": "Bookstore.ListBooks",
            "get": "/shelves/{shelf=*}/books"
        },{
            "selector": "Bookstore.CreateBook",
            "post": "/shelves/{shelf=*}/books",
            "body": "book"
        }]
    }
}
"""

class TestMethodRegistryBookstoreConfig(_JsonServiceBase, unittest2.TestCase):
    _INPUT = _BOOKSTORE_CONFIG_TEST

    def test_configures_list_shelves_ok(self):
        registry = self._get_registry()
        info = registry.lookup('GET', '/shelves')
        expect(info).not_to(be_none)
        expect(info.selector).to(equal('Bookstore.ListShelves'))
        expect(info.body_field_path).to(equal(''))

    def test_configures_options_shelves_ok(self):
        registry = self._get_registry()
        info = registry.lookup('OPTIONS', '/shelves')
        expect(info).not_to(be_none)
        expect(info.selector).to(equal('Bookstore.CorsShelves'))

    def test_configures_list_books_ok(self):
        registry = self._get_registry()
        info = registry.lookup('GET', '/shelves/88/books')
        expect(info).not_to(be_none)
        expect(info.selector).to(equal('Bookstore.ListBooks'))
        expect(info.body_field_path).to(equal(''))

    def test_configures_create_book_ok(self):
        registry = self._get_registry()
        info = registry.lookup('POST', '/shelves/88/books')
        expect(info).not_to(be_none)
        expect(info.selector).to(equal('Bookstore.CreateBook'))
        expect(info.body_field_path).to(equal('book'))


_OPTIONS_SELECTOR_CONFIG_TEST = """
{
    "name": "options-selector",
    "http": {
        "rules": [{
            "selector": "options-selector.OPTIONS",
            "get": "/shelves"
        }, {
            "selector": "options-selector.OPTIONS.1",
            "get": "/shelves/{shelf}"
        }]
    }
}
"""

class TestOptionsSelectorConfig(_JsonServiceBase, unittest2.TestCase):
    _INPUT = _OPTIONS_SELECTOR_CONFIG_TEST

    def test_should_options_to_be_updated(self):
        registry = self._get_registry()
        info = registry.lookup('OPTIONS', '/shelves')
        expect(info).not_to(be_none)
        expect(info.selector).to(equal('options-selector.OPTIONS.2'))


class TestSimpleLoader(unittest2.TestCase):

    def test_should_load_service_ok(self):
        loaded = service.Loaders.SIMPLE.load()
        registry = service.MethodRegistry(loaded)
        info = registry.lookup('GET', '/anything')
        expect(info).not_to(be_none)
        info = registry.lookup('POST', '/anything')
        expect(info).not_to(be_none)


class TestEnvironmentLoader(unittest2.TestCase):

    def setUp(self):
        _config_fd = tempfile.NamedTemporaryFile(delete=False)
        with _config_fd as f:
            f.write(_BOOKSTORE_CONFIG_TEST)
        self._config_file = _config_fd.name
        os.environ[service.CONFIG_VAR] = self._config_file

    def tearDown(self):
        if os.path.exists(self._config_file):
            os.remove(self._config_file)

    def test_does_not_load_if_env_is_not_set(self):
        del os.environ[service.CONFIG_VAR]
        loaded = service.Loaders.ENVIRONMENT.load()
        expect(loaded).to(be_none)

    def test_does_not_load_if_file_is_missing(self):
        os.remove(self._config_file)
        loaded = service.Loaders.ENVIRONMENT.load()
        expect(loaded).to(be_none)

    def test_does_not_load_if_config_is_bad(self):
        with open(self._config_file, 'w') as f:
            f.write('this is not json {')
        loaded = service.Loaders.ENVIRONMENT.load()
        expect(loaded).to(be_none)

    def test_should_load_service_ok(self):
        loaded = service.Loaders.ENVIRONMENT.load()
        expect(loaded).not_to(be_none)
        registry = service.MethodRegistry(loaded)
        info = registry.lookup('GET', '/shelves')
        expect(info).not_to(be_none)


_AUTHENTICATION_CONFIG_TEST = """
{
    "name": "authentication-config",
    "authentication": {
        "rules": [{
            "selector": "Bookstore.ListShelves",
            "requirements": [{
                "providerId": "shelves-provider",
                "audiences": "aud1,aud2"
            }]
        }]
    },
    "http": {
        "rules": [{
            "selector": "Bookstore.ListShelves",
            "get": "/shelves"
        }, {
            "selector": "Bookstore.CorsShelves",
            "custom": {
               "kind": "OPTIONS",
               "path": "shelves"
            }
        }, {
            "selector": "Bookstore.ListBooks",
            "get": "/shelves/{shelf=*}/books"
        },{
            "selector": "Bookstore.CreateBook",
            "post": "/shelves/{shelf=*}/books",
            "body": "book"
        }]
    }
}
"""

class TestAuthenticationConfig(_JsonServiceBase, unittest2.TestCase):
    _INPUT = _AUTHENTICATION_CONFIG_TEST

    def test_lookup_method_with_authentication(self):
      registry = self._get_registry()
      info = registry.lookup('GET', '/shelves')
      auth_info = info.auth_info
      self.assertIsNotNone(auth_info)
      self.assertTrue(auth_info.is_provider_allowed("shelves-provider"))
      self.assertFalse(auth_info.is_provider_allowed("random-provider"))
      self.assertEqual(["aud1", "aud2"],
                       auth_info.get_allowed_audiences("shelves-provider"))
      self.assertEqual([], auth_info.get_allowed_audiences("random-provider"))

    def test_lookup_method_without_authentication(self):
      registry = self._get_registry()
      info = registry.lookup('OPTIONS', '/shelves')
      self.assertIsNotNone(info)
      self.assertIsNone(info.auth_info)
