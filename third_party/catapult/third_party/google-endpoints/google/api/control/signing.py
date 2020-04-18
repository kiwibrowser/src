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

"""Provides support for creating signatures using secure hashes."""

from __future__ import absolute_import


def add_dict_to_hash(a_hash, a_dict):
    """Adds `a_dict` to `a_hash`

    Args:
       a_hash (`Hash`): the secure hash, e.g created by hashlib.md5
       a_dict (dict[string, [string]]): the dictionary to add to the hash

    """
    if a_dict is None:
        return
    for k, v in a_dict.items():
        a_hash.update('\x00' + k + '\x00' + v)
