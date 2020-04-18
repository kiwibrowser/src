# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from data_source import DataSource
from docs_server_utils import StringIdentity
from environment import IsPreviewServer
from file_system import FileNotFoundError
from future import All, Future
from jsc_view import CreateJSCView, GetEventByNameFromEvents
from platform_util import GetPlatforms


class APIDataSource(DataSource):
  '''This class fetches and loads JSON APIs from the FileSystem passed in with
  |compiled_fs_factory|, so the APIs can be plugged into templates.
  '''
  def __init__(self, server_instance, request):
    file_system = server_instance.host_file_system_provider.GetMaster()
    self._json_cache = server_instance.compiled_fs_factory.ForJson(file_system)
    self._template_cache = server_instance.compiled_fs_factory.ForTemplates(
        file_system)
    self._platform_bundle = server_instance.platform_bundle
    self._view_cache = server_instance.object_store_creator.Create(
        APIDataSource,
        # Update the models when any of templates, APIs, or Features change.
        category=StringIdentity(self._json_cache.GetIdentity(),
                                self._template_cache.GetIdentity(),
                                self._platform_bundle.GetIdentity()))

    # This caches the result of _LoadEventByName.
    self._event_byname_futures = {}
    self._request = request

  def _LoadEventByName(self, platform):
    '''All events have some members in common. We source their description
    from Event in events.json.
    '''
    if platform not in self._event_byname_futures:
      future = self._GetSchemaView(platform, 'events')
      self._event_byname_futures[platform] = Future(
          callback=lambda: GetEventByNameFromEvents(future.Get()))
    return self._event_byname_futures[platform]

  def _GetSchemaView(self, platform, api_name):
    object_store_key = '/'.join((platform, api_name))
    api_models = self._platform_bundle.GetAPIModels(platform)
    jsc_view_future = self._view_cache.Get(object_store_key)
    model_future = api_models.GetModel(api_name)
    content_script_apis_future = api_models.GetContentScriptAPIs()

    # Parsing samples on the preview server takes forever, so disable it.
    if IsPreviewServer():
      samples_future = Future(value=[])
    else:
      samples_future = (self._platform_bundle.GetSamplesModel(platform)
          .FilterSamples(api_name))

    def resolve():
      jsc_view = jsc_view_future.Get()
      if jsc_view is None:
        jsc_view = CreateJSCView(
            content_script_apis_future.Get(),
            model_future.Get(),
            self._platform_bundle.GetAvailabilityFinder(platform),
            self._json_cache,
            self._template_cache,
            self._platform_bundle.GetFeaturesBundle(platform),
            self._LoadEventByName(platform),
            platform,
            samples_future.Get(),
            self._request)
        self._view_cache.Set(object_store_key, jsc_view)
      return jsc_view
    return Future(callback=resolve)

  def get(self, platform):
    '''Return a getter object so that templates can perform lookups such
    as apis.extensions.runtime.
    '''
    getter = lambda: 0
    getter.get = lambda api_name: self._GetSchemaView(platform, api_name).Get()
    return getter

  def Refresh(self):
    def get_api_schema(platform, api):
      return self._GetSchemaView(platform, api)

    def get_platform_schemas(platform):
      # Internal APIs are an internal implementation detail. Do not pass them to
      # templates.
      return All([get_api_schema(platform, api)
                  for api in self._platform_bundle.GetAPIModels(platform)
                  .GetNames()
                  if self._platform_bundle.GetAPICategorizer(platform)
                  .GetCategory(api) != 'internal'],
                 except_pass=FileNotFoundError)

    return All([get_platform_schemas(platform) for platform in GetPlatforms()])
