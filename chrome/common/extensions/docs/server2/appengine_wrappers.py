# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


# This will attempt to import the actual App Engine modules, and if it fails,
# they will be replaced with fake modules. This is useful during testing.
try:
  import google.appengine.api.memcache as memcache
except ImportError:
  class _RPC(object):
    def __init__(self, result=None):
      self.result = result

    def get_result(self):
      return self.result

    def wait(self):
      pass


  class InMemoryMemcache(object):
    """An in-memory memcache implementation.
    """
    def __init__(self):
      self._namespaces = {}

    class Client(object):
      def set_multi_async(self, mapping, namespace='', time=0):
        return _RPC(result=dict(
          (k, memcache.set(k, v, namespace=namespace, time=time))
           for k, v in mapping.iteritems()))

      def get_multi_async(self, keys, namespace='', time=0):
        return _RPC(result=dict(
          (k, memcache.get(k, namespace=namespace, time=time)) for k in keys))

    def set(self, key, value, namespace='', time=0):
      self._GetNamespace(namespace)[key] = value

    def get(self, key, namespace='', time=0):
      return self._GetNamespace(namespace).get(key)

    def delete(self, key, namespace=''):
      self._GetNamespace(namespace).pop(key, None)

    def delete_multi(self, keys, namespace=''):
      for k in keys:
        self.delete(k, namespace=namespace)

    def _GetNamespace(self, namespace):
      if namespace not in self._namespaces:
        self._namespaces[namespace] = {}
      return self._namespaces[namespace]

    def flush_all(self):
      self._namespaces = {}
      return False

  memcache = InMemoryMemcache()
