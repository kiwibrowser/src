# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from collections import defaultdict, Mapping
import traceback

from third_party.json_schema_compiler import json_parse, idl_schema, idl_parser
from reference_resolver import ReferenceResolver
from compiled_file_system import CompiledFileSystem

class SchemaProcessorForTest(object):
  '''Fake SchemaProcessor class. Returns the original schema, without
  processing.
  '''
  def Process(self, path, file_data):
    if path.endswith('.idl'):
      idl = idl_schema.IDLSchema(idl_parser.IDLParser().ParseData(file_data))
      # Wrap the result in a list so that it behaves like JSON API data.
      return [idl.process()[0]]
    return json_parse.Parse(file_data)

class SchemaProcessorFactoryForTest(object):
  '''Returns a fake SchemaProcessor class to be used for testing.
  '''
  def Create(self, retain_inlined_types):
    return SchemaProcessorForTest()


class SchemaProcessorFactory(object):
  '''Factory for creating the schema processing utility.
  '''
  def __init__(self,
               reference_resolver,
               api_models,
               features_bundle,
               compiled_fs_factory,
               file_system):
    self._reference_resolver = reference_resolver
    self._api_models = api_models
    self._features_bundle = features_bundle
    self._compiled_fs_factory = compiled_fs_factory
    self._file_system = file_system

  def Create(self, retain_inlined_types):
    return SchemaProcessor(self._reference_resolver.Get(),
                           self._api_models.Get(),
                           self._features_bundle.Get(),
                           self._compiled_fs_factory,
                           self._file_system,
                           retain_inlined_types)


