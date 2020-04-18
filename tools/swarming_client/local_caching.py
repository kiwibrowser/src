# Copyright 2018 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""Define local cache policies."""


class CachePolicies(object):
  def __init__(self, max_cache_size, min_free_space, max_items, max_age_secs):
    """Common caching policies for the multiple caches (isolated, named, cipd).

    Arguments:
    - max_cache_size: Trim if the cache gets larger than this value. If 0, the
                      cache is effectively a leak.
    - min_free_space: Trim if disk free space becomes lower than this value. If
                      0, it will unconditionally fill the disk.
    - max_items: Maximum number of items to keep in the cache. If 0, do not
                 enforce a limit.
    - max_age_secs: Maximum age an item is kept in the cache until it is
                    automatically evicted. Having a lot of dead luggage slows
                    everything down.
    """
    self.max_cache_size = max_cache_size
    self.min_free_space = min_free_space
    self.max_items = max_items
    self.max_age_secs = max_age_secs

  def __str__(self):
    return (
        'CachePolicies(max_cache_size=%s; max_items=%s; min_free_space=%s; '
        'max_age_secs=%s)') % (
            self.max_cache_size, self.max_items, self.min_free_space,
            self.max_age_secs)
