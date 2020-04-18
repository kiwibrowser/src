# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from collections import defaultdict
import hashlib
import json
import os
import sys

from conditions import TrustedConditions, UntrustedConditions
from properties import ConvertIfSingle, ParsePropertyTable
from nodes import ConditionNode, ObjectNode, PropertyNode, ValueNode, TopNode
from nodes import OrganizeProperties
from merge_data import MergeRawTree
from raw_object import RawObject, RawTree
from scons_environment import Environment

"""The workhorse of scons to gn.

the Environment holds the state of a scons 'env' objects, while the object
tracker stores the matrix of conditions and values.

The object tracket is responsible for building the Node tree.
"""


NOTICE = """# Copyright (c) 2014 The Native ClientAuthors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""

class ObjectTracker(object):
  def __init__(self, name, cond_obj):
    self.object_tree = RawTree()
    self.top = TopNode(name)
    self.cond_obj = cond_obj
    self.installs = []
    self.BuildObjectMap(name)
    self.BuildTree()

  def ActiveCondition(self):
    return self.cond_obj.ActiveCondition()

  def Dump(self, fileobj):
    fileobj.write(NOTICE)
    self.cond_obj.WriteImports(fileobj)
    self.top.Dump(fileobj, 0)
    for ins in self.installs:
      print "Install " + ins

  def ExecCondition(self, name):
    env = Environment(self, self.cond_obj)
    global_map = {
      'Action': Action,
      'Import': Import,
      'COMMAND_LINE_TARGETS' : [],
      'env' : env,
      'os' : os
    }
    local_map = {
      'env' : env,
      'os' : os
    }
    try:
      execfile(name, global_map, local_map)
      env.Flush()
    except BaseException, e:
      pass

  def AddHeader(self, node):
    self.installs.append("Header: " + node)

  def AddLibrary(self, node):
    self.installs.append("Library: " + node)

  def AddObject(self, name, obj_type, add_props={}, del_props={}):
    obj = self.object_tree[name]
    obj.SetType(obj_type)

    for prop_name, prop_values in add_props.iteritems():
      if not prop_values:
        continue
      prop_tree = obj[prop_name]
      for value in prop_values:
        prop_tree[value].AddCondition(self.ActiveCondition())
    obj.AddCondition(self.ActiveCondition());

  def BuildObjectMap(self, name):
    for cond in self.cond_obj.All():
      # Execute with an empty dict for this condition
      self.cond_obj.SetActiveCondition(cond)
      self.ExecCondition(name)

  def BuildTree(self):
    # First we parse the TOP objects to get top conditions
    data = self.object_tree
    set_a = self.cond_obj.SetA()
    set_b = self.cond_obj.SetB()
    avail_a = set_a
    avail_b = set_b
    merged_data = MergeRawTree(data, set_a, set_b, avail_b)

    for names, use_a, use_b in merged_data:
      # Create a condition node for this sub-group
      cond = ConditionNode(use_a, avail_a, use_b, avail_b)
      self.top.AddChild(cond)

      # For each top child in this condition group create the object
      for name in names:
        raw_object = data[name]
        node = ObjectNode(name, raw_object.Type())
        cond.AddChild(node)

        # Now add properties to that object
        for prop_name, prop_tree in raw_object.Properties().iteritems():
          merged_sub = MergeRawTree(prop_tree, use_a, use_b, use_b)
          for names, prop_a, prop_b in merged_sub:
            # Create a condition for this property set
            prop_cond = ConditionNode(prop_a, use_a, prop_b, use_b)
            node.AddChild(prop_cond)

            # Add property set to condition
            prop_node = PropertyNode(prop_name)
            prop_cond.AddChild(prop_node)
            for value in names:
              prop_node.AddChild(ValueNode(value))
    self.top.Examine(OrganizeProperties())

def Action(*args):
  return []

def Import(name):
  if name != 'env':
    print 'Warning: Tried to IMPORT: ' + name

def ParseSource(name):
  tracker = ObjectTracker(name);
