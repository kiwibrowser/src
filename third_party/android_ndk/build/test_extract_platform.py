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
import textwrap
import unittest

import build.extract_platform


class ExtractPlatformTest(unittest.TestCase):
    def testNumericVersion(self):
        props_file = textwrap.dedent("""\
            some
            # other
            junk
            target=android-9
            foo
            """).splitlines()

        self.assertEqual(
            'android-9', build.extract_platform.get_platform(props_file))

    def testNamedVersion(self):
        props_file = textwrap.dedent("""\
            some
            # other
            junk
            target=android-nougat
            foo
            """).splitlines()

        self.assertEqual(
            'android-nougat', build.extract_platform.get_platform(props_file))

    def testVendorVersion(self):
        props_file = textwrap.dedent("""\
            some
            # other
            junk
            target=vendor:something:21
            foo
            """).splitlines()

        self.assertEqual(
            'android-21', build.extract_platform.get_platform(props_file))

    def testNoVersion(self):
        self.assertEqual('unknown', build.extract_platform.get_platform([]))
