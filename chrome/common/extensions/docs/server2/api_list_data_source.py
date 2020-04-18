# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from data_source import DataSource
from future import Future
from operator import itemgetter
from platform_util import GetPlatforms

from docs_server_utils import MarkFirstAndLast, MarkLast

class APIListDataSource(DataSource):
  """ This class creates a list of chrome.* APIs and chrome.experimental.* APIs
  for extensions and apps that are used in the api_index.html,
  experimental.html, and private_apis.html pages.

  An API is considered listable if it is listed in _api_features.json,
  it has a corresponding HTML file in the public template path, and one of
  the following conditions is met:
    - It has no "dependencies" or "extension_types" properties in _api_features
    - It has an "extension_types" property in _api_features with either/both
      "extension"/"platform_app" values present.
    - It has a dependency in _{api,manifest,permission}_features with an
      "extension_types" property where either/both "extension"/"platform_app"
      values are present.
  """
  def __init__(self, server_instance, _):
    self._platform_bundle = server_instance.platform_bundle
    self._object_store = server_instance.object_store_creator.Create(
        # Update the model when the API or Features model updates.
        APIListDataSource, category=self._platform_bundle.GetIdentity())

  def _GenerateAPIDict(self):
    def make_list_for_content_scripts():
      def convert_to_list(content_script_apis):
        content_script_apis_list = [csa.__dict__ for api_name, csa
                                    in content_script_apis.iteritems()
                                    if self._platform_bundle.GetAPICategorizer(
                                        'extensions').IsDocumented(api_name)]
        content_script_apis_list.sort(key=itemgetter('name'))
        for csa in content_script_apis_list:
          restricted_nodes = csa['restrictedTo']
          if restricted_nodes:
            restricted_nodes.sort(key=itemgetter('node'))
            MarkFirstAndLast(restricted_nodes)
          else:
            del csa['restrictedTo']
        return content_script_apis_list

      return (self._platform_bundle.GetAPIModels('extensions')
              .GetContentScriptAPIs()
              .Then(convert_to_list))

    def make_dict_for_platform(platform):
      platform_dict = {
        'chrome': {'stable': [], 'beta': [], 'dev': [], 'master': []},
      }
      private_apis = []
      experimental_apis = []
      all_apis = []
      for api_name, api_model in self._platform_bundle.GetAPIModels(
          platform).IterModels():
        if not self._platform_bundle.GetAPICategorizer(platform).IsDocumented(
            api_name):
          continue
        api = {
          'name': api_name,
          'description': api_model.description,
        }
        category = self._platform_bundle.GetAPICategorizer(
            platform).GetCategory(api_name)
        if category == 'chrome':
          channel_info = self._platform_bundle.GetAvailabilityFinder(
              platform).GetAPIAvailability(api_name).channel_info
          channel = channel_info.channel
          if channel == 'stable':
            version = channel_info.version
            api['version'] = version
          platform_dict[category][channel].append(api)
          all_apis.append(api)
        elif category == 'experimental':
          experimental_apis.append(api)
          all_apis.append(api)
        elif category == 'private':
          private_apis.append(api)

      for channel, apis_by_channel in platform_dict['chrome'].iteritems():
        apis_by_channel.sort(key=itemgetter('name'))
        MarkLast(apis_by_channel)
        platform_dict['chrome'][channel] = apis_by_channel

      for key, apis in (('all', all_apis),
                        ('private', private_apis),
                        ('experimental', experimental_apis)):
        apis.sort(key=itemgetter('name'))
        MarkLast(apis)
        platform_dict[key] = apis

      return platform_dict

    def make_api_dict(content_script_apis):
      api_dict = dict((platform, make_dict_for_platform(platform))
                       for platform in GetPlatforms())
      api_dict['contentScripts'] = content_script_apis
      return api_dict

    return make_list_for_content_scripts().Then(make_api_dict)

  def _GetCachedAPIData(self):
    def persist_and_return(data):
      self._object_store.Set('api_data', data)
      return data
    def return_or_generate(data):
      if data is None:
        return self._GenerateAPIDict().Then(persist_and_return)
      return data
    return self._object_store.Get('api_data').Then(return_or_generate)

  def get(self, key):
    return self._GetCachedAPIData().Get().get(key)

  def Refresh(self):
    return self._GetCachedAPIData()
