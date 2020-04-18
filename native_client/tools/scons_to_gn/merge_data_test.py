# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from collections import defaultdict
import pprint
import sys

from merge_data import CreateIndexedLookup, MergeRawTree
from print_data import PrintData


CONDA = ['A1', 'A2', 'A3']
CONDB = ['B1', 'B2', 'B3']

CONFIG_ALL = [a + '_' + b for a in CONDA for b in CONDB]

CONFIG_A1 = [ 'A1_B1',  'A1_B2', 'A1_B3' ]
CONFIG_B1 = [ 'A1_B1',  'A2_B1', 'A3_B1' ]

CONFIG_SET1 = [ 'A1_B1',  'A1_B2', 'A2_B2' ]
CONFIG_SET2 = [ 'A1_B1']
CONFIG_SET3 = [ 'A1_B1', 'A2_B3']


class TestNode(object):
  def __init__(self, configs):
    self.configs = configs

TEST_ALL = {
  'target_1': TestNode(CONFIG_ALL),
  'target_2': TestNode(CONFIG_ALL),
  'target_3': TestNode(CONFIG_ALL),
}

TEST_NOT3 = {
  'target_1': TestNode(CONFIG_ALL),
  'target_2': TestNode(CONFIG_ALL),
  'target_3': TestNode(CONFIG_SET2),
}

TEST_A1_B1 = {
  'target_1': TestNode(CONFIG_A1),
  'target_2': TestNode(CONFIG_B1),
  'target_3': TestNode(CONFIG_ALL),
}

def SameAsOriginal(table, hits):
  for target in hits:
    table_set = set(table[target].configs)
    trans_set = set(hits[target])
    if table_set != trans_set:
      table_set_items = ' '.join(table_set)
      trans_set_items = ' '.join(trans_set)
      print 'FAIL %s: %s vs %s' % (target, table_set_items, trans_set_items)
      return False
  return True


def CompareOriginalToTransformed(table, transformed, name):
  trans_hits = defaultdict(list)
  for keya, hit_table in transformed.iteritems():
    for hitsb, values in hit_table.iteritems():
      for value in values:
        trans_hits[value].extend([keya + '_' + b for b in hitsb.split(' ')])

  if not SameAsOriginal(table, trans_hits):
    PrintData(table)
    PrintData(transformed)
    return 1
  return 0


def CompareOriginalToMerged(table, merged, name):
  trans_hits = defaultdict(list)
  for targets, seta, setb in merged:
    for target in targets:
      trans_hits[target].extend([a+'_'+b for a in seta for b in setb])

  if not SameAsOriginal(table, trans_hits):
    PrintData(table)
    PrintData(merged)
    return 1
  return 0


def TestTransform(table, name, verbose=False):
  transformed  = CreateIndexedLookup(table, CONDA, CONDB)
  cnt  = CompareOriginalToTransformed(table, transformed, name)
  merged = MergeRawTree(table, CONDA, CONDA, CONDB)
  cnt += CompareOriginalToMerged(table, merged, name)
  if verbose:
    PrintData(merged)
  if not cnt:
    print 'PASS ' + name
    return 0
  print 'FAILED ' + name
  return 1


if __name__ == '__main__':
  retval = 0
  retval += TestTransform(TEST_ALL, 'All')
  retval += TestTransform(TEST_NOT3, 'Not3')
  retval += TestTransform(TEST_A1_B1, 'A1B1')
  sys.exit(retval)
