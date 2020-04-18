# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import posixpath
import traceback

from data_source import DataSource
from docs_server_utils import FormatKey
from extensions_paths import (
    ARTICLES_TEMPLATES, INTROS_TEMPLATES, PRIVATE_TEMPLATES)
from file_system import FileNotFoundError
from future import All
from path_util import AssertIsDirectory


class TemplateDataSource(DataSource):
  '''Provides a DataSource interface for compiled templates.
  '''
  def __init__(self, server_instance, request=None):
    self._dir = type(self)._BASE
    AssertIsDirectory(self._dir)
    self._request = request
    self._template_cache = server_instance.compiled_fs_factory.ForTemplates(
        server_instance.host_file_system_provider.GetMaster())
    self._file_system = server_instance.host_file_system_provider.GetMaster()

  def get(self, path):
    try:
      return self._template_cache.GetFromFile('%s%s' %
          (self._dir, FormatKey(path))).Get()
    except FileNotFoundError:
      logging.warning(traceback.format_exc())
      return None

  def Refresh(self):
    futures = []
    for root, _, files in self._file_system.Walk(self._dir):
      futures += [self._template_cache.GetFromFile(
                      posixpath.join(self._dir, root, FormatKey(f)))
                  for f in files
                  if posixpath.splitext(f)[1] == '.html']
    return All(futures)


class ArticleDataSource(TemplateDataSource):
  '''Serves templates for Articles.
  '''
  _BASE = ARTICLES_TEMPLATES


class IntroDataSource(TemplateDataSource):
  '''Serves templates for Intros.
  '''
  _BASE = INTROS_TEMPLATES


class PartialDataSource(TemplateDataSource):
  '''Serves templates for private templates.
  '''
  _BASE = PRIVATE_TEMPLATES
