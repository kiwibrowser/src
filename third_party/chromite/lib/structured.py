# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A convenience class for objects which can be converted to JSON."""

from __future__ import print_function

_BUILTINS = {str, int, float, bool, type(None)}


class Structured(object):
  """An object with a whitelisted set of public properties (.VISIBLE_KEYS)"""

  def _Keys(self):
    seen = set()
    for cls in type(self).mro():
      for k in getattr(cls, 'VISIBLE_KEYS', ()):
        if k not in seen:
          yield k
          seen.add(k)

  def ToDict(self):
    return ToStructure(self)


def ToStructure(value):
  """Makes an object JSON-encodable.

  Args:
    value: An object which can be converted to a JSON-encodable object.

  Returns:
    An object which is legal to pass into json.dumps(obj), namely
    type JSONEncodable = (
        str | int | float | NoneType | dict<str, JSONEncodable> |
        list<JSONEncodable>

  Raises:
    StackOverflow if the object has circular references (parent -> child ->
    parent).
  """
  if type(value) in _BUILTINS:
    return value

  elif hasattr(value, '_Keys'):
    ret = {}
    for k in value._Keys():  # pylint: disable=protected-access
      v = ToStructure(getattr(value, k, None))
      if v is not None:
        ret[k] = v
    return ret

  elif isinstance(value, dict):
    ret = {}
    for k, v in value.iteritems():
      v = ToStructure(v)
      if v is not None:
        ret[k] = v
    return ret

  else:
    try:
      iterator = iter(value)
    except TypeError:
      return None

    ret = []
    for element in iterator:
      v = ToStructure(element)
      if v is not None:
        ret.append(v)

    return ret
