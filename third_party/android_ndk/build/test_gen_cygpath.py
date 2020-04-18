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

import build.gen_cygpath


class GetMountsTest(unittest.TestCase):
    def testSingleMount(self):
        mount_output = 'C:/cygwin on / type ntfs (binary,auto)'
        self.assertEqual(
            [('/', 'C:/cygwin')], build.gen_cygpath.get_mounts(mount_output))

    def testCaseInsensitiveMount(self):
        mount_output = 'C: on /cygdrive/c type ntfs'
        expected_output = [
            ('/cygdrive/c', 'C:'),
            ('/cygdrive/C', 'C:'),
        ]

        self.assertEqual(
            expected_output, build.gen_cygpath.get_mounts(mount_output))

    def testManyMounts(self):
        mount_output = textwrap.dedent("""\
            C:/cygwin/bin on /usr/bin type ntfs (binary,auto)
            C:/cygwin/lib on /usr/lib type ntfs (binary,auto)
            C:/cygwin on / type ntfs (binary,auto)
            C: on /cygdrive/c type ntfs (binary,posix=0,user,noumount,auto)
            D: on /cygdrive/d type udf (binary,posix=0,user,noumount,auto)
            """)

        expected_output = [
            ('/', 'C:/cygwin'),
            ('/usr/bin', 'C:/cygwin/bin'),
            ('/usr/lib', 'C:/cygwin/lib'),
            ('/cygdrive/c', 'C:'),
            ('/cygdrive/C', 'C:'),
            ('/cygdrive/d', 'D:'),
            ('/cygdrive/D', 'D:'),
        ]

        self.assertEqual(
            expected_output, build.gen_cygpath.get_mounts(mount_output))


class MakeCygpathFunctionTest(unittest.TestCase):
    def testSingleMount(self):
        mounts = [('/', 'C:/cygwin')]
        expected_output = '$(patsubst /%,C:/cygwin/%,\n$1)'

        self.assertEqual(
            expected_output, build.gen_cygpath.make_cygpath_function(mounts))

    def testManyMounts(self):
        mounts = [
            ('/', 'C:/cygwin'),
            ('/usr/bin', 'C:/cygwin/bin'),
            ('/usr/lib', 'C:/cygwin/lib'),
            ('/cygdrive/c', 'C:'),
            ('/cygdrive/C', 'C:'),
            ('/cygdrive/d', 'D:'),
            ('/cygdrive/D', 'D:'),
        ]

        expected_output = textwrap.dedent("""\
            $(patsubst /%,C:/cygwin/%,
            $(patsubst /usr/bin/%,C:/cygwin/bin/%,
            $(patsubst /usr/lib/%,C:/cygwin/lib/%,
            $(patsubst /cygdrive/c/%,C:/%,
            $(patsubst /cygdrive/C/%,C:/%,
            $(patsubst /cygdrive/d/%,D:/%,
            $(patsubst /cygdrive/D/%,D:/%,
            $1)))))))""")

        self.assertEqual(
            expected_output, build.gen_cygpath.make_cygpath_function(mounts))
