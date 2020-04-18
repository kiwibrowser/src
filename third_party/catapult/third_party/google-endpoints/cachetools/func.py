"""`functools.lru_cache` compatible memoizing function decorators."""

import collections
import functools
import random
import time
import warnings

try:
    from threading import RLock
except ImportError:
    from dummy_threading import RLock

from .keys import hashkey, typedkey

__all__ = ('lfu_cache', 'lru_cache', 'rr_cache', 'ttl_cache')


class _NLock:
    def __enter__(self):
        pass

    def __exit__(self, *exc):
        pass

_CacheInfo = collections.namedtuple('CacheInfo', [
    'hits', 'misses', 'maxsize', 'currsize'
])

_marker = object()


def _deprecated(message, level=2):
    warnings.warn('%s is deprecated' % message, DeprecationWarning, level)


def _cache(cache, typed=False, context=_marker):
    def decorator(func):
        key = typedkey if typed else hashkey
        if context is _marker:
            lock = RLock()
        elif context is None:
            lock = _NLock()
        else:
            lock = context()
        stats = [0, 0]

        def cache_info():
            with lock:
                hits, misses = stats
                maxsize = cache.maxsize
                currsize = cache.currsize
            return _CacheInfo(hits, misses, maxsize, currsize)

        def cache_clear():
            with lock:
                try:
                    cache.clear()
                finally:
                    stats[:] = [0, 0]

        def wrapper(*args, **kwargs):
            k = key(*args, **kwargs)
            with lock:
                try:
                    v = cache[k]
                    stats[0] += 1
                    return v
                except KeyError:
                    stats[1] += 1
            v = func(*args, **kwargs)
            try:
                with lock:
                    cache[k] = v
            except ValueError:
                pass  # value too large
            return v
        functools.update_wrapper(wrapper, func)
        if not hasattr(wrapper, '__wrapped__'):
            wrapper.__wrapped__ = func  # Python < 3.2
        wrapper.cache_info = cache_info
        wrapper.cache_clear = cache_clear
        return wrapper
    return decorator


def lfu_cache(maxsize=128, typed=False, getsizeof=None, lock=_marker):
    """Decorator to wrap a function with a memoizing callable that saves
    up to `maxsize` results based on a Least Frequently Used (LFU)
    algorithm.

    """
    from .lfu import LFUCache
    if lock is not _marker:
        _deprecated("Passing 'lock' to lfu_cache()", 3)
    return _cache(LFUCache(maxsize, getsizeof), typed, lock)


def lru_cache(maxsize=128, typed=False, getsizeof=None, lock=_marker):
    """Decorator to wrap a function with a memoizing callable that saves
    up to `maxsize` results based on a Least Recently Used (LRU)
    algorithm.

    """
    from .lru import LRUCache
    if lock is not _marker:
        _deprecated("Passing 'lock' to lru_cache()", 3)
    return _cache(LRUCache(maxsize, getsizeof), typed, lock)


def rr_cache(maxsize=128, choice=random.choice, typed=False, getsizeof=None,
             lock=_marker):
    """Decorator to wrap a function with a memoizing callable that saves
    up to `maxsize` results based on a Random Replacement (RR)
    algorithm.

    """
    from .rr import RRCache
    if lock is not _marker:
        _deprecated("Passing 'lock' to rr_cache()", 3)
    return _cache(RRCache(maxsize, choice, getsizeof), typed, lock)


def ttl_cache(maxsize=128, ttl=600, timer=time.time, typed=False,
              getsizeof=None, lock=_marker):
    """Decorator to wrap a function with a memoizing callable that saves
    up to `maxsize` results based on a Least Recently Used (LRU)
    algorithm with a per-item time-to-live (TTL) value.
    """
    from .ttl import TTLCache
    if lock is not _marker:
        _deprecated("Passing 'lock' to ttl_cache()", 3)
    return _cache(TTLCache(maxsize, ttl, timer, getsizeof), typed, lock)
