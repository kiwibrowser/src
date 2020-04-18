# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import traceback

from chroot_file_system import ChrootFileSystem
from content_provider import ContentProvider
import environment
from extensions_paths import CONTENT_PROVIDERS, LOCAL_DEBUG_DIR
from future import All, Future
from local_file_system import LocalFileSystem
from third_party.json_schema_compiler.memoize import memoize


_IGNORE_MISSING_CONTENT_PROVIDERS = [False]


def IgnoreMissingContentProviders(fn):
  '''Decorates |fn| to ignore missing content providers during its run.
  '''
  def run(*args, **optargs):
    saved = _IGNORE_MISSING_CONTENT_PROVIDERS[0]
    _IGNORE_MISSING_CONTENT_PROVIDERS[0] = True
    try:
      return fn(*args, **optargs)
    finally:
      _IGNORE_MISSING_CONTENT_PROVIDERS[0] = saved
  return run


class ContentProviders(object):
  '''Implements the content_providers.json configuration; see
  chrome/common/extensions/docs/templates/json/content_providers.json for its
  current state and a description of the format.

  Returns ContentProvider instances based on how they're configured there.
  '''

  def __init__(self,
               object_store_creator,
               compiled_fs_factory,
               host_file_system,
               gcs_file_system_provider):
    self._object_store_creator = object_store_creator
    self._compiled_fs_factory = compiled_fs_factory
    self._host_file_system = host_file_system
    self._gcs_file_system_provider = gcs_file_system_provider
    self._cache = None

    # If running the devserver and there is a LOCAL_DEBUG_DIR, we
    # will read the content_provider configuration from there instead
    # of fetching it from Gitiles or patch.
    if environment.IsDevServer() and os.path.exists(LOCAL_DEBUG_DIR):
      local_fs = LocalFileSystem(LOCAL_DEBUG_DIR)
      conf_stat = None
      try:
        conf_stat = local_fs.Stat(CONTENT_PROVIDERS)
      except:
        pass

      if conf_stat:
        logging.warn(("Using local debug folder (%s) for "
                      "content_provider.json configuration") % LOCAL_DEBUG_DIR)
        self._cache = compiled_fs_factory.ForJson(local_fs)

    if not self._cache:
      self._cache = compiled_fs_factory.ForJson(host_file_system)

  @memoize
  def GetByName(self, name):
    '''Gets the ContentProvider keyed by |name| in content_providers.json, or
    None of there is no such content provider.
    '''
    config = self._GetConfig().get(name)
    if config is None:
      logging.error('No content provider found with name "%s"' % name)
      return None
    return self._CreateContentProvider(name, config)

  @memoize
  def GetByServeFrom(self, path):
    '''Gets a (content_provider, serve_from, path_in_content_provider) tuple,
    where content_provider is the ContentProvider with the longest "serveFrom"
    property that is a subpath of |path|, serve_from is that property, and
    path_in_content_provider is the remainder of |path|.

    For example, if content provider A serves from "foo" and content provider B
    serves from "foo/bar", GetByServeFrom("foo/bar/baz") will return (B,
    "foo/bar", "baz").

    Returns (None, '', |path|) if no ContentProvider serves from |path|.
    '''
    serve_from_to_config = dict(
        (config['serveFrom'], (name, config))
        for name, config in self._GetConfig().iteritems())
    path_parts = path.split('/')
    for i in xrange(len(path_parts), -1, -1):
      name_and_config = serve_from_to_config.get('/'.join(path_parts[:i]))
      if name_and_config is not None:
        return (self._CreateContentProvider(name_and_config[0],
                                            name_and_config[1]),
                '/'.join(path_parts[:i]),
                '/'.join(path_parts[i:]))
    return None, '', path

  def _GetConfig(self):
    return self._cache.GetFromFile(CONTENT_PROVIDERS).Get()

  def _CreateContentProvider(self, name, config):
    default_extensions = config.get('defaultExtensions', ())
    supports_templates = config.get('supportsTemplates', False)
    supports_zip = config.get('supportsZip', False)

    if 'chromium' in config:
      chromium_config = config['chromium']
      if 'dir' not in chromium_config:
        logging.error('%s: "chromium" must have a "dir" property' % name)
        return None
      file_system = ChrootFileSystem(self._host_file_system,

                                     chromium_config['dir'])
    # TODO(rockot): Remove this in a future patch. It should not be needed once
    # the new content_providers.json is committed.
    elif 'gitiles' in config:
      chromium_config = config['gitiles']
      if 'dir' not in chromium_config:
        logging.error('%s: "chromium" must have a "dir" property' % name)
        return None
      file_system = ChrootFileSystem(self._host_file_system,
                                     chromium_config['dir'])
    elif 'gcs' in config:
      gcs_config = config['gcs']
      if 'bucket' not in gcs_config:
        logging.error('%s: "gcs" must have a "bucket" property' % name)
        return None
      bucket = gcs_config['bucket']
      if not bucket.startswith('gs://'):
        logging.error('%s: bucket %s should start with gs://' % (name, bucket))
        return None
      bucket = bucket[len('gs://'):]
      file_system = self._gcs_file_system_provider.Create(bucket)
      if 'dir' in gcs_config:
        file_system = ChrootFileSystem(file_system, gcs_config['dir'])

    else:
      logging.error('%s: content provider type not supported' % name)
      return None

    return ContentProvider(name,
                           self._compiled_fs_factory,
                           file_system,
                           self._object_store_creator,
                           default_extensions=default_extensions,
                           supports_templates=supports_templates,
                           supports_zip=supports_zip)

  def Refresh(self):
    def safe(name, action, callback):
      '''Safely runs |callback| for a ContentProvider called |name| by
      swallowing exceptions and turning them into a None return value. It's
      important to run all ContentProvider Refreshes even if some of them fail.
      '''
      try:
        return callback()
      except:
        if not _IGNORE_MISSING_CONTENT_PROVIDERS[0]:
          logging.error('Error %s Refresh for ContentProvider "%s":\n%s' %
                        (action, name, traceback.format_exc()))
        return None

    def refresh_provider(path, config):
      provider = self._CreateContentProvider(path, config)
      future = safe(path,
                    'initializing',
                    self._CreateContentProvider(path, config).Refresh)
      if future is None:
        return Future(callback=lambda: True)
      return Future(callback=lambda: safe(path, 'resolving', future.Get))

    return All(refresh_provider(path, config)
               for path, config in self._GetConfig().iteritems())
