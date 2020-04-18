# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module classifies MemoryMap objects filtering their mmap entries.

Two filters are currently available:
  - 'mmap_file': 'foo.*\.so': matches any entry which mmap file is foo*.so.
  - 'mmap_prot': 'rw.-': matches any entry which prot. flags is rw*-.
"""

import re

from memory_inspector.classification import results
from memory_inspector.classification import rules
from memory_inspector.core import exceptions
from memory_inspector.core import memory_map


_RESULT_KEYS = ['RSS', 'Private Dirty', 'Private Clean', 'Shared Dirty',
                'Shared Clean']


def LoadRules(content):
  """Loads and parses a mmap rule tree from a content (string).

  Returns:
    An instance of |rules.Rule|, which nodes are |_MmapRule| instances.
  """
  return rules.Load(content, _MmapRule)


def Classify(mmap, rule_tree):
  """Create aggregated results of memory maps using the provided rules.

  Args:
    mmap: the memory map dump being processed (a |memory_map.Map| instance).
    rule_tree: the user-defined rules that define the filtering categories.

  Returns:
    An instance of |AggreatedResults|.
  """
  assert(isinstance(mmap, memory_map.Map))
  assert(isinstance(rule_tree, rules.Rule))

  res = results.AggreatedResults(rule_tree, _RESULT_KEYS)
  for map_entry in mmap.entries:
    values = [0, map_entry.priv_dirty_bytes, map_entry.priv_clean_bytes,
              map_entry.shared_dirty_bytes, map_entry.shared_clean_bytes]
    values[0] = values[1] + values[2] + values[3] + values[4]
    res.AddToMatchingNodes(map_entry, values)
  return res


class _MmapRule(rules.Rule):
  def __init__(self, name, filters):
    super(_MmapRule, self).__init__(name)
    try:
      self._file_re = (
          re.compile(filters['mmap_file']) if 'mmap_file' in filters else None)
      self._prot_re = (
          re.compile(filters['mmap_prot']) if 'mmap_prot' in filters else None)
    except re.error, descr:
      raise exceptions.MemoryInspectorException(
        'Regex parse error "%s" : %s' % (filters, descr))

  def Match(self, map_entry):
    if self._file_re and not self._file_re.search(map_entry.mapped_file):
      return False
    if self._prot_re and not self._prot_re.search(map_entry.prot_flags):
      return False
    return True