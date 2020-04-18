# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from collections import defaultdict
import hashlib
import json
import sys

from print_data import PrintData


# Original data arrives as a dictionary of:
# name = { configs:[], type:'', properties: {}}
# where properties is a sub dictionary of the same type.
# So:
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


def HashKeys(key_a, key_b):
  m = hashlib.md5()
  if key_a:
    m.update(str(key_a))
  if key_b:
    m.update(str(key_b))
  return m.hexdigest()


def FindBsInConfigOfA(configs, setb, keya):
  bhits = []
  for config in configs:
    a, b = config.split('_')
    if a == keya:
      bhits.append(b)
  return bhits

#
# TransformRawToIndexed
#
# Transforms data from the original form shown above, to a dictionary of
# dictionaries with condition A as the key to the outer dictionary, and
# the set of B conditions for that key
#
# {
#   'keyA1' : {
#     'keyBx keyBy ...' : [ name1, name2 ]
#     'keyBz' : [ name3 ],
#   }
# }
#
def CreateIndexedLookup(tree, seta, setb):
  # Build dictionary of dictionaries with a key of seta
  out = {}
  for keya in seta:
    out[keya] = defaultdict(list)

  for name in tree:
    configs = sorted(tree[name].configs)
    for keya in seta:
      bhits = FindBsInConfigOfA(configs, setb, keya)
      if bhits:
        out[keya][' '.join(bhits)].append(name)
  return out


#
# MergeNodesToConditions
#
# Takes a dataset containing DICT[targets] = configs and generates a list of
# tupples containing the list of targets sharing a set of conditions.
# (name1, name2,...), (conda1, conda2), (condb1, condb2...)
#
def MergeRawTree(original, seta, availa, availb):
  merge_A = defaultdict(list)

  merge_arch = {}
  merge_data = {}

  # Build table of DATA[OS][CPU_SET] = List of NODES
  data = CreateIndexedLookup(original, seta, availb)

  # For every item in the data table, generate key based on CPUs and NODES
  # matching a configuration, and build tables indexed by that key.
  for key_a in data:
    for arches, hits in data[key_a].iteritems():
      keyval_hash = HashKeys(arches, hits)
      merge_A[keyval_hash].append(key_a)
      merge_arch[keyval_hash] = arches.split(' ')
      merge_data[keyval_hash] = hits

  # Merge all subsets
  keys = merge_A.keys()
  for i, i_key in enumerate(keys):
    # For each key, get the PLATFORMS, CPUS, and NODES
    i_oses = merge_A[i_key]
    i_cpus = merge_arch[i_key]
    i_hits = merge_data[i_key]

    for j, j_key in enumerate(keys):
      # For each other non-matching key, get PLATFORMS, CPUS, and NODES
      if i == j:
        continue
      j_oses = merge_A[j_key]
      j_cpus = merge_arch[j_key]
      j_hits = merge_data[j_key]

      # If the set of supported CPUS matches
      if i_cpus == j_cpus:
        i_set = set(i_hits)
        j_set = set(j_hits)
        # If the nodes in J are a subset of nodes in I
        # then move those nodes from I to J and add the OS
        if j_set.issubset(i_set):
          for hit in j_hits:
            merge_data[i_key].remove(hit)
          for os in i_oses:
            if os not in j_oses:
              merge_A[j_key].append(os)

  out_list = []
  keys = merge_A.keys()
  for i in range(len(keys)):
    hash_cpu_data = keys[i]
    os_use = merge_A[hash_cpu_data]
    cpus = merge_arch[hash_cpu_data]
    names = merge_data[hash_cpu_data]

    node = (names, os_use, cpus)
    out_list.append(node)

  def SortByEffect(x, y):
    # Sort by bigest combition first (# platforms * # cpu architectures)
    xval = len(x[1]) * len(x[2])
    yval = len(y[1]) * len(y[2])
    return  yval - xval

  out_list = sorted(out_list, cmp=SortByEffect)
  return out_list
