#!/usr/bin/env python

# Copyright 2016 Google Inc.
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


import json
import os
import sys

ROOT_PATH = os.path.dirname(os.path.abspath(__file__))
METADATA_FILENAME = 'METADATA'
METADATA_TEMPLATE_PATH = os.path.join(ROOT_PATH, 'METADATA.template')
BASE_NODE_MODULES = os.path.join(ROOT_PATH, 'third_party/node_modules')
NODE_MODULES_COUNT = 0


def main():
  check_directory(BASE_NODE_MODULES)
  print('\n\n RESULTS - node modules visited {}'.format(NODE_MODULES_COUNT))


def check_directory(path):
  files = os.listdir(path)
  parent_dir = os.path.basename(os.path.dirname(path))
  if 'package.json' in files and parent_dir == 'node_modules':
    process_node_module(path)
  for f in files:
    file_path = os.path.join(path, f)
    if os.path.isdir(file_path):
      check_directory(file_path)


def process_node_module(path):
  global NODE_MODULES_COUNT
  # not real node modules, these are test fixtures
  blacklist_paths = [
    '/resolve/test/resolver/biz/node_modules/garply',
    '/resolve/test/subdirs/node_modules/a',
    '/resolve/test/pathfilter/deep_ref/node_modules/deep'
  ]
  if any(blacklist_path in path for blacklist_path in blacklist_paths):
    return
  NODE_MODULES_COUNT += 1
  check_license_file(path)
  generate_metadata(path)


def check_license_file(path):
  for f in os.listdir(path):
    if f != 'LICENSE' and f.lower() in ['license', 'license.txt', 'license.md',
                                        'licence', 'license.bsd', 'license-mit',
                                        'license.apache2',
                                        'license.closure-compiler',
                                        'mit.license']:
      src = os.path.join(path, f)
      dest = os.path.join(path, 'LICENSE')
      os.rename(src, dest)
  if 'LICENSE' not in os.listdir(path):
    print('Invalid license for module:', path)


def generate_metadata(path):
  template_lines = []
  with open(METADATA_TEMPLATE_PATH, 'r') as f:
    for line in f:
      template_lines.append(line)

  package_path = os.path.join(path, 'package.json')
  with open(package_path) as file:
    data = json.load(file)
    name = data['name']
    version = data['version']
    tarball_address = 'https://registry.npmjs.org/%s/-/%s-%s.tgz' % (name, name, version)

  output_lines = []
  for line in template_lines:
    replace_line = line.replace('@@name@@', name)\
      .replace('@@tarball_address@@', tarball_address)\
      .replace('@@version@@', version)
    output_lines.append(replace_line)

  dest = os.path.join(path, METADATA_FILENAME)
  with open(dest, 'w') as f:
    for line in output_lines:
      try:
        f.write(line)
      except UnicodeEncodeError as error:
        print('Error writing metadata for module:', path, error)


if __name__ == '__main__':
  sys.exit(main())
