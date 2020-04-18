# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from copy import copy

from branch_utility import BranchUtility
from compiled_file_system import SingleFile, Unicode
from docs_server_utils import StringIdentity
from extensions_paths import API_PATHS, JSON_TEMPLATES
from file_system import FileNotFoundError
from future import All, Future
from path_util import Join
from platform_util import GetExtensionTypes, PlatformToExtensionType
from third_party.json_schema_compiler.json_parse import Parse


_API_FEATURES = '_api_features.json'
_MANIFEST_FEATURES = '_manifest_features.json'
_PERMISSION_FEATURES = '_permission_features.json'


def HasParent(feature_name, feature, all_feature_names):
  # A feature has a parent if it has a . in its name, its parent exists,
  # and it does not explicitly specify that it has no parent.
  return ('.' in feature_name and
          feature_name.rsplit('.', 1)[0] in all_feature_names and
          not feature.get('noparent'))


def GetParentName(feature_name, feature, all_feature_names):
  '''Returns the name of the parent feature, or None if it does not have a
  parent.
  '''
  if not HasParent(feature_name, feature, all_feature_names):
    return None
  return feature_name.rsplit('.', 1)[0]


def _CreateFeaturesFromJSONFutures(json_futures):
  '''Returns a dict of features. The value of each feature is a list with
  all of its possible values.
  '''
  def ignore_feature(name, value):
    '''Returns true if this feature should be ignored. Features are ignored if
    they are only available to whitelisted apps or component extensions/apps, as
    in these cases the APIs are not available to public developers.

    Private APIs are also unavailable to public developers, but logic elsewhere
    makes sure they are not listed. So they shouldn't be ignored via this
    mechanism.
    '''
    if name.endswith('Private'):
      return False
    return value.get('location') == 'component' or 'whitelist' in value

  features = {}

  for json_future in json_futures:
    try:
      features_json = Parse(json_future.Get())
    except FileNotFoundError:
      # Not all file system configurations have the extra files.
      continue
    for name, rawvalue in features_json.iteritems():
      if name not in features:
        features[name] = []
      for value in (rawvalue if isinstance(rawvalue, list) else (rawvalue,)):
        if not ignore_feature(name, value):
          features[name].append(value)

  return features


def _CopyParentFeatureValues(child, parent):
  '''Takes data from feature dict |parent| and copies/merges it
  into feature dict |child|. Two passes are run over the features,
  and on the first pass features are not resolved across caches,
  so a None value for |parent| may be passed in.
  '''
  if parent is None:
    return child
  merged = copy(parent)
  merged.pop('noparent', None)
  merged.pop('name', None)
  merged.update(child)
  return merged


