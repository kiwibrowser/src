# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import math
import numbers

from tracing.value.diagnostics import diagnostic


class Breakdown(diagnostic.Diagnostic):
  __slots__ = '_values', '_color_scheme'

  def __init__(self):
    super(Breakdown, self).__init__()
    self._values = {}
    self._color_scheme = None

  @property
  def color_scheme(self):
    return self._color_scheme

  @staticmethod
  def FromDict(d):
    result = Breakdown()
    result._color_scheme = d.get('colorScheme')
    for name, value in d['values'].iteritems():
      if value in ['NaN', 'Infinity', '-Infinity']:
        value = float(value)
      result.Set(name, value)
    return result

  def _AsDictInto(self, d):
    d['values'] = {}
    for name, value in self:
      # JSON serializes NaN and the infinities as 'null', preventing
      # distinguishing between them. Override that behavior by serializing them
      # as their Javascript string names, not their python string names since
      # the reference implementation is in Javascript.
      if math.isnan(value):
        value = 'NaN'
      elif math.isinf(value):
        if value > 0:
          value = 'Infinity'
        else:
          value = '-Infinity'
      d['values'][name] = value
    if self._color_scheme:
      d['colorScheme'] = self._color_scheme

  def Set(self, name, value):
    assert isinstance(name, basestring), (
        'Expected basestring, found %s: "%r"' % (type(name).__name__, name))
    assert isinstance(value, numbers.Number), (
        'Expected number, found %s: "%r"', (type(value).__name__, value))
    self._values[name] = value

  def Get(self, name):
    return self._values.get(name, 0)

  def __iter__(self):
    for name, value in self._values.iteritems():
      yield name, value
