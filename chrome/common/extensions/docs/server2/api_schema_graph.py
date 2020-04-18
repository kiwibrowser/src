# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging

from api_models import GetNodeCategories
from collections import Iterable, Mapping

class LookupResult(object):
  '''Returned from APISchemaGraph.Lookup(), and relays whether or not
  some element was found and what annotation object was associated with it,
  if any.
  '''

  def __init__(self, found=None, annotation=None):
    assert found is not None, 'LookupResult was given None value for |found|.'
    self.found = found
    self.annotation = annotation

  def __eq__(self, other):
    return self.__dict__ == other.__dict__

  def __ne__(self, other):
    return not (self == other)

  def __repr__(self):
    return '%s%s' % (type(self).__name__, repr(self.__dict__))

  def __str__(self):
    return repr(self)


class APINodeCursor(object):
  '''An abstract representation of a node in an APISchemaGraph.
  The current position in the graph is represented by a path into the
  underlying dictionary. So if the APISchemaGraph is:

    {
      'tabs': {
        'types': {
          'Tab': {
            'properties': {
              'url': {
                ...
              }
            }
          }
        }
      }
    }

  then the 'url' property would be represented by:

    ['tabs', 'types', 'Tab', 'properties', 'url']
  '''

  def __init__(self, availability_finder, namespace_name):
    self._lookup_path = []
    self._node_availabilities = availability_finder.GetAPINodeAvailability(
        namespace_name)
    self._namespace_name = namespace_name
    self._ignored_categories = []

  def _AssertIsValidCategory(self, category):
    assert category in GetNodeCategories(), \
        '%s is not a valid category. Full path: %s' % (category, str(self))

  def _GetParentPath(self):
    '''Returns the path pointing to this node's parent.
    '''
    assert len(self._lookup_path) > 1, \
        'Tried to look up parent for the top-level node.'

    # lookup_path[-1] is the name of the current node. If this lookup_path
    # describes a regular node, then lookup_path[-2] will be a node category.
    # Otherwise, it's an event callback or a function parameter.
    if self._lookup_path[-2] not in GetNodeCategories():
      if self._lookup_path[-1] == 'callback':
        # This is an event callback, so lookup_path[-2] is the event
        # node name, thus lookup_path[-3] must be 'events'.
        assert self._lookup_path[-3] == 'events' , \
            'Invalid lookup path: %s' % (self._lookup_path)
        return self._lookup_path[:-1]
      # This is a function parameter.
      assert self._lookup_path[-2] == 'parameters'
      return self._lookup_path[:-2]
    # This is a regular node, so lookup_path[-2] should
    # be a node category.
    self._AssertIsValidCategory(self._lookup_path[-2])
    return self._lookup_path[:-2]

  def _LookupNodeAvailability(self, lookup_path):
    '''Returns the ChannelInfo object for this node.
    '''
    return self._node_availabilities.Lookup(self._namespace_name,
                                            *lookup_path).annotation

  def _CheckNamespacePrefix(self, lookup_path):
    '''API schemas may prepend the namespace name to top-level types
    (e.g. declarativeWebRequest > types > declarativeWebRequest.IgnoreRules),
    but just the base name (here, 'IgnoreRules') will be in the |lookup_path|.
    Try creating an alternate |lookup_path| by adding the namespace name.
    '''
    # lookup_path[0] is always the node category (e.g. types, functions, etc.).
    # Thus, lookup_path[1] is always the top-level node name.
    self._AssertIsValidCategory(lookup_path[0])
    base_name = lookup_path[1]
    lookup_path[1] = '%s.%s' % (self._namespace_name, base_name)
    try:
      node_availability = self._LookupNodeAvailability(lookup_path)
      if node_availability is not None:
        return node_availability
    finally:
      # Restore lookup_path.
      lookup_path[1] = base_name
    return None

  def _CheckEventCallback(self, lookup_path):
    '''Within API schemas, an event has a list of 'properties' that the event's
    callback expects. The callback itself is not explicitly represented in the
    schema. However, when creating an event node in JSCView, a callback node
    is generated and acts as the parent for the event's properties.
    Modify |lookup_path| to check the original schema format.
    '''
    if 'events' in lookup_path:
      assert 'callback' in lookup_path, self
      callback_index = lookup_path.index('callback')
      try:
        lookup_path.pop(callback_index)
        node_availability = self._LookupNodeAvailability(lookup_path)
      finally:
        lookup_path.insert(callback_index, 'callback')
      return node_availability
    return None

  def _LookupAvailability(self, lookup_path):
    '''Runs all the lookup checks on |lookup_path| and
    returns the node availability if found, None otherwise.
    '''
    for lookup in (self._LookupNodeAvailability,
                   self._CheckEventCallback,
                   self._CheckNamespacePrefix):
      node_availability = lookup(lookup_path)
      if node_availability is not None:
        return node_availability
    return None

  def _GetCategory(self):
    '''Returns the category this node belongs to.
    '''
    if self._lookup_path[-2] in GetNodeCategories():
      return self._lookup_path[-2]
    # If lookup_path[-2] is not a valid category and lookup_path[-1] is
    # 'callback', then we know we have an event callback.
    if self._lookup_path[-1] == 'callback':
      return 'events'
    if self._lookup_path[-2] == 'parameters':
      # Function parameters are modelled as properties.
      return 'properties'
    if (self._lookup_path[-1].endswith('Type') and
        (self._lookup_path[-1][:-len('Type')] == self._lookup_path[-2] or
         self._lookup_path[-1][:-len('ReturnType')] == self._lookup_path[-2])):
      # Array elements and function return objects have 'Type' and 'ReturnType'
      # appended to their names, respectively, in model.py. This results in
      # lookup paths like
      # 'events > types > Rule > properties > tags > tagsType'.
      # These nodes are treated as properties.
      return 'properties'
    if self._lookup_path[0] == 'events':
      # HACK(ahernandez.miralles): This catches a few edge cases,
      # such as 'webviewTag > events > consolemessage > level'.
      return 'properties'
    raise AssertionError('Could not classify node %s' % self)

  def GetDeprecated(self):
    '''Returns when this node became deprecated, or None if it
    is not deprecated.
    '''
    deprecated_path = self._lookup_path + ['deprecated']
    for lookup in (self._LookupNodeAvailability,
                   self._CheckNamespacePrefix):
      node_availability = lookup(deprecated_path)
      if node_availability is not None:
        return node_availability
    if 'callback' in self._lookup_path:
      return self._CheckEventCallback(deprecated_path)
    return None

  def GetAvailability(self):
    '''Returns availability information for this node.
    '''
    if self._GetCategory() in self._ignored_categories:
      return None
    node_availability = self._LookupAvailability(self._lookup_path)
    if node_availability is None:
      logging.warning('No availability found for: %s' % self)
      return None

    parent_node_availability = self._LookupAvailability(self._GetParentPath())
    # If the parent node availability couldn't be found, something
    # is very wrong.
    assert parent_node_availability is not None

    # Only render this node's availability if it differs from the parent
    # node's availability.
    if node_availability == parent_node_availability:
      return None
    return node_availability

  def Descend(self, *path, **kwargs):
    '''Moves down the APISchemaGraph, following |path|.
    |ignore| should be a tuple of category strings (e.g. ('types',))
    for which nodes should not have availability data generated.
    '''
    ignore = kwargs.get('ignore')
    class scope(object):
      def __enter__(self2):
        if ignore:
          self._ignored_categories.extend(ignore)
        if path:
          self._lookup_path.extend(path)

      def __exit__(self2, _, __, ___):
        if ignore:
          self._ignored_categories[:] = self._ignored_categories[:-len(ignore)]
        if path:
          self._lookup_path[:] = self._lookup_path[:-len(path)]
    return scope()

  def __str__(self):
    return repr(self)

  def __repr__(self):
    return '%s > %s' % (self._namespace_name, ' > '.join(self._lookup_path))