def _ResolveFeature(feature_name,
                    feature_values,
                    extra_feature_values,
                    platform,
                    features_type,
                    features_map):
  '''Filters and combines the possible values for a feature into one dict.

  It uses |features_map| to resolve dependencies for each value and inherit
  unspecified platform and channel data. |feature_values| is then filtered
  by platform and all values with the most stable platform are merged into one
  dict. All values in |extra_feature_values| get merged into this dict.

  Returns |resolve_successful| and |feature|. |resolve_successful| is False
  if the feature's dependencies have not been merged yet themselves, meaning
  that this feature can not be reliably resolved yet. |feature| is the
  resulting feature dict, or None if the feature does not exist on the
  platform specified.
  '''
  feature = None
  most_stable_channel = None
  for value in feature_values:
    # If 'extension_types' or 'channel' is unspecified, these values should
    # be inherited from dependencies. If they are specified, these values
    # should override anything specified by dependencies.
    inherit_valid_platform = 'extension_types' not in value
    if inherit_valid_platform:
      valid_platform = None
    else:
      valid_platform = (value['extension_types'] == 'all' or
                        platform in value['extension_types'])
    inherit_channel = 'channel' not in value
    channel = value.get('channel')

    dependencies = value.get('dependencies', [])
    parent = GetParentName(
        feature_name, value, features_map[features_type]['all_names'])
    if parent is not None:
      # The parent data needs to be resolved so the child can inherit it.
      if parent in features_map[features_type].get('unresolved', ()):
        return False, None
      value = _CopyParentFeatureValues(
          value, features_map[features_type]['resolved'].get(parent))
      # Add the parent as a dependency to ensure proper platform filtering.
      dependencies.append(features_type + ':' + parent)

    for dependency in dependencies:
      dep_type, dep_name = dependency.split(':')
      if (dep_type not in features_map or
          dep_name in features_map[dep_type].get('unresolved', ())):
        # The dependency itself has not been merged yet or the features map
        # does not have the needed data. Fail to resolve.
        return False, None

      dep = features_map[dep_type]['resolved'].get(dep_name)
      if inherit_valid_platform and (valid_platform is None or valid_platform):
        # If dep is None, the dependency does not exist because it has been
        # filtered out by platform. This feature value does not explicitly
        # specify platform data, so filter this feature value out.
        # Only run this check if valid_platform is True or None so that it
        # can't be reset once it is False.
        valid_platform = dep is not None
      if inherit_channel and dep and 'channel' in dep:
        if channel is None or BranchUtility.NewestChannel(
            (dep['channel'], channel)) != channel:
          # Inherit the least stable channel from the dependencies.
          channel = dep['channel']

    # Default to stable on all platforms.
    if valid_platform is None:
      valid_platform = True
    if valid_platform and channel is None:
      channel = 'stable'

    if valid_platform:
      # The feature value is valid. Merge it into the feature dict.
      if feature is None or BranchUtility.NewestChannel(
          (most_stable_channel, channel)) != channel:
        # If this is the first feature value to be merged, copy the dict.
        # If this feature value has a more stable channel than the most stable
        # channel so far, replace the old dict so that it only merges values
        # from the most stable channel.
        feature = copy(value)
        most_stable_channel = channel
      elif channel == most_stable_channel:
        feature.update(value)

  if feature is None:
    # Nothing was left after filtering the values, but all dependency resolves
    # were successful. This feature does not exist on |platform|.
    return True, None

  # Merge in any extra values.
  for value in extra_feature_values:
    feature.update(value)

  # Cleanup, fill in missing fields.
  if 'name' not in feature:
    feature['name'] = feature_name
  feature['channel'] = most_stable_channel
  return True, feature


class _FeaturesCache(object):
  def __init__(self,
               file_system,
               compiled_fs_factory,
               json_paths,
               extra_paths,
               platform,
               features_type):
    self._cache = compiled_fs_factory.Create(
        file_system, self._CreateCache, type(self), category=platform)
    self._text_cache = compiled_fs_factory.ForUnicode(file_system)
    self._json_paths = json_paths
    self._extra_paths = extra_paths
    self._platform = platform
    self._features_type = features_type

  @Unicode
  def _CreateCache(self, _, features_json):
    json_path_futures = [self._text_cache.GetFromFile(path)
                         for path in self._json_paths[1:]]
    extra_path_futures = [self._text_cache.GetFromFile(path)
                          for path in self._extra_paths]

    features_values = _CreateFeaturesFromJSONFutures(
        [Future(value=features_json)] + json_path_futures)

    extra_features_values = _CreateFeaturesFromJSONFutures(extra_path_futures)

    features = {
      'resolved': {},
      'unresolved': copy(features_values),
      'extra': extra_features_values,
      'all_names': set(features_values.keys())
    }

    # Merges as many feature values as possible without resolving dependencies
    # from other FeaturesCaches. Pass in a features_map with just this
    # FeatureCache's features_type. Makes repeated passes until no new
    # resolves are successful.
    new_resolves = True
    while new_resolves:
      new_resolves = False
      for feature_name, feature_values in features_values.iteritems():
        if feature_name not in features['unresolved']:
          continue
        resolve_successful, feature = _ResolveFeature(
            feature_name,
            feature_values,
            extra_features_values.get(feature_name, ()),
            self._platform,
            self._features_type,
            {self._features_type: features})
        if resolve_successful:
          del features['unresolved'][feature_name]
          new_resolves = True
          if feature is not None:
            features['resolved'][feature_name] = feature

    return features

  def GetFeatures(self):
    if not self._json_paths:
      return Future(value={})
    return self._cache.GetFromFile(self._json_paths[0])


