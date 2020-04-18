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

"""Test for gen_client module."""

import os

import unittest2

from apitools.gen import gen_client
from apitools.gen import test_utils


def GetDocPath(name):
    return os.path.join(os.path.dirname(__file__), 'testdata', name)


@test_utils.RunOnlyOnPython27
class ClientGenCliTest(unittest2.TestCase):

    def testHelp_NotEnoughArguments(self):
        with self.assertRaisesRegexp(SystemExit, '0'):
            with test_utils.CaptureOutput() as (_, err):
                gen_client.main([gen_client.__file__, '-h'])
                err_output = err.getvalue()
                self.assertIn('usage:', err_output)
                self.assertIn('error: too few arguments', err_output)

    def testGenClient_SimpleDoc(self):
        with test_utils.TempDir() as tmp_dir_path:
            gen_client.main([
                gen_client.__file__,
                '--nogenerate_cli',
                '--infile', GetDocPath('dns_v1.json'),
                '--outdir', tmp_dir_path,
                '--overwrite',
                '--root_package', 'google.apis',
                'client'
            ])
            self.assertEquals(
                set(['dns_v1_client.py', 'dns_v1_messages.py', '__init__.py']),
                set(os.listdir(tmp_dir_path)))

    def testGenClient_SimpleDocWithV4(self):
        with test_utils.TempDir() as tmp_dir_path:
            gen_client.main([
                gen_client.__file__,
                '--nogenerate_cli',
                '--infile', GetDocPath('dns_v1.json'),
                '--outdir', tmp_dir_path,
                '--overwrite',
                '--apitools_version', '0.4.12',
                '--root_package', 'google.apis',
                'client'
            ])
            self.assertEquals(
                set(['dns_v1_client.py', 'dns_v1_messages.py', '__init__.py']),
                set(os.listdir(tmp_dir_path)))

    def testGenClient_SimpleDocWithV5(self):
        with test_utils.TempDir() as tmp_dir_path:
            gen_client.main([
                gen_client.__file__,
                '--nogenerate_cli',
                '--infile', GetDocPath('dns_v1.json'),
                '--outdir', tmp_dir_path,
                '--overwrite',
                '--apitools_version', '0.5.0',
                '--root_package', 'google.apis',
                'client'
            ])
            self.assertEquals(
                set(['dns_v1_client.py', 'dns_v1_messages.py', '__init__.py']),
                set(os.listdir(tmp_dir_path)))

    def testGenPipPackage_SimpleDoc(self):
        with test_utils.TempDir() as tmp_dir_path:
            gen_client.main([
                gen_client.__file__,
                '--nogenerate_cli',
                '--infile', GetDocPath('dns_v1.json'),
                '--outdir', tmp_dir_path,
                '--overwrite',
                '--root_package', 'google.apis',
                'pip_package'
            ])
            self.assertEquals(
                set(['apitools', 'setup.py']),
                set(os.listdir(tmp_dir_path)))

    def testGenProto_SimpleDoc(self):
        with test_utils.TempDir() as tmp_dir_path:
            gen_client.main([
                gen_client.__file__,
                '--nogenerate_cli',
                '--infile', GetDocPath('dns_v1.json'),
                '--outdir', tmp_dir_path,
                '--overwrite',
                '--root_package', 'google.apis',
                'proto'
            ])
            self.assertEquals(
                set(['dns_v1_messages.proto', 'dns_v1_services.proto']),
                set(os.listdir(tmp_dir_path)))
