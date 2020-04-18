# Copyright 2016 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from __future__ import absolute_import

import hashlib

import unittest2
from expects import equal, expect

from google.api.control import signing


class TestAddDictToHash(unittest2.TestCase):
    NOTHING_ADDED = hashlib.md5().digest()

    def test_should_add_nothing_when_dict_is_none(self):
        md5 = hashlib.md5()
        signing.add_dict_to_hash(md5, None)
        expect(md5.digest()).to(equal(self.NOTHING_ADDED))

    def test_should_add_matching_hashes_for_matching_dicts(self):
        a_dict = {'test': 'dict'}
        same_dict = dict(a_dict)
        want_hash = hashlib.md5()
        signing.add_dict_to_hash(want_hash, a_dict)
        want = want_hash.digest()
        got_hash = hashlib.md5()
        signing.add_dict_to_hash(got_hash, same_dict)
        got = got_hash.digest()
        expect(got).to(equal(want))
