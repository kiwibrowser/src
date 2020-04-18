# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from extensions_paths import JSON_TEMPLATES
from data_source import DataSource


class StringsDataSource(DataSource):
  '''Provides templates with access to a key to string mapping defined in a
  JSON configuration file.
  '''
  def __init__(self, server_instance, _):
    self._cache = server_instance.compiled_fs_factory.ForJson(
        server_instance.host_file_system_provider.GetMaster())

  def _GetStringsData(self):
    return self._cache.GetFromFile('%sstrings.json' % JSON_TEMPLATES)

  def Refresh(self):
    return self._GetStringsData()

  def get(self, key):
    string = self._GetStringsData().Get().get(key)
    if string is None:
      logging.warning('String "%s" not found' % key)
    return string
