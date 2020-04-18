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

"""caches provide functions and classes used to support caching.

caching is provide by extensions of the cache classes provided by the
cachetools open-source library.

:func:`create` creates a cache instance specifed by either
:class:`google.api.control.CheckAggregationOptions` or a
:class:`google.api.control.ReportAggregationOptions`

"""

from __future__ import absolute_import

# pylint: disable=too-many-ancestors
#
# It affects the DequeOutTTLCache and DequeOutLRUCache which extend
# cachetools.TTLCache and cachetools.LRUCache respectively.  Within cachetools,
# those classes each extend Cache, which itself extends DefaultMapping. It does
# makes sense to have this chain of ancestors, so it's right the disable the
# warning here.

import collections
import logging
import threading
from datetime import datetime, timedelta

import cachetools

logger = logging.getLogger(__name__)


class CheckOptions(
        collections.namedtuple(
            'CheckOptions',
            ['num_entries',
             'flush_interval',
             'expiration'])):
    """Holds values used to control report check behavior.

    Attributes:

        num_entries: the maximum number of cache entries that can be kept in
          the aggregation cache
        flush_interval (:class:`datetime.timedelta`): the maximum delta before
          aggregated report requests are flushed to the server.  The cache
          entry is deleted after the flush.
        expiration (:class:`datetime.timedelta`): elapsed time before a cached
          check response should be deleted.  This value should be larger than
          ``flush_interval``, otherwise it will be ignored, and instead a value
          equivalent to flush_interval + 1ms will be used.
    """
    # pylint: disable=too-few-public-methods
    DEFAULT_NUM_ENTRIES = 200
    DEFAULT_FLUSH_INTERVAL = timedelta(milliseconds=500)
    DEFAULT_EXPIRATION = timedelta(seconds=1)

    def __new__(cls,
                num_entries=DEFAULT_NUM_ENTRIES,
                flush_interval=DEFAULT_FLUSH_INTERVAL,
                expiration=DEFAULT_EXPIRATION):
        """Invokes the base constructor with default values."""
        assert isinstance(num_entries, int), 'should be an int'
        assert isinstance(flush_interval, timedelta), 'should be a timedelta'
        assert isinstance(expiration, timedelta), 'should be a timedelta'
        if expiration <= flush_interval:
            expiration = flush_interval + timedelta(milliseconds=1)
        return super(cls, CheckOptions).__new__(
            cls,
            num_entries,
            flush_interval,
            expiration)


class ReportOptions(
        collections.namedtuple(
            'ReportOptions',
            ['num_entries',
             'flush_interval'])):
    """Holds values used to control report aggregation behavior.

    Attributes:

        num_entries: the maximum number of cache entries that can be kept in
          the aggregation cache

        flush_interval (:class:`datetime.timedelta`): the maximum delta before
          aggregated report requests are flushed to the server.  The cache
          entry is deleted after the flush
    """
    # pylint: disable=too-few-public-methods
    DEFAULT_NUM_ENTRIES = 200
    DEFAULT_FLUSH_INTERVAL = timedelta(seconds=1)

    def __new__(cls,
                num_entries=DEFAULT_NUM_ENTRIES,
                flush_interval=DEFAULT_FLUSH_INTERVAL):
        """Invokes the base constructor with default values."""
        assert isinstance(num_entries, int), 'should be an int'
        assert isinstance(flush_interval, timedelta), 'should be a timedelta'

        return super(cls, ReportOptions).__new__(
            cls,
            num_entries,
            flush_interval)


ZERO_INTERVAL = timedelta()


def create(options, timer=None):
    """Create a cache specified by ``options``

    ``options`` is an instance of either
    :class:`google.api.control.caches.CheckOptions` or
    :class:`google.api.control.caches.ReportOptions`

    The returned cache is wrapped in a :class:`LockedObject`, requiring it to
    be accessed in a with statement that gives synchronized access

    Example:
      >>> options = CheckOptions()
      >>> synced_cache = make_cache(options)
      >>> with synced_cache as cache:  #  acquire the lock
      ...    cache['a_key'] = 'a_value'

    Args:
      options (object): an instance of either of the options classes

    Returns:
      :class:`cachetools.Cache`: the cache implementation specified by options
        or None: if options is ``None`` or if options.num_entries < 0

    Raises:
       ValueError: if options is not a support type

    """
    if options is None:  # no options, don't create cache
        return None

    if not (isinstance(options, CheckOptions) or
            isinstance(options, ReportOptions)):
        logger.error('make_cache(): bad options %s', options)
        raise ValueError('Invalid options')

    if (options.num_entries <= 0):
        logger.info("did not create cache, options was %s", options)
        return None

    logger.info("creating a cache from %s", options)
    if (options.flush_interval > ZERO_INTERVAL):
        # options always has a flush_interval, but may have an expiration
        # field. If the expiration is present, use that instead of the
        # flush_interval for the ttl
        ttl = getattr(options, 'expiration', options.flush_interval)
        return LockedObject(
            DequeOutTTLCache(
                options.num_entries,
                ttl=ttl.total_seconds(),
                timer=to_cache_timer(timer)
            ))

    return LockedObject(DequeOutLRUCache(options.num_entries))