class _GraphNode(dict):
  '''Represents some element of an API schema, and allows extra information
  about that element to be stored on the |_annotation| object.
  '''

  def __init__(self, *args, **kwargs):
    # Use **kwargs here since Python is picky with ordering of default args
    # and variadic args in the method signature. The only keyword arg we care
    # about here is 'annotation'. Intentionally don't pass |**kwargs| into the
    # superclass' __init__().
    dict.__init__(self, *args)
    self._annotation = kwargs.get('annotation')

  def __eq__(self, other):
    # _GraphNode inherits __eq__() from dict, which will not take annotation
    # objects into account when comparing.
    return dict.__eq__(self, other)

  def __ne__(self, other):
    return not (self == other)

  def GetAnnotation(self):
    return self._annotation

  def SetAnnotation(self, annotation):
    self._annotation = annotation


def _NameForNode(node):
  '''Creates a unique id for an object in an API schema, depending on
  what type of attribute the object is a member of.
  '''
  if 'namespace' in node: return node['namespace']
  if 'name' in node: return node['name']
  if 'id' in node: return node['id']
  if 'type' in node: return node['type']
  if '$ref' in node: return node['$ref']
  assert False, 'Problems with naming node: %s' % json.dumps(node, indent=3)


def _IsObjectList(value):
  '''Determines whether or not |value| is a list made up entirely of
  dict-like objects.
  '''
  return (isinstance(value, Iterable) and
          all(isinstance(node, Mapping) for node in value))


