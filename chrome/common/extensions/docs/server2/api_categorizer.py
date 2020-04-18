# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import posixpath

from compiled_file_system import SingleFile
from extensions_paths import PUBLIC_TEMPLATES


class APICategorizer(object):
  ''' This class gets api category from documented apis.
  '''

  def __init__(self, file_system, compiled_fs_factory, platform):
    self._file_system = file_system
    self._cache = compiled_fs_factory.Create(file_system,
                                             self._CollectDocumentedAPIs,
                                             APICategorizer)
    self._platform = platform

  def _GenerateAPICategories(self):
    return self._cache.GetFromFileListing(
        posixpath.join(PUBLIC_TEMPLATES, self._platform) + '/').Get()

  @SingleFile
  def _CollectDocumentedAPIs(self, base_dir, files):
    public_templates = []
    for root, _, files in self._file_system.Walk(base_dir):
      public_templates.extend(posixpath.join(root, name) for name in files)
    template_names = set(os.path.splitext(name)[0].replace('_', '.')
                         for name in public_templates)
    return template_names

  def GetCategory(self, api_name):
    '''Returns the type of api:
        "internal":     Used by chrome internally. Never documented.
        "private":      APIs which are undocumented or are available to
                        whitelisted apps/extensions.
        "experimental": Experimental APIs.
        "chrome":       Public APIs.
    '''
    documented_apis = self._GenerateAPICategories()
    if api_name.endswith('Internal'):
      assert api_name not in documented_apis, \
          "Internal API %s on %s platform should not be documented" % (
              api_name, self._platform)
      return 'internal'
    if (api_name.endswith('Private') or
        api_name not in documented_apis):
      return 'private'
    if api_name.startswith('experimental.'):
      return 'experimental'
    return 'chrome'

  def IsDocumented(self, api_name):
    return (api_name in self._GenerateAPICategories())