class DequeOutTTLCache(cachetools.TTLCache):
    """Extends ``TTLCache`` so that expired items are placed in a ``deque``."""

    def __init__(self, maxsize, ttl, out_deque=None, **kw):
        """Constructor.

        Args:
          maxsize (int): the maximum number of entries in the queue
          ttl (int): the ttl for entries added to the cache
          out_deque :class:`collections.deque`: a `deque` in which to add items
            that expire from the cache
          **kw: the other keyword args supported by the constructor to
            :class:`cachetools.TTLCache`

        Raises:
          ValueError: if out_deque is not a collections.deque

        """
        super(DequeOutTTLCache, self).__init__(maxsize, ttl, **kw)
        if out_deque is None:
            out_deque = collections.deque()
        elif not isinstance(out_deque, collections.deque):
            raise ValueError('out_deque should be a collections.deque')
        self._out_deque = out_deque
        self._tracking = {}

    def __setitem__(self, key, value, **kw):
        super(DequeOutTTLCache, self).__setitem__(key, value, **kw)
        self._tracking[key] = value

    @property
    def out_deque(self):
        """The :class:`collections.deque` to which expired items are added."""
        self.expire()
        expired = dict((k, v) for (k, v) in self._tracking.items()
                       if self.get(k) is None)
        for k, v in expired.items():
            del self._tracking[k]
            self._out_deque.append(v)
        return self._out_deque


class DequeOutLRUCache(cachetools.LRUCache):
    """Extends ``LRUCache`` so that expired items are placed in a ``deque``."""

    def __init__(self, maxsize, out_deque=None, **kw):
        """Constructor.

        Args:
          maxsize (int): the maximum number of entries in the queue
          out_deque :class:`collections.deque`: a `deque` in which to add items
            that expire from the cache
          **kw: the other keyword args supported by constructor to
            :class:`cachetools.LRUCache`

        Raises:
          ValueError: if out_deque is not a collections.deque

        """
        super(DequeOutLRUCache, self).__init__(maxsize, **kw)
        if out_deque is None:
            out_deque = collections.deque()
        elif not isinstance(out_deque, collections.deque):
            raise ValueError('out_deque should be collections.deque')
        self._out_deque = out_deque
        self._tracking = {}

    def __setitem__(self, key, value, **kw):
        super(DequeOutLRUCache, self).__setitem__(key, value, **kw)
        self._tracking[key] = value

    @property
    def out_deque(self):
        """The :class:`collections.deque` to which expired items are added."""
        expired = dict((k, v) for (k, v) in iter(self._tracking.items())
                       if self.get(k) is None)
        for k, v in iter(expired.items()):
            del self._tracking[k]
            self._out_deque.append(v)
        return self._out_deque


class LockedObject(object):
    """LockedObject protects an object with a re-entrant lock.

    The lock is required by the context manager protocol.
    """
    # pylint: disable=too-few-public-methods

    def __init__(self, obj):
        self._lock = threading.RLock()
        self._obj = obj

    def __enter__(self):
        self._lock.acquire()
        return self._obj

    def __exit__(self, _exc_type, _exc_val, _exc_tb):
        self._lock.release()


def to_cache_timer(datetime_func):
    """Converts a datetime_func to a timestamp_func.

    Args:
       datetime_func (callable[[datatime]]): a func that returns the current
         time

    Returns:
       time_func (callable[[timestamp]): a func that returns the timestamp
         from the epoch
    """
    if datetime_func is None:
        datetime_func = datetime.utcnow

    def _timer():
        """Return the timestamp since the epoch."""
        return (datetime_func() - datetime(1970, 1, 1)).total_seconds()

    return _timer
