# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import traceback

from data_source import DataSource
from extensions_paths import EXAMPLES
from future import All, Future
from jsc_view import CreateSamplesView
from platform_util import GetPlatforms


class SamplesDataSource(DataSource):
  '''Constructs a list of samples and their respective files and api calls.
  '''
  def __init__(self, server_instance, request):
    self._platform_bundle = server_instance.platform_bundle
    self._request = request

  def _GetImpl(self, platform):
    cache = self._platform_bundle.GetSamplesModel(platform).GetCache()
    create_view = lambda samp_list: CreateSamplesView(samp_list, self._request)
    return cache.GetFromFileListing('' if platform == 'apps'
                                       else EXAMPLES).Then(create_view)

  def get(self, platform):
    return self._GetImpl(platform).Get()

  def Refresh(self):
    return All(self._GetImpl(platform) for platform in GetPlatforms())
