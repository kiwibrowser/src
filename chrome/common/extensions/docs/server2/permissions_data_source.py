# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from itertools import ifilter
from operator import itemgetter

from data_source import DataSource
from extensions_paths import PRIVATE_TEMPLATES
from future import Future
from platform_util import GetPlatforms


def _ListifyPermissions(permissions):
  '''Filter out any permissions that do not have a description or with a name
  that ends with Private then sort permissions features by name into a list.
  '''
  def filter_permissions(perm):
    return 'description' in perm and not perm['name'].endswith('Private')

  return sorted(
      ifilter(filter_permissions, permissions.itervalues()),
      key=itemgetter('name'))


def _AddDependencyDescriptions(permissions, api_features):
  '''Use |api_features| to determine the dependencies APIs have on permissions.
  Add descriptions to |permissions| based on those dependencies.
  '''
  for name, permission in permissions.iteritems():
    # Don't overwrite the description created by expanding a partial template.
    if 'partial' in permission:
      continue

    has_deps = False
    if name in api_features:
      for dependency in api_features[name].get('dependencies', ()):
        if dependency.startswith('permission:'):
          has_deps = True

    if has_deps:
      permission['partial'] = 'permissions/generic_description.html'

class PermissionsDataSource(DataSource):
  '''Load and format permissions features to be used by templates.
  '''
  def __init__(self, server_instance, request):
    self._platform_bundle = server_instance.platform_bundle
    self._object_store = server_instance.object_store_creator.Create(
        PermissionsDataSource)
    self._template_cache = server_instance.compiled_fs_factory.ForTemplates(
        server_instance.host_file_system_provider.GetMaster())

  def _CreatePermissionsDataForPlatform(self, platform):
    features_bundle = self._platform_bundle.GetFeaturesBundle(platform)
    api_features_future = features_bundle.GetAPIFeatures()
    permission_features_future = features_bundle.GetPermissionFeatures()

    def resolve():
      api_features = api_features_future.Get()
      permission_features = permission_features_future.Get()
      _AddDependencyDescriptions(permission_features, api_features)

      # Turn partial templates into descriptions, ensure anchors are set.
      for permission in permission_features.values():
        if not 'anchor' in permission:
          permission['anchor'] = permission['name']
        if 'partial' in permission:
          permission['description'] = self._template_cache.GetFromFile(
              PRIVATE_TEMPLATES + permission['partial']).Get()
          del permission['partial']

      return _ListifyPermissions(permission_features)
    return Future(callback=resolve)

  def _CreatePermissionsData(self):
    permissions_data_futures = dict(
        (platform, self._CreatePermissionsDataForPlatform(platform))
        for platform in GetPlatforms())

    def resolve():
      return dict(('declare_' + platform, future.Get())
                  for platform, future in permissions_data_futures.iteritems())
    return Future(callback=resolve)

  def _GetCachedPermissionsData(self):
    data = self._object_store.Get('permissions_data').Get()
    if data is None:
      data = self._CreatePermissionsData().Get()
      self._object_store.Set('permissions_data', data)
    return data

  def get(self, key):
    return self._GetCachedPermissionsData().get(key)

  def Refresh(self):
    return self._CreatePermissionsData()
