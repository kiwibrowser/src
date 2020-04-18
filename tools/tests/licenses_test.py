#!/usr/bin/env python
# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for //tools/licenses.py.
"""

import os
import sys
import unittest

REPOSITORY_ROOT = os.path.abspath(os.path.join(
    os.path.dirname(__file__), '..', '..'))
sys.path.append(os.path.join(REPOSITORY_ROOT, 'tools'))

import licenses


class LicensesTest(unittest.TestCase):

    def test_get_third_party_deps_from_gn_deps_output(self):
        third_party_deps = licenses.GetThirdPartyDepsFromGNDepsOutput(
            '/home/example/src/net/BUILD.gn\n'
            '/home/example/src/third_party/zlib/BUILD.gn\n'
            '/home/example/src/third_party/cld_3/src/src/BUILD.gn\n')

        # '/home/example/src/net' is not in the output because it's not a
        # third_party dependency.
        #
        # It must return the direct sub-directory of "third_party". So it should
        # return '/home/example/src/third_party/cld_3', not
        # '/home/example/src/third_party/cld_3/src/src'.
        assert third_party_deps == set([
            '/home/example/src/third_party/zlib',
            '/home/example/src/third_party/cld_3',
        ])


if __name__ == '__main__':
    unittest.main()
