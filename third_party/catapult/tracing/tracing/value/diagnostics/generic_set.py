# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json

from tracing.value.diagnostics import diagnostic


# TODO(#4143): Make this comment a docstring.
# A GenericSet diagnostic can contain any Plain-Ol'-Data objects that can be
# serialized using json.dumps(): None, boolean, number, string, list, dict.
# Dicts, lists, and booleans are deduplicated by their JSON representation.
# Dicts and lists are not hashable.
# (1 == True) and (0 == False) in Python, but not in JSON.
class GenericSet(diagnostic.Diagnostic):
  __slots__ = '_values', '_comparable_set'

  def __init__(self, values):
    super(GenericSet, self).__init__()

    # Use a list because Python sets cannot store dicts or lists because they
    # are not hashable.
    self._values = list(values)

    # Cache a set to facilitate comparing and merging GenericSets.
    # Dicts, lists, and booleans are serialized; other types are not.
    self._comparable_set = None

  def __iter__(self):
    for value in self._values:
      yield value

  def __len__(self):
    return len(self._values)

  def __eq__(self, other):
    return self._GetComparableSet() == other._GetComparableSet()

  def _GetComparableSet(self):
    if self._comparable_set is None:
      self._comparable_set = set()
      for value in self:
        if isinstance(value, (dict, list, bool)):
          self._comparable_set.add(json.dumps(value, sort_keys=True))
        else:
          self._comparable_set.add(value)
    return self._comparable_set

  def CanAddDiagnostic(self, other_diagnostic):
    return isinstance(other_diagnostic, GenericSet)

  def AddDiagnostic(self, other_diagnostic):
    comparable_set = self._GetComparableSet()
    for value in other_diagnostic:
      if isinstance(value, (dict, list, bool)):
        json_value = json.dumps(value, sort_keys=True)
        if json_value not in comparable_set:
          self._values.append(value)
          self._comparable_set.add(json_value)
      elif value not in comparable_set:
        self._values.append(value)
        self._comparable_set.add(value)

  def _AsDictInto(self, dct):
    dct['values'] = list(self)

  @staticmethod
  def FromDict(dct):
    return GenericSet(dct['values'])

  def GetOnlyElement(self):
    assert len(self) == 1
    return self._values[0]
