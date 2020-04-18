# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''
Provides a Manifest Feature abstraction, similar to but more strict than the
Feature schema (see feature_utility.py).

Each Manifest Feature has a 'level' in addition to the keys defined in a
Feature. 'level' can be 'required', 'only_one', 'recommended', or 'optional',
indicating how an app or extension should define a manifest property. If 'level'
is missing, 'optional' is assumed.
'''

def ConvertDottedKeysToNested(features):
  '''Some Manifest Features are subordinate to others, such as app.background to
  app. Subordinate Features can be moved inside the parent Feature under the key
  'children'.

  Modifies |features|, a Manifest Features dictionary, by moving subordinate
  Features with names of the form 'parent.child' into the 'parent' Feature.
  Child features are renamed to the 'child' section of their previous name.

  Applied recursively so that children can be nested arbitrarily.
  '''
  def add_child(features, parent, child_name, value):
    value['name'] = child_name
    if not 'children' in features[parent]:
      features[parent]['children'] = {}
    features[parent]['children'][child_name] = value

  def insert_children(features):
    for name in features.keys():
      if '.' in name:
        value = features.pop(name)
        parent, child_name = name.split('.', 1)
        add_child(features, parent, child_name, value)

    for value in features.values():
      if 'children' in value:
        insert_children(value['children'])

  insert_children(features)
  return features