def _CreateGraph(root):
  '''Recursively moves through an API schema, replacing lists of objects
  and non-object values with objects.
  '''
  schema_graph = _GraphNode()
  if _IsObjectList(root):
    for node in root:
      name = _NameForNode(node)
      assert name not in schema_graph, 'Duplicate name in API schema graph.'
      schema_graph[name] = _GraphNode((key, _CreateGraph(value)) for
                                      key, value in node.iteritems())

  elif isinstance(root, Mapping):
    for name, node in root.iteritems():
      if not isinstance(node, Mapping):
        schema_graph[name] = _GraphNode()
      else:
        schema_graph[name] = _GraphNode((key, _CreateGraph(value)) for
                                        key, value in node.iteritems())
  return schema_graph


def _Subtract(minuend, subtrahend):
  ''' A Set Difference adaptation for graphs. Returns a |difference|,
  which contains key-value pairs found in |minuend| but not in
  |subtrahend|.
  '''
  difference = _GraphNode()
  for key in minuend:
    if key not in subtrahend:
      # Record all of this key's children as being part of the difference.
      difference[key] = _Subtract(minuend[key], {})
    else:
      # Note that |minuend| and |subtrahend| are assumed to be graphs, and
      # therefore should have no lists present, only keys and nodes.
      rest = _Subtract(minuend[key], subtrahend[key])
      if rest:
        # Record a difference if children of this key differed at some point.
        difference[key] = rest
  return difference


class APISchemaGraph(object):
  '''Provides an interface for interacting with an API schema graph, a
  nested dict structure that allows for simpler lookups of schema data.
  '''

  def __init__(self, api_schema=None, _graph=None):
    self._graph = _graph if _graph is not None else _CreateGraph(api_schema)

  def __eq__(self, other):
    return self._graph == other._graph

  def __ne__(self, other):
    return not (self == other)

  def Subtract(self, other):
    '''Returns an APISchemaGraph instance representing keys that are in
    this graph but not in |other|.
    '''
    return APISchemaGraph(_graph=_Subtract(self._graph, other._graph))

  def Update(self, other, annotator):
    '''Modifies this graph by adding keys from |other| that are not
    already present in this graph.
    '''
    def update(base, addend):
      '''A Set Union adaptation for graphs. Returns a graph which contains
      the key-value pairs from |base| combined with any key-value pairs
      from |addend| that are not present in |base|.
      '''
      for key in addend:
        if key not in base:
          # Add this key and the rest of its children.
          base[key] = update(_GraphNode(annotation=annotator(key)), addend[key])
        else:
          # The key is already in |base|, but check its children.
           update(base[key], addend[key])
      return base

    update(self._graph, other._graph)

  def Lookup(self, *path):
    '''Given a list of path components, |path|, checks if the
    APISchemaGraph instance contains |path|.
    '''
    node = self._graph
    for path_piece in path:
      node = node.get(path_piece)
      if node is None:
        return LookupResult(found=False, annotation=None)
    return LookupResult(found=True, annotation=node._annotation)

  def IsEmpty(self):
    '''Checks for an empty schema graph.
    '''
    return not self._graph
