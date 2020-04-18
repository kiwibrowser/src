# Copyright 2013 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""Defines a dictionary that can evict least recently used items."""

import collections
import json
import time


class LRUDict(object):
  """Dictionary that can evict least recently used items.

  Implemented as a wrapper around OrderedDict object. An OrderedDict stores
  (key, (value, timestamp)) pairs in order they are
  inserted and can effectively pop oldest items.

  That is, the first item in self._items is the oldest item.

  Can also store its state as *.json file on disk.
  """

  # Used to determine current timestamp.
  # Can be substituted in individual LRUDict instances.
  time_fn = time.time

  def __init__(self):
    # Ordered key -> (value, timestamp) mapping,
    # newest items at the bottom.
    self._items = collections.OrderedDict()
    # True if was modified after loading.
    self._dirty = True

  def __nonzero__(self):
    """False if dict is empty."""
    return bool(self._items)

  def __iter__(self):
    """Iterate over the keys."""
    return self._items.__iter__()

  def __len__(self):
    """Number of items in the dict."""
    return len(self._items)

  def __contains__(self, key):
    """True if |key| is in the dict."""
    return key in self._items

  def __getitem__(self, key):
    """Returns value for |key| or raises KeyError if not found."""
    return self._items[key][0]

  @classmethod
  def load(cls, state_file):
    """Loads previously saved state and returns LRUDict in that state.

    Raises ValueError if state file is corrupted.
    """
    try:
      state = json.load(open(state_file, 'r'))
    except (IOError, ValueError) as e:
      raise ValueError('Broken state file %s: %s' % (state_file, e))
    if not isinstance(state, dict):
      raise ValueError(
          'Broken state file %s, should be json object or list' % (state_file,))
    state_ver = state.get('version')
    if state_ver != 2:
      raise ValueError(
          'Unsupported state file %s, version is %s. '
          'Latest supported is 2' % (state_file, state_ver))
    state_items = state.get('items')
    if not isinstance(state_items, list):
      raise ValueError(
          'Broken state file %s, items should be json list' % (state_file,))
    lru = cls()
    # Items are stored oldest to newest. Put them back in the same order.
    for item in state_items:
      if not isinstance(item, list) or len(item) != 2:
        raise ValueError(
            'Broken state file %s, expecting pairs: %s' % (state_file, item))
      if not isinstance(item[1], list) or len(item[1]) != 2:
        raise ValueError(
            'Broken state file %s, expecting second item to be a item: %s' % (
              state_file, item))
      if not isinstance(item[1][1], (int, float)):
        raise ValueError(
            'Broken state file %s, expecting second item of the second item '
            'to be a number: %s' % (state_file, item))

    lru._items = collections.OrderedDict(state_items)

    # Check for duplicate keys.
    if len(lru) != len(state_items):
      raise ValueError(
          'Broken state file %s, found duplicate keys' % (state_file,))

    # Now state from the file corresponds to state in the memory.
    lru._dirty = False
    return lru

  def save(self, state_file):
    """Saves cache state to a file if it was modified."""
    if not self._dirty:
      return False

    with open(state_file, 'wb') as f:
      contents = {
        'version': 2,
        'items': self._items.items(),
      }
      json.dump(contents, f, separators=(',',':'))

    self._dirty = False
    return True

  def add(self, key, value):
    """Adds or replaces a |value| for |key|, marks it as most recently used."""
    self._items.pop(key, None)
    self._items[key] = (value, self.time_fn())
    self._dirty = True

  def keys_set(self):
    """Set of keys of items in this dict."""
    return set(self._items)

  def get(self, key, default=None):
    """Returns value for |key| or |default| if not found."""
    item = self._items.get(key)
    return item[0] if item is not None else default

  def get_timestamp(self, key):
    """Returns timestamp of last use of |key|.

    Raises KeyError if |key| is not in the dict.
    """
    return self._items[key][1]

  def touch(self, key):
    """Marks |key| as most recently used.

    Raises KeyError if |key| is not in the dict.
    """
    self._items[key] = (self._items.pop(key)[0], self.time_fn())
    self._dirty = True

  def pop(self, key):
    """Removes item from the dict, returns its value.

    Raises KeyError if |key| is not in the dict.
    """
    item = self._items.pop(key)
    self._dirty = True
    return item[0]

  def get_oldest(self):
    """Returns oldest item as tuple (key, (value, timestamp)).

    Raises KeyError if dict is empty.
    """
    for item in self._items.iteritems():
      return item
    raise KeyError('dictionary is empty')

  def pop_oldest(self):
    """Removes oldest item and returns it as (key, (value, timestamp)).

    Raises KeyError if dict is empty.
    """
    item = self._items.popitem(last=False)
    self._dirty = True
    return item

  def itervalues(self):
    """Iterator over stored values in arbitrary order."""
    for val, _ in self._items.itervalues():
      yield val
