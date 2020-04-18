# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import posixpath

from compiled_file_system import Cache, SingleFile, Unicode
from extensions_paths import API_PATHS
from features_bundle import HasParent, GetParentName
from file_system import FileNotFoundError
from future import All, Future, Race
from operator import itemgetter
from path_util import Join
from platform_util import PlatformToExtensionType
from schema_processor import SchemaProcessor, SchemaProcessorFactory
from third_party.json_schema_compiler.json_schema import DeleteNodes
from third_party.json_schema_compiler.model import Namespace, UnixName


def GetNodeCategories():
  '''Returns a tuple of the possible categories a node may belong to.
  '''
  return ('types', 'functions', 'events', 'properties')


class ContentScriptAPI(object):
  '''Represents an API available to content scripts.

  |name| is the name of the API or API node this object represents.
  |restrictedTo| is a list of dictionaries representing the nodes
  of this API that are available to content scripts, or None if the
  entire API is available to content scripts.
  '''
  def __init__(self, name):
    self.name = name
    self.restrictedTo = None

  def __eq__(self, o):
    return self.name == o.name and self.restrictedTo == o.restrictedTo

  def __ne__(self, o):
    return not (self == o)

  def __repr__(self):
    return ('<ContentScriptAPI name=%s, restrictedTo=%s>' %
            (self.name, self.restrictedTo))

  def __str__(self):
    return repr(self)


class APIModels(object):
  '''Tracks APIs and their Models.
  '''

  def __init__(self,
               features_bundle,
               compiled_fs_factory,
               file_system,
               object_store_creator,
               platform,
               schema_processor_factory):
    self._features_bundle = features_bundle
    self._platform = PlatformToExtensionType(platform)
    self._model_cache = compiled_fs_factory.Create(
        file_system, self._CreateAPIModel, APIModels, category=self._platform)
    self._object_store = object_store_creator.Create(APIModels)
    self._schema_processor = Future(callback=lambda:
                                    schema_processor_factory.Create(False))

  @Cache
  @SingleFile
  @Unicode
  def _CreateAPIModel(self, path, data):
    def does_not_include_platform(node):
      return ('extension_types' in node and
              node['extension_types'] != 'all' and
              self._platform not in node['extension_types'])

    schema = self._schema_processor.Get().Process(path, data)[0]
    if not schema:
      raise ValueError('No schema for %s' % path)
    return Namespace(DeleteNodes(
        schema, matcher=does_not_include_platform), path)

  def GetNames(self):
    # API names appear alongside some of their methods/events/etc in the
    # features file. APIs are those which either implicitly or explicitly have
    # no parent feature (e.g. app, app.window, and devtools.inspectedWindow are
    # APIs; runtime.onConnectNative is not).
    api_features = self._features_bundle.GetAPIFeatures().Get()
    return [name for name, feature in api_features.iteritems()
            if not HasParent(name, feature, api_features)]

  def _GetPotentialPathsForModel(self, api_name):
    '''Returns the list of file system paths that the model for |api_name|
    might be located at.
    '''
    # By default |api_name| is assumed to be given without a path or extension,
    # so combinations of known paths and extension types will be searched.
    api_extensions = ('.json', '.idl')
    api_paths = API_PATHS

    # Callers sometimes include a file extension and/or prefix path with the
    # |api_name| argument. We believe them and narrow the search space
    # accordingly.
    name, ext = posixpath.splitext(api_name)
    if ext in api_extensions:
      api_extensions = (ext,)
      api_name = name
    for api_path in api_paths:
      if api_name.startswith(api_path):
        api_name = api_name[len(api_path):]
        api_paths = (api_path,)
        break

    # API names are given as declarativeContent and app.window but file names
    # will be declarative_content and app_window.
    file_name = UnixName(api_name).replace('.', '_')
    # Devtools APIs are in API/devtools/ not API/, and have their
    # "devtools" names removed from the file names.
    basename = posixpath.basename(file_name)
    if 'devtools_' in basename:
      file_name = posixpath.join(
          'devtools', file_name.replace(basename,
                                        basename.replace('devtools_' , '')))

    return [Join(path, file_name + ext) for ext in api_extensions
                                        for path in api_paths]

  def GetModel(self, api_name):
    futures = [self._model_cache.GetFromFile(path)
               for path in self._GetPotentialPathsForModel(api_name)]
    return Race(futures, except_pass=(FileNotFoundError, ValueError))

  def GetContentScriptAPIs(self):
    '''Creates a dict of APIs and nodes supported by content scripts in
    this format:

      {
        'extension': '<ContentScriptAPI name='extension',
                                        restrictedTo=[{'node': 'onRequest'}]>',
        ...
      }
    '''
    content_script_apis_future = self._object_store.Get('content_script_apis')
    api_features_future = self._features_bundle.GetAPIFeatures()
    def resolve():
      content_script_apis = content_script_apis_future.Get()
      if content_script_apis is not None:
        return content_script_apis

      api_features = api_features_future.Get()
      content_script_apis = {}
      for name, feature in api_features.iteritems():
        if 'content_script' not in feature.get('contexts', ()):
          continue
        parent = GetParentName(name, feature, api_features)
        if parent is None:
          content_script_apis[name] = ContentScriptAPI(name)
        else:
          # Creates a dict for the individual node.
          node = {'node': name[len(parent) + 1:]}
          if parent not in content_script_apis:
            content_script_apis[parent] = ContentScriptAPI(parent)
          if content_script_apis[parent].restrictedTo:
            content_script_apis[parent].restrictedTo.append(node)
          else:
            content_script_apis[parent].restrictedTo = [node]

      self._object_store.Set('content_script_apis', content_script_apis)
      return content_script_apis
    return Future(callback=resolve)

  def Refresh(self):
    futures = [self.GetModel(name) for name in self.GetNames()]
    return All(futures, except_pass=(FileNotFoundError, ValueError))

  def IterModels(self):
    future_models = [(name, self.GetModel(name)) for name in self.GetNames()]
    for name, future_model in future_models:
      try:
        model = future_model.Get()
      except FileNotFoundError:
        continue
      if model:
        yield name, model