class FeaturesBundle(object):
  '''Provides access to properties of API, Manifest, and Permission features.
  '''
  def __init__(self,
               file_system,
               compiled_fs_factory,
               object_store_creator,
               platform):
    def create_features_cache(features_type, feature_file, *extra_paths):
      return _FeaturesCache(
          file_system,
          compiled_fs_factory,
          [Join(path, feature_file) for path in API_PATHS],
          extra_paths,
          self._platform,
          features_type)

    if platform not in GetExtensionTypes():
      self._platform = PlatformToExtensionType(platform)
    else:
      self._platform = platform

    self._caches = {
      'api': create_features_cache('api', _API_FEATURES),
      'manifest': create_features_cache(
          'manifest',
          _MANIFEST_FEATURES,
          Join(JSON_TEMPLATES, 'manifest.json')),
      'permission': create_features_cache(
          'permission',
          _PERMISSION_FEATURES,
          Join(JSON_TEMPLATES, 'permissions.json'))
    }
    # Namespace the object store by the file system ID because this class is
    # used by the availability finder cross-channel.
    self._object_store = object_store_creator.Create(
        _FeaturesCache,
        category=StringIdentity(file_system.GetIdentity(), self._platform))

  def GetPermissionFeatures(self):
    return self.GetFeatures('permission', ('permission',))

  def GetManifestFeatures(self):
    return self.GetFeatures('manifest', ('manifest',))

  def GetAPIFeatures(self):
    return self.GetFeatures('api', ('api', 'manifest', 'permission'))

  def GetFeatures(self, features_type, dependencies):
    '''Resolves all dependencies in the categories specified by |dependencies|.
    Returns the features in the |features_type| category.
    '''
    def next_(features):
      if features is not None:
        return Future(value=features)

      dependency_futures = []
      cache_types = []
      for cache_type in dependencies:
        cache_types.append(cache_type)
        dependency_futures.append(self._object_store.Get(cache_type))

      def load_features(dependency_features_list):
        futures = []
        for dependency_features, cache_type in zip(dependency_features_list,
                                                   cache_types):
          if dependency_features is not None:
            # Get cached dependencies if possible. If it has been cached, all
            # of its features have been resolved, so the other fields are
            # unnecessary.
            futures.append(Future(value={'resolved': dependency_features}))
          else:
            futures.append(self._caches[cache_type].GetFeatures())

        def resolve(features):
          features_map = {}
          for cache_type, feature in zip(cache_types, features):
            # Copy down to features_map level because the 'resolved' and
            # 'unresolved' dicts will be modified.
            features_map[cache_type] = dict((c, copy(d))
                                            for c, d in feature.iteritems())

          def has_unresolved():
            '''Determines if there are any unresolved features left over in any
            of the categories in |dependencies|.
            '''
            return any(cache.get('unresolved')
                       for cache in features_map.itervalues())

          # Iterate until everything is resolved. If dependencies are multiple
          # levels deep, it might take multiple passes to inherit data to the
          # topmost feature.
          while has_unresolved():
            for cache_type, cache in features_map.iteritems():
              if 'unresolved' not in cache:
                continue
              to_remove = []
              for name, values in cache['unresolved'].iteritems():
                resolve_successful, feature = _ResolveFeature(
                    name,
                    values,
                    cache['extra'].get(name, ()),
                    self._platform,
                    cache_type,
                    features_map)
                if not resolve_successful:
                  continue  # Try again on the next iteration of the while loop

                # When successfully resolved, remove it from the unresolved
                # dict. Add it to the resolved dict if it didn't get deleted.
                to_remove.append(name)
                if feature is not None:
                  cache['resolved'][name] = feature

              for key in to_remove:
                del cache['unresolved'][key]

          for cache_type, cache in features_map.iteritems():
            self._object_store.Set(cache_type, cache['resolved'])
          return features_map[features_type]['resolved']
        return All(futures).Then(resolve)
      return All(dependency_futures).Then(load_features)
    return self._object_store.Get(features_type).Then(next_)
