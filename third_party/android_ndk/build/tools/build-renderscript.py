#!/usr/bin/env python
#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""Packages the platform's RenderScript for the NDK."""
import os
import shutil
import site
import subprocess
import sys

site.addsitedir(os.path.join(os.path.dirname(__file__), '../lib'))

import build_support  # pylint: disable=import-error


def get_rs_prebuilt_path(host):
    rel_prebuilt_path = 'prebuilts/renderscript/host/{}'.format(host)
    prebuilt_path = os.path.join(build_support.android_path(),
                                 rel_prebuilt_path)
    if not os.path.isdir(prebuilt_path):
        sys.exit('Could not find prebuilt RenderScript at {}'.format(prebuilt_path))
    return prebuilt_path


def main(args):
    RS_VERSION = 'current'

    host = args.host
    out_dir = args.out_dir;
    package_dir = args.dist_dir

    os_name = host
    if os_name == 'windows64':
        os_name = 'windows'
    prebuilt_path = get_rs_prebuilt_path(os_name + '-x86')
    print('prebuilt path: ' + prebuilt_path)
    if host == 'darwin':
        host = 'darwin-x86_64'
    elif host == 'linux':
        host = 'linux-x86_64'
    elif host == 'windows':
        host = 'windows'
    elif host == 'windows64':
        host = 'windows-x86_64'

    package_name = 'renderscript-toolchain-{}'.format(host)
    built_path = os.path.join(prebuilt_path, RS_VERSION)
    build_support.make_package(package_name, built_path, package_dir)

if __name__ == '__main__':
    build_support.run(main)