class SchemaProcessor(object):
  '''Helper for parsing the API schema.
  '''
  def __init__(self,
               reference_resolver,
               api_models,
               features_bundle,
               compiled_fs_factory,
               file_system,
               retain_inlined_types):
    self._reference_resolver = reference_resolver
    self._api_models = api_models
    self._features_bundle = features_bundle
    self._retain_inlined_types = retain_inlined_types
    self._compiled_file_system = compiled_fs_factory.Create(
        file_system, self.Process, SchemaProcessor, category='json-cache')
    self._api_stack = []

  def _RemoveNoDocs(self, item):
    '''Removes nodes that should not be rendered from an API schema.
    '''
    if json_parse.IsDict(item):
      if item.get('nodoc', False):
        return True
      for key, value in item.items():
        if self._RemoveNoDocs(value):
          del item[key]
    elif type(item) == list:
      to_remove = []
      for i in item:
        if self._RemoveNoDocs(i):
          to_remove.append(i)
      for i in to_remove:
        item.remove(i)
    return False


  def _DetectInlineableTypes(self, schema):
    '''Look for documents that are only referenced once and mark them as inline.
    Actual inlining is done by _InlineDocs.
    '''
    if not schema.get('types'):
      return

    ignore = frozenset(('value', 'choices'))
    refcounts = defaultdict(int)
    # Use an explicit stack instead of recursion.
    stack = [schema]

    while stack:
      node = stack.pop()
      if isinstance(node, list):
        stack.extend(node)
      elif isinstance(node, Mapping):
        if '$ref' in node:
          refcounts[node['$ref']] += 1
        stack.extend(v for k, v in node.iteritems() if k not in ignore)

    for type_ in schema['types']:
      if not 'noinline_doc' in type_:
        if refcounts[type_['id']] == 1:
          type_['inline_doc'] = True


  def _InlineDocs(self, schema):
    '''Replace '$ref's that refer to inline_docs with the json for those docs.
    If |retain_inlined_types| is False, then the inlined nodes are removed
    from the schema.
    '''
    inline_docs = {}
    types_without_inline_doc = []
    internal_api = False

    api_features = self._features_bundle.GetAPIFeatures().Get()
    # We don't want to inline the events API, as it's handled differently
    # Also, the webviewTag API is handled differently, as it only exists
    # for the purpose of documentation, it's not a true internal api
    namespace = schema.get('namespace', '')
    if namespace != 'events' and namespace != 'webviewTag':
      internal_api = api_features.get(schema.get('namespace', ''), {}).get(
          'internal', False)

    api_refs = set()
    # Gather refs to internal APIs
    def gather_api_refs(node):
      if isinstance(node, list):
        for i in node:
          gather_api_refs(i)
      elif isinstance(node, Mapping):
        ref = node.get('$ref')
        if ref:
          api_refs.add(ref)
        for k, v in node.iteritems():
          gather_api_refs(v)
    gather_api_refs(schema)

    if len(api_refs) > 0:
      api_list = self._api_models.GetNames()
      api_name = schema.get('namespace', '')
      self._api_stack.append(api_name)
      for api in self._api_stack:
        if api in api_list:
          api_list.remove(api)
      for ref in api_refs:
        model, node_info = self._reference_resolver.GetRefModel(ref, api_list)
        if model and api_features.get(model.name, {}).get('internal', False):
          category, name = node_info
          for ref_schema in self._compiled_file_system.GetFromFile(
              model.source_file).Get():
            if category == 'type':
              for type_json in ref_schema.get('types'):
                if type_json['id'] == name:
                  inline_docs[ref] = type_json
            elif category == 'event':
              for type_json in ref_schema.get('events'):
                if type_json['name'] == name:
                  inline_docs[ref] = type_json
      self._api_stack.remove(api_name)

    types = schema.get('types')
    if types:
      # Gather the types with inline_doc.
      for type_ in types:
        if type_.get('inline_doc'):
          inline_docs[type_['id']] = type_
          if not self._retain_inlined_types:
            for k in ('description', 'id', 'inline_doc'):
              type_.pop(k, None)
        elif internal_api:
          inline_docs[type_['id']] = type_
          # For internal apis that are not inline_doc we want to retain them
          # in the schema (i.e. same behaviour as remain_inlined_types)
          types_without_inline_doc.append(type_)
        else:
          types_without_inline_doc.append(type_)
      if not self._retain_inlined_types:
        schema['types'] = types_without_inline_doc

    def apply_inline(node):
      if isinstance(node, list):
        for i in node:
          apply_inline(i)
      elif isinstance(node, Mapping):
        ref = node.get('$ref')
        if ref and ref in inline_docs:
          del node['$ref']
          for k, v in inline_docs[ref].iteritems():
            if k not in node:
              node[k] = v
        for k, v in node.iteritems():
          apply_inline(v)

    apply_inline(schema)


  def Process(self, path, file_data):
    '''Parses |file_data| using a method determined by checking the
    extension of the file at the given |path|. Then, trims 'nodoc' and if
    |self.retain_inlined_types| is given and False, removes inlineable types
    from the parsed schema data.
    '''
    def trim_and_inline(schema, is_idl=False):
      '''Modifies an API schema in place by removing nodes that shouldn't be
      documented and inlining schema types that are only referenced once.
      '''
      if self._RemoveNoDocs(schema):
        # A return of True signifies that the entire schema should not be
        # documented. Otherwise, only nodes that request 'nodoc' are removed.
        return None
      if is_idl:
        self._DetectInlineableTypes(schema)
      self._InlineDocs(schema)
      return schema

    if path.endswith('.idl'):
      idl = idl_schema.IDLSchema(
          idl_parser.IDLParser().ParseData(file_data))
      # Wrap the result in a list so that it behaves like JSON API data.
      return [trim_and_inline(idl.process()[0], is_idl=True)]

    try:
      schemas = json_parse.Parse(file_data)
    except:
      raise ValueError('Cannot parse "%s" as JSON:\n%s' %
                       (path, traceback.format_exc()))
    for schema in schemas:
      # Schemas could consist of one API schema (data for a specific API file)
      # or multiple (data from extension_api.json).
      trim_and_inline(schema)
    return schemas
