#!/usr/bin/env python
#
# Copyright (C) 2017 The Android Open Source Project
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
"""Tests for import_abi_metadata.py."""
import unittest

import build.import_abi_metadata


class ImportAbiMetadataTest(unittest.TestCase):
    def test_generate_make_vars(self):
        self.assertEqual(
            'foo := bar',
            build.import_abi_metadata.generate_make_vars(
                {'foo': 'bar'}))
        self.assertEqual(
            build.import_abi_metadata.NEWLINE.join(
                ['foo := bar', 'baz := qux']),
            build.import_abi_metadata.generate_make_vars(
                {'foo': 'bar', 'baz': 'qux'}))

    def test_metadata_to_make_vars(self):
        make_vars = build.import_abi_metadata.metadata_to_make_vars({
            'armeabi': {
                'bitness': 32,
                'default': False,
                'deprecated': True,
            },
            'armeabi-v7a': {
                'bitness': 32,
                'default': True,
                'deprecated': False,
            },
            'arm64-v8a': {
                'bitness': 64,
                'default': True,
                'deprecated': False,
            },
            'mips': {
                'bitness': 32,
                'default': False,
                'deprecated': False,
            },
            'mips64': {
                'bitness': 64,
                'default': False,
                'deprecated': False,
            },
            'x86': {
                'bitness': 32,
                'default': True,
                'deprecated': False,
            },
            'x86_64': {
                'bitness': 64,
                'default': True,
                'deprecated': False,
            },
        })

        self.assertDictEqual({
            'NDK_DEFAULT_ABIS': 'arm64-v8a armeabi-v7a x86 x86_64',
            'NDK_DEPRECATED_ABIS': 'armeabi',
            'NDK_KNOWN_DEVICE_ABI32S': 'armeabi armeabi-v7a mips x86',
            'NDK_KNOWN_DEVICE_ABI64S': 'arm64-v8a mips64 x86_64',
        }, make_vars)
