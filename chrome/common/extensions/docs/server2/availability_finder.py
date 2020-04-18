# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import posixpath

from api_models import GetNodeCategories
from api_schema_graph import APISchemaGraph
from branch_utility import BranchUtility, ChannelInfo
from compiled_file_system import Cache, CompiledFileSystem, SingleFile, Unicode
from extensions_paths import API_PATHS, JSON_TEMPLATES
from features_bundle import FeaturesBundle
from file_system import FileNotFoundError
from schema_processor import SchemaProcessor
from third_party.json_schema_compiler.memoize import memoize
from third_party.json_schema_compiler.model import UnixName


_DEVTOOLS_API = 'devtools_api.json'
_EXTENSION_API = 'extension_api.json'
# The version where api_features.json is first available.
_API_FEATURES_MIN_VERSION = 28
# The version where permission_ and manifest_features.json are available and
# presented in the current format.
_ORIGINAL_FEATURES_MIN_VERSION = 20
# API schemas are aggregated in extension_api.json up to this version.
_EXTENSION_API_MAX_VERSION = 17
# The earliest version for which we have SVN data.
_SVN_MIN_VERSION = 5


def _GetChannelFromFeatures(api_name, features):
  '''Finds API channel information for |api_name| from |features|.
  Returns None if channel information for the API cannot be located.
  '''
  feature = features.Get().get(api_name)
  return feature.get('channel') if feature else None


def _GetChannelFromAPIFeatures(api_name, features_bundle):
  return _GetChannelFromFeatures(api_name, features_bundle.GetAPIFeatures())


def _GetChannelFromManifestFeatures(api_name, features_bundle):
  # _manifest_features.json uses unix_style API names.
  api_name = UnixName(api_name)
  return _GetChannelFromFeatures(api_name,
                                 features_bundle.GetManifestFeatures())


def _GetChannelFromPermissionFeatures(api_name, features_bundle):
  return _GetChannelFromFeatures(api_name,
                                 features_bundle.GetPermissionFeatures())


def _GetAPISchemaFilename(api_name, file_system, version):
  '''Gets the name of the file which may contain the schema for |api_name| in
  |file_system|, or None if the API is not found. Note that this may be the
  single _EXTENSION_API file which all APIs share in older versions of Chrome,
  in which case it is unknown whether the API actually exists there.
  '''
  if version == 'master' or version > _ORIGINAL_FEATURES_MIN_VERSION:
    # API schema filenames switch format to unix_hacker_style.
    api_name = UnixName(api_name)

  # Devtools API names have 'devtools.' prepended to them.
  # The corresponding filenames do not.
  if 'devtools_' in api_name:
    api_name = api_name.replace('devtools_', '')

  for api_path in API_PATHS:
    try:
      for base, _, filenames in file_system.Walk(api_path):
        for ext in ('json', 'idl'):
          filename = '%s.%s' % (api_name, ext)
          if filename in filenames:
            return posixpath.join(api_path, base, filename)
          if _EXTENSION_API in filenames:
            return posixpath.join(api_path, base, _EXTENSION_API)
    except FileNotFoundError:
      continue
  return None


class AvailabilityInfo(object):
  '''Represents availability data for an API. |scheduled| is a version number
  specifying when dev and beta APIs will become stable, or None if that data
  is unknown.
  '''
  def __init__(self, channel_info, scheduled=None):
    assert isinstance(channel_info, ChannelInfo)
    assert isinstance(scheduled, int) or scheduled is None
    self.channel_info = channel_info
    self.scheduled = scheduled

  def __eq__(self, other):
    return self.__dict__ == other.__dict__

  def __ne__(self, other):
    return not (self == other)

  def __repr__(self):
    return '%s%s' % (type(self).__name__, repr(self.__dict__))

  def __str__(self):
    return repr(self)


