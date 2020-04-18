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

"""Defines a dogpile in-memory cache backend that supports size management."""

from dogpile.cache import api
import pylru


class LruBackend(api.CacheBackend):
  """A dogpile.cache backend that uses LRU as the size management."""

  def __init__(self, options):
    """Initializes an LruBackend.

    Args:
      options: a dictionary that contains configuration options.
    """
    capacity = options["capacity"] if "capacity" in options else 200
    self._cache = pylru.lrucache(capacity)

  def get(self, key):
    return self._cache[key] if key in self._cache else api.NO_VALUE

  def set(self, key, value):
    self._cache[key] = value

  def delete(self, key):
    del self._cache[key]
