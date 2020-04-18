#
# Copyright 2015 Google Inc.
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

import re

import mock
import six
from six.moves import http_client
import unittest2

from apitools.base.py import credentials_lib
from apitools.base.py import util


def CreateUriValidator(uri_regexp, content=''):
    def CheckUri(uri, headers=None):
        if 'X-Google-Metadata-Request' not in headers:
            raise ValueError('Missing required header')
        if uri_regexp.match(uri):
            message = content
            status = http_client.OK
        else:
            message = 'Expected uri matching pattern %s' % uri_regexp.pattern
            status = http_client.BAD_REQUEST
        return type('HttpResponse', (object,), {'status': status})(), message
    return CheckUri


class CredentialsLibTest(unittest2.TestCase):

    def _GetServiceCreds(self, service_account_name=None, scopes=None):
        kwargs = {}
        if service_account_name is not None:
            kwargs['service_account_name'] = service_account_name
        service_account_name = service_account_name or 'default'

        def MockMetadataCalls(request_url):
            default_scopes = scopes or ['scope1']
            if request_url.endswith('scopes'):
                return six.StringIO(''.join(default_scopes))
            elif request_url.endswith('service-accounts'):
                return six.StringIO(service_account_name)
            elif request_url.endswith(
                    '/service-accounts/%s/token' % service_account_name):
                return six.StringIO('{"access_token": "token"}')
            self.fail('Unexpected HTTP request to %s' % request_url)

        with mock.patch.object(credentials_lib, '_GceMetadataRequest',
                               side_effect=MockMetadataCalls,
                               autospec=True) as opener_mock:
            with mock.patch.object(util, 'DetectGce',
                                   autospec=True) as mock_detect:
                mock_detect.return_value = True
                validator = CreateUriValidator(
                    re.compile(r'.*/%s/.*' % service_account_name),
                    content='{"access_token": "token"}')
                credentials = credentials_lib.GceAssertionCredentials(
                    scopes, **kwargs)
                self.assertIsNone(credentials._refresh(validator))
            self.assertEqual(3, opener_mock.call_count)

    def testGceServiceAccounts(self):
        scopes = ['scope1']
        self._GetServiceCreds()
        self._GetServiceCreds(scopes=scopes)
        self._GetServiceCreds(service_account_name='my_service_account',
                              scopes=scopes)


class TestGetRunFlowFlags(unittest2.TestCase):

    def setUp(self):
        self._flags_actual = credentials_lib.FLAGS

    def tearDown(self):
        credentials_lib.FLAGS = self._flags_actual

    def test_with_gflags(self):
        HOST = 'myhostname'
        PORT = '144169'

        class MockFlags(object):
            auth_host_name = HOST
            auth_host_port = PORT
            auth_local_webserver = False

        credentials_lib.FLAGS = MockFlags
        flags = credentials_lib._GetRunFlowFlags([
            '--auth_host_name=%s' % HOST,
            '--auth_host_port=%s' % PORT,
            '--noauth_local_webserver',
        ])
        self.assertEqual(flags.auth_host_name, HOST)
        self.assertEqual(flags.auth_host_port, PORT)
        self.assertEqual(flags.logging_level, 'ERROR')
        self.assertEqual(flags.noauth_local_webserver, True)

    def test_without_gflags(self):
        credentials_lib.FLAGS = None
        flags = credentials_lib._GetRunFlowFlags([])
        self.assertEqual(flags.auth_host_name, 'localhost')
        self.assertEqual(flags.auth_host_port, [8080, 8090])
        self.assertEqual(flags.logging_level, 'ERROR')
        self.assertEqual(flags.noauth_local_webserver, False)
