# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from itertools import groupby
from operator import itemgetter
import posixpath

from data_source import DataSource
from extensions_paths import JSON_TEMPLATES, PUBLIC_TEMPLATES
from future import Future
from platform_util import GetPlatforms


class WhatsNewDataSource(DataSource):
  ''' This class creates a list of "what is new" by chrome version.
  '''

  def __init__(self, server_instance, _):
    self._parse_cache = server_instance.compiled_fs_factory.ForJson(
        server_instance.host_file_system_provider.GetMaster())
    self._object_store = server_instance.object_store_creator.Create(
        WhatsNewDataSource)
    self._platform_bundle = server_instance.platform_bundle

  def _GenerateChangesListWithVersion(self, platform, whats_new_json):
    return [{
      'id': change_id,
      'type': change['type'],
      'description': change['description'],
      'version': change['version']
    } for change_id, change in whats_new_json.iteritems()]

  def _GetAPIVersion(self, platform, api_name):
    version = None
    category = self._platform_bundle.GetAPICategorizer(platform).GetCategory(
        api_name)
    if category == 'chrome':
      channel_info = self._platform_bundle.GetAvailabilityFinder(
          platform).GetAPIAvailability(api_name).channel_info
      channel = channel_info.channel
      if channel == 'stable':
        version = channel_info.version
    return version

  def _GenerateAPIListWithVersion(self, platform):
    data = []
    for api_name, api_model in self._platform_bundle.GetAPIModels(
        platform).IterModels():
      version = self._GetAPIVersion(platform, api_name)
      if version:
        api = {
          'name': api_name,
          'description': api_model.description,
          'version' : version,
          'type': 'apis',
        }
        data.append(api)
    data.sort(key=itemgetter('version'))
    return data

  def _GenerateWhatsNewDict(self):
    whats_new_json_future = self._parse_cache.GetFromFile(
        posixpath.join(JSON_TEMPLATES, 'whats_new.json'))
    def _MakeDictByPlatform(platform):
      whats_new_json = whats_new_json_future.Get()
      platform_list = []
      apis = self._GenerateAPIListWithVersion(platform)
      apis.extend(self._GenerateChangesListWithVersion(platform,
          whats_new_json))
      apis.sort(key=itemgetter('version'), reverse=True)
      for version, group in groupby(apis, key=itemgetter('version')):
        whats_new_by_version = {
          'version': version,
        }
        for item in group:
          item_type = item['type']
          if item_type not in whats_new_by_version:
            whats_new_by_version[item_type] = []
          whats_new_by_version[item_type].append(item)
        platform_list.append(whats_new_by_version)
      return platform_list

    def resolve():
      return dict((platform, _MakeDictByPlatform(platform))
          for platform in GetPlatforms())
    return Future(callback=resolve)

  def _GetCachedWhatsNewData(self):
    data = self._object_store.Get('whats_new_data').Get()
    if data is None:
      data = self._GenerateWhatsNewDict().Get()
      self._object_store.Set('whats_new_data', data)
    return data

  def get(self, key):
    return self._GetCachedWhatsNewData().get(key)

  def Refresh(self):
    return self._GenerateWhatsNewDict()
