#!/usr/bin/env python
#
# Copyright (C) 2015 The Android Open Source Project
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
"""Runs the libc++ tests against the platform libc++."""
from __future__ import print_function

import argparse
import logging
import os
import sys


THIS_DIR = os.path.dirname(os.path.realpath(__file__))
ANDROID_DIR = os.path.realpath(os.path.join(THIS_DIR, '../..'))


def logger():
    """Returns the logger for the module."""
    return logging.getLogger(__name__)


def call(cmd, *args, **kwargs):
    """subprocess.call with logging."""
    import subprocess
    logger().info('call %s', ' '.join(cmd))
    return subprocess.call(cmd, *args, **kwargs)


def check_call(cmd, *args, **kwargs):
    """subprocess.check_call with logging."""
    import subprocess
    logger().info('check_call %s', ' '.join(cmd))
    return subprocess.check_call(cmd, *args, **kwargs)


class ArgParser(argparse.ArgumentParser):
    """Parses command line arguments."""
    def __init__(self):
        super(ArgParser, self).__init__()
        self.add_argument(
            '--compiler', choices=('clang', 'gcc'), default='clang')
        self.add_argument(
            '--bitness', choices=(32, 64), type=int, default=32)
        self.add_argument('--host', action='store_true')


def gen_test_config(bitness, compiler, host):
    """Generates the test configuration makefile for buildcmds."""
    testconfig_mk_path = os.path.join(THIS_DIR, 'buildcmds/testconfig.mk')
    with open(testconfig_mk_path, 'w') as test_config:
        if compiler == 'clang':
            print('LOCAL_CLANG := true', file=test_config)
        elif compiler == 'gcc':
            print('LOCAL_CLANG := false', file=test_config)

        if bitness == 32:
            print('LOCAL_MULTILIB := 32', file=test_config)
        elif bitness == 64:
            print('LOCAL_MULTILIB := 64', file=test_config)

        if compiler == 'clang':
            print('LOCAL_CXX := $(LOCAL_PATH)/buildcmdscc $(CLANG_CXX)',
                  file=test_config)
        else:
            if host:
                prefix = 'HOST_'
            else:
                prefix = 'TARGET_'
            print('LOCAL_CXX := $(LOCAL_PATH)/buildcmdscc '
                  '$($(LOCAL_2ND_ARCH_VAR_PREFIX){}CXX)'.format(prefix),
                  file=test_config)

        if host:
            print('include $(BUILD_HOST_EXECUTABLE)', file=test_config)
        else:
            print('include $(BUILD_EXECUTABLE)', file=test_config)


def mmm(path):
    """Invokes the Android build command mmm."""
    makefile = os.path.join(path, 'Android.mk')
    main_mk = 'build/core/main.mk'

    env = dict(os.environ)
    env['ONE_SHOT_MAKEFILE'] = makefile
    env['LIBCXX_TESTING'] = 'true'
    cmd = [
        'make', '-j', '-C', ANDROID_DIR, '-f', main_mk,
        'MODULES-IN-' + path.replace('/', '-'),
    ]
    check_call(cmd, env=env)


def gen_build_cmds(bitness, compiler, host):
    """Generates the build commands file for the test runner."""
    gen_test_config(bitness, compiler, host)
    mmm('external/libcxx/buildcmds')


def main():
    """Program entry point."""
    logging.basicConfig(level=logging.INFO)

    args, lit_args = ArgParser().parse_known_args()
    lit_path = os.path.join(ANDROID_DIR, 'external/llvm/utils/lit/lit.py')
    gen_build_cmds(args.bitness, args.compiler, args.host)

    mode_str = 'host' if args.host else 'device'
    android_mode_arg = '--param=android_mode=' + mode_str
    site_cfg_path = os.path.join(THIS_DIR, 'test/lit.site.cfg')
    site_cfg_arg = '--param=libcxx_site_config=' + site_cfg_path
    default_test_path = os.path.join(THIS_DIR, 'test')

    have_filter_args = False
    for arg in lit_args:
        # If the argument is a valid path with default_test_path, it is a test
        # filter.
        real_path = os.path.realpath(arg)
        if not real_path.startswith(default_test_path):
            continue
        if not os.path.exists(real_path):
            continue

        have_filter_args = True
        break  # No need to keep scanning.

    lit_args = ['-sv', android_mode_arg, site_cfg_arg] + lit_args
    cmd = ['python', lit_path] + lit_args
    if not have_filter_args:
        cmd.append(default_test_path)
    sys.exit(call(cmd))


if __name__ == '__main__':
    main()
