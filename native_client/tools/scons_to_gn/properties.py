# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from collections import defaultdict


PROPERTY_REMAP = {
  'CFLAGS': 'cflags',
  'CCFLAGS': 'cflags_c',
  'CXXFLAGS': 'cflags_cc',
  'LDFLAGS': 'ldflags',
  'EXTRA_LIBS': 'deps'
}
COMPILER_FLAGS = ['cflags', 'cflags_c', 'cflags_cc']
LINKER_FLAGS = ['ldfalgs']
OTHER_KEYS = ['defines', 'include_dirs', 'sources']

LIST_TYPES = set(COMPILER_FLAGS + LINKER_FLAGS + OTHER_KEYS)

def ConvertIfSingle(key, values):
  if key in LIST_TYPES:
    return values
  return values[0]


def RemapCompilerProperties(propname, items):
  props = defaultdict(list)
  props[propname] = []
  for item in items:
    if not item:
      continue

    if len(item) > 2:
      if item[:2] == '-D':
        props['defines'].append(item[2:])
        continue
      if item[:2] == '-I':
        props['include_dirs'].append(item[2:])
        continue
    props[propname].append(item)
  return props


def ConvertSconsPropertyToSubTable(key, items):
  # Force all items to be lists
  if items == None:
    items = [None]
  if type(items) is not type([]):
    items = [items]

  # If the name is not in the remap, then keep it
  propname = PROPERTY_REMAP.get(key, key)
  print "KEY=%s, PROPNAME=%s" %(key, propname)

  # If this is a compiler flag, we will need to build a new table
  if propname in COMPILER_FLAGS:
    return RemapCompilerProperties(propname, items)

  if propname == 'deps':
    items = [i for i in items if i not in ['pthread']]

  return { propname: items }


def ParsePropertyTable(table):
  props = defaultdict(list)

  # Take a table of KEY=VALUE pairs
  for key, value in table.iteritems():
    # Convert into a new table with GN style names.  Since a scons properties
    # like CFLAGS can convert into defines, included_dirs, etc...
    sub_table = ConvertSconsPropertyToSubTable(key, value)
    if not sub_table:
      continue
    for k,v in sub_table.iteritems():
      # Now we have a single property name and list of values.
      props[k].extend(v)

  return props