class AvailabilityFinder(object):
  '''Generates availability information for APIs by looking at API schemas and
  _features files over multiple release versions of Chrome.
  '''

  def __init__(self,
               branch_utility,
               compiled_fs_factory,
               file_system_iterator,
               host_file_system,
               object_store_creator,
               platform,
               schema_processor_factory):
    self._branch_utility = branch_utility
    self._compiled_fs_factory = compiled_fs_factory
    self._file_system_iterator = file_system_iterator
    self._host_file_system = host_file_system
    self._object_store_creator = object_store_creator
    def create_object_store(category):
      return object_store_creator.Create(
          AvailabilityFinder, category='/'.join((platform, category)))
    self._top_level_object_store = create_object_store('top_level')
    self._node_level_object_store = create_object_store('node_level')
    self._json_fs = compiled_fs_factory.ForJson(self._host_file_system)
    self._platform = platform
    # When processing the API schemas, we retain inlined types in the schema
    # so that there are not missing nodes in the APISchemaGraphs when trying
    # to lookup availability.
    self._schema_processor = schema_processor_factory.Create(True)

  def _GetPredeterminedNodeAvailability(self, node_name):
    '''Checks a configuration file for hardcoded (i.e. predetermined)
    availability information for an API node.
    '''
    node_info = self._json_fs.GetFromFile(
        JSON_TEMPLATES + 'api_availabilities.json').Get().get(node_name)
    if node_info is None:
      return None

    channel_info = None
    if node_info['channel'] == 'stable':
      channel_info = self._branch_utility.GetStableChannelInfo(
          node_info['version'])
    else:
      channel_info = self._branch_utility.GetChannelInfo(node_info['channel'])
    return AvailabilityInfo(channel_info) if channel_info else None

  @memoize
  def _CreateAPISchemaFileSystem(self, file_system):
    '''Creates a CompiledFileSystem for parsing raw JSON or IDL API schema
    data and formatting it so that it can be used to create APISchemaGraphs.
    '''
    def process_schema(path, data):
      return self._schema_processor.Process(path, data)
    return self._compiled_fs_factory.Create(
        file_system,
        Cache(SingleFile(Unicode(process_schema))),
        CompiledFileSystem,
        category='api-schema')

  def _GetAPISchema(self, api_name, file_system, version):
    '''Searches |file_system| for |api_name|'s API schema data, and processes
    and returns it if found.
    '''
    api_filename = _GetAPISchemaFilename(api_name, file_system, version)
    if api_filename is None:
      # No file for the API could be found in the given |file_system|.
      return None

    schema_fs = self._CreateAPISchemaFileSystem(file_system)
    api_schemas = schema_fs.GetFromFile(api_filename).Get()
    matching_schemas = [api for api in api_schemas
                        if api and api['namespace'] == api_name]
    # There should only be a single matching schema per file, or zero in the
    # case of no API data being found in _EXTENSION_API.
    assert len(matching_schemas) <= 1
    return matching_schemas or None

  def _HasAPISchema(self, api_name, file_system, version):
    '''Whether or not an API schema for |api_name| exists in the given
    |file_system|.
    '''
    filename = _GetAPISchemaFilename(api_name, file_system, version)
    if filename is None:
      return False
    if filename.endswith(_EXTENSION_API) or filename.endswith(_DEVTOOLS_API):
      return self._GetAPISchema(api_name, file_system, version) is not None
    return True

  def _CheckStableAvailability(self,
                               api_name,
                               file_system,
                               version,
                               earliest_version=None):
    '''Checks for availability of an API, |api_name|, on the stable channel.
    Considers several _features.json files, file system existence, and
    extension_api.json depending on the given |version|.
    |earliest_version| is the version of Chrome at which |api_name| first became
    available. It should only be given when checking stable availability for
    API nodes, so it can be used as an alternative to the check for filesystem
    existence.
    '''
    earliest_version = earliest_version or _SVN_MIN_VERSION
    if version < earliest_version:
      # SVN data isn't available below this version.
      return False
    features_bundle = self._CreateFeaturesBundle(file_system)
    available_channel = None
    if version >= _API_FEATURES_MIN_VERSION:
      # The _api_features.json file first appears in version 28 and should be
      # the most reliable for finding API availability.
      available_channel = _GetChannelFromAPIFeatures(api_name,
                                                     features_bundle)
    if version >= _ORIGINAL_FEATURES_MIN_VERSION:
      # The _permission_features.json and _manifest_features.json files are
      # present in Chrome 20 and onwards. Use these if no information could be
      # found using _api_features.json.
      available_channel = (
          available_channel or
          _GetChannelFromPermissionFeatures(api_name, features_bundle) or
          _GetChannelFromManifestFeatures(api_name, features_bundle))
      if available_channel is not None:
        return available_channel == 'stable'

    # |earliest_version| == _SVN_MIN_VERSION implies we're dealing with an API.
    # Fall back to a check for file system existence if the API is not
    # stable in any of the _features.json files, or if the _features files
    # do not exist (version 19 and earlier).
    if earliest_version == _SVN_MIN_VERSION:
      return self._HasAPISchema(api_name, file_system, version)
    # For API nodes, assume it's available if |version| is greater than the
    # version the node became available (which it is, because of the first
    # check).
    return True

  def _CheckChannelAvailability(self, api_name, file_system, channel_info):
    '''Searches through the _features files in a given |file_system|, falling
    back to checking the file system for API schema existence, to determine
    whether or not an API is available on the given channel, |channel_info|.
    '''
    features_bundle = self._CreateFeaturesBundle(file_system)
    available_channel = (
        _GetChannelFromAPIFeatures(api_name, features_bundle) or
        _GetChannelFromPermissionFeatures(api_name, features_bundle) or
        _GetChannelFromManifestFeatures(api_name, features_bundle))
    if (available_channel is None and
        self._HasAPISchema(api_name, file_system, channel_info.version)):
      # If an API is not represented in any of the _features files, but exists
      # in the filesystem, then assume it is available in this version.
      # The chrome.windows API is an example of this.
      available_channel = channel_info.channel
    # If the channel we're checking is the same as or newer than the
    # |available_channel| then the API is available at this channel.
    newest = BranchUtility.NewestChannel((available_channel,
                                          channel_info.channel))
    return available_channel is not None and newest == channel_info.channel

  def _CheckChannelAvailabilityForNode(self,
                                       node_name,
                                       file_system,
                                       channel_info,
                                       earliest_channel_info):
    '''Searches through the _features files in a given |file_system| to
    determine whether or not an API node is available on the given channel,
    |channel_info|. |earliest_channel_info| is the earliest channel the node
    was introduced.
    '''
    features_bundle = self._CreateFeaturesBundle(file_system)
    available_channel = None
    # Only API nodes can have their availability overriden on a per-node basis,
    # so we only need to check _api_features.json.
    if channel_info.version >= _API_FEATURES_MIN_VERSION:
      available_channel = _GetChannelFromAPIFeatures(node_name, features_bundle)
    if (available_channel is None and
        channel_info.version >= earliest_channel_info.version):
      # Most API nodes inherit their availabiltity from their parent, so don't
      # explicitly appear in _api_features.json. For example, "tabs.create"
      # isn't listed; it inherits from "tabs". Assume these are available at
      # |channel_info|.
      available_channel = channel_info.channel
    newest = BranchUtility.NewestChannel((available_channel,
                                          channel_info.channel))
    return available_channel is not None and newest == channel_info.channel

  @memoize
  def _CreateFeaturesBundle(self, file_system):
    return FeaturesBundle(file_system,
                          self._compiled_fs_factory,
                          self._object_store_creator,
                          self._platform)

  def _CheckAPIAvailability(self, api_name, file_system, channel_info):
    '''Determines the availability for an API at a certain version of Chrome.
    Two branches of logic are used depending on whether or not the API is
    determined to be 'stable' at the given version.
    '''
    if channel_info.channel == 'stable':
      return self._CheckStableAvailability(api_name,
                                           file_system,
                                           channel_info.version)
    return self._CheckChannelAvailability(api_name,
                                          file_system,
                                          channel_info)

  def _FindScheduled(self, api_name, earliest_version=None):
    '''Determines the earliest version of Chrome where the API is stable.
    Unlike the code in GetAPIAvailability, this checks if the API is stable
    even when Chrome is in dev or beta, which shows that the API is scheduled
    to be stable in that verison of Chrome. |earliest_version| is the version
    |api_name| became first available. Only use it when finding scheduled
    availability for nodes.
    '''
    def check_scheduled(file_system, channel_info):
      return self._CheckStableAvailability(api_name,
                                           file_system,
                                           channel_info.version,
                                           earliest_version=earliest_version)

    stable_channel = self._file_system_iterator.Descending(
        self._branch_utility.GetChannelInfo('dev'), check_scheduled)

    return stable_channel.version if stable_channel else None

  def _CheckAPINodeAvailability(self, node_name, earliest_channel_info):
    '''Gets availability data for a node by checking _features files.
    '''
    # Check for predetermined availability and cache this information if found.
    availability = self._GetPredeterminedNodeAvailability(node_name)
    if availability is not None:
      self._top_level_object_store.Set(node_name, availability)
      return availability

    def check_node_availability(file_system, channel_info):
      return self._CheckChannelAvailabilityForNode(node_name,
                                                   file_system,
                                                   channel_info,
                                                   earliest_channel_info)
    channel_info = (self._file_system_iterator.Descending(
        self._branch_utility.GetChannelInfo('dev'), check_node_availability) or
        earliest_channel_info)

    if channel_info.channel == 'stable':
      scheduled = None
    else:
      scheduled = self._FindScheduled(
          node_name,
          earliest_version=earliest_channel_info.version)

    return AvailabilityInfo(channel_info, scheduled=scheduled)

  def GetAPIAvailability(self, api_name):
    '''Performs a search for an API's top-level availability by using a
    HostFileSystemIterator instance to traverse multiple version of the
    SVN filesystem.
    '''
    availability = self._top_level_object_store.Get(api_name).Get()
    if availability is not None:
      return availability

    # Check for predetermined availability and cache this information if found.
    availability = self._GetPredeterminedNodeAvailability(api_name)
    if availability is not None:
      self._top_level_object_store.Set(api_name, availability)
      return availability

    def check_api_availability(file_system, channel_info):
      return self._CheckAPIAvailability(api_name, file_system, channel_info)

    channel_info = self._file_system_iterator.Descending(
        self._branch_utility.GetChannelInfo('dev'),
        check_api_availability)
    if channel_info is None:
      # The API wasn't available on 'dev', so it must be a 'master'-only API.
      channel_info = self._branch_utility.GetChannelInfo('master')

    # If the API is not stable, check when it will be scheduled to be stable.
    if channel_info.channel == 'stable':
      scheduled = None
    else:
      scheduled = self._FindScheduled(api_name)

    availability = AvailabilityInfo(channel_info, scheduled=scheduled)

    self._top_level_object_store.Set(api_name, availability)
    return availability

  def GetAPINodeAvailability(self, api_name):
    '''Returns an APISchemaGraph annotated with each node's availability (the
    ChannelInfo at the oldest channel it's available in).
    '''
    availability_graph = self._node_level_object_store.Get(api_name).Get()
    if availability_graph is not None:
      return availability_graph

    def assert_not_none(value):
      assert value is not None
      return value

    availability_graph = APISchemaGraph()
    host_fs = self._host_file_system
    master_stat = assert_not_none(host_fs.Stat(_GetAPISchemaFilename(
        api_name, host_fs, 'master')))

    # Weird object thing here because nonlocal is Python 3.
    previous = type('previous', (object,), {'stat': None, 'graph': None})

    def update_availability_graph(file_system, channel_info):
      # If we can't find a filename, skip checking at this branch.
      # For example, something could have a predetermined availability of 23,
      # but it doesn't show up in the file system until 26.
      # We know that the file will become available at some point.
      #
      # The problem with this is that at the first version where the API file
      # exists, we'll get a huge chunk of new objects that don't match
      # the predetermined API availability.
      version_filename = _GetAPISchemaFilename(api_name,
                                               file_system,
                                               channel_info.version)
      if version_filename is None:
        # Continue the loop at the next version.
        return True

      version_stat = assert_not_none(file_system.Stat(version_filename))

      # Important optimisation: only re-parse the graph if the file changed in
      # the last revision. Parsing the same schema and forming a graph on every
      # iteration is really expensive.
      if version_stat == previous.stat:
        version_graph = previous.graph
      else:
        # Keep track of any new schema elements from this version by adding
        # them to |availability_graph|.
        #
        # Calling |availability_graph|.Lookup() on the nodes being updated
        # will return the |annotation| object -- the current |channel_info|.
        version_graph = APISchemaGraph(
            api_schema=self._GetAPISchema(api_name,
                                          file_system,
                                          channel_info.version))
        def annotator(node_name):
          return self._CheckAPINodeAvailability('%s.%s' % (api_name, node_name),
                                                channel_info)

        availability_graph.Update(version_graph.Subtract(availability_graph),
                                  annotator)

      previous.stat = version_stat
      previous.graph = version_graph

      # Continue looping until there are no longer differences between this
      # version and master.
      return version_stat != master_stat

    self._file_system_iterator.Ascending(
        self.GetAPIAvailability(api_name).channel_info,
        update_availability_graph)

    self._node_level_object_store.Set(api_name, availability_graph)
    return availability_graph
