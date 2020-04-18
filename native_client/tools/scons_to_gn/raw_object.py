# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from collections import defaultdict


# {
#   'libnacl': {
#      'configs' : [ X1_Y1, X2_Y1, ...],
#      'type': 'source_set' | 'NaClSharedLibrary' | ....,
#      'properties' : {
#         'sources' : {
#             'source_file1.c' : {
#               'configs' : [ X1_Y1, X2_Y1, ...],
#             },
#             'source_file1.c' : {
#               'configs' : [ X1_Y1, X2_Y1, ...],
#             },
#
#  RawTree(TOP)
#     RawObject(libnacl)
#       RawTree(sources)
#           RawObject(source_file.c)

class RawTree(object):
  def __init__(self):
    self.children = defaultdict(RawObject)

  def __iter__(self):
    return self.children.__iter__()

  def __len__(self):
    return len(self.children)

  def __getitem__(self, key):
    return self.children[key]

  def __setitem__(self, key, value):
    self.children[key] = value


class RawObject(object):
  def __init__(self):
    self.obj_type = None
    self.configs = []
    self.properties = defaultdict(RawTree)

  def __getitem__(self, key):
    return self.properties[key]

  def __setitem__(self, key, value):
    self.properties[key] = value

  def SetType(self, obj_type):
    if self.obj_type == None:
      self.obj_type = obj_type
    if self.obj_type != obj_type:
      raise RuntimeError('Mismatch type for %s, expected %s.' %
                         (name, obj_type))

  def AddCondition(self, cond):
    self.configs.append(cond)

  def Configs(self):
    return self.configs

  def Properties(self):
    return self.properties

  def Type(self):
    return self.obj_type
