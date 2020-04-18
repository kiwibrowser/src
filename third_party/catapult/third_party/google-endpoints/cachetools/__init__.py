"""Extensible memoizing collections and decorators."""

import functools
import warnings

from .cache import Cache
from .func import lfu_cache, lru_cache, rr_cache, ttl_cache
from .keys import hashkey, typedkey
from .lfu import LFUCache
from .lru import LRUCache
from .rr import RRCache
from .ttl import TTLCache

__all__ = (
    'Cache', 'LFUCache', 'LRUCache', 'RRCache', 'TTLCache',
    'cached', 'cachedmethod', 'hashkey', 'typedkey',
    # make cachetools.func.* available for backwards compatibility
    'lfu_cache', 'lru_cache', 'rr_cache', 'ttl_cache',
)

__version__ = '1.1.6'

_default = []  # evaluates to False

if hasattr(functools.update_wrapper(lambda f: f(), lambda: 42), '__wrapped__'):
    _update_wrapper = functools.update_wrapper
else:
    def _update_wrapper(wrapper, wrapped):
        functools.update_wrapper(wrapper, wrapped)
        wrapper.__wrapped__ = wrapped
        return wrapper


def cached(cache, key=hashkey, lock=None):
    """Decorator to wrap a function with a memoizing callable that saves
    results in a cache.

    """
    def decorator(func):
        if cache is None:
            def wrapper(*args, **kwargs):
                return func(*args, **kwargs)
        elif lock is None:
            def wrapper(*args, **kwargs):
                k = key(*args, **kwargs)
                try:
                    return cache[k]
                except KeyError:
                    pass  # key not found
                v = func(*args, **kwargs)
                try:
                    cache[k] = v
                except ValueError:
                    pass  # value too large
                return v
        else:
            def wrapper(*args, **kwargs):
                k = key(*args, **kwargs)
                try:
                    with lock:
                        return cache[k]
                except KeyError:
                    pass  # key not found
                v = func(*args, **kwargs)
                try:
                    with lock:
                        cache[k] = v
                except ValueError:
                    pass  # value too large
                return v
        return _update_wrapper(wrapper, func)
    return decorator


def cachedmethod(cache, key=_default, lock=None, typed=_default):
    """Decorator to wrap a class or instance method with a memoizing
    callable that saves results in a cache.

    """
    if key is not _default and not callable(key):
        key, typed = _default, key
    if typed is not _default:
        warnings.warn("Passing 'typed' to cachedmethod() is deprecated, "
                      "use 'key=typedkey' instead", DeprecationWarning, 2)

    def decorator(method):
        # pass method to default key function for backwards compatibilty
        if key is _default:
            makekey = functools.partial(typedkey if typed else hashkey, method)
        else:
            makekey = key  # custom key function always receive method args
        if lock is None:
            def wrapper(self, *args, **kwargs):
                c = cache(self)
                if c is None:
                    return method(self, *args, **kwargs)
                k = makekey(self, *args, **kwargs)
                try:
                    return c[k]
                except KeyError:
                    pass  # key not found
                v = method(self, *args, **kwargs)
                try:
                    c[k] = v
                except ValueError:
                    pass  # value too large
                return v
        else:
            def wrapper(self, *args, **kwargs):
                c = cache(self)
                if c is None:
                    return method(self, *args, **kwargs)
                k = makekey(self, *args, **kwargs)
                try:
                    with lock(self):
                        return c[k]
                except KeyError:
                    pass  # key not found
                v = method(self, *args, **kwargs)
                try:
                    with lock(self):
                        c[k] = v
                except ValueError:
                    pass  # value too large
                return v
        _update_wrapper(wrapper, method)

        # deprecated wrapper attribute
        def getter(self):
            warnings.warn('%s.cache is deprecated' % method.__name__,
                          DeprecationWarning, 2)
            return cache(self)
        wrapper.cache = getter
        return wrapper
    return decorator
