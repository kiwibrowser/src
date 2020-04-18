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

"""Script to regenerate samples with latest client generator."""

import os
import subprocess

_GEN_CLIENT_BINARY = 'gen_client'

_SAMPLES = [
    'bigquery_sample/bigquery_v2.json',
    'dns_sample/dns_v1.json',
    'iam_sample/iam_v1.json',
    'fusiontables_sample/fusiontables_v1.json',
    'servicemanagement_sample/servicemanagement_v1.json',
    'storage_sample/storage_v1.json',
]


def _Generate(samples):
    for sample in samples:
        sample_dir, sample_doc = os.path.split(sample)
        name, ext = os.path.splitext(sample_doc)
        if ext != '.json':
            raise RuntimeError('Expected .json discovery doc [{0}]'
                               .format(sample))
        api_name, api_version = name.split('_')
        args = [
            _GEN_CLIENT_BINARY,
            '--infile', sample,
            '--init-file', 'empty',
            '--outdir={0}'.format(os.path.join(sample_dir, name)),
            '--overwrite',
            '--root_package',
            'samples.{0}_sample.{0}_{1}'.format(api_name, api_version),
            'client',
        ]
        subprocess.check_call(args)


if __name__ == '__main__':
    _Generate(_SAMPLES)
