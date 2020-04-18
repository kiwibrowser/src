# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import uuid

from py_utils import slots_metaclass

from tracing.value.diagnostics import all_diagnostics


class Diagnostic(object):
  __slots__ = '_guid',

  # Ensure that new subclasses remember to specify __slots__ in order to prevent
  # regressing memory consumption:
  __metaclass__ = slots_metaclass.SlotsMetaclass

  def __init__(self):
    self._guid = None

  def __ne__(self, other):
    return not self == other

  @property
  def guid(self):
    if self._guid is None:
      self._guid = str(uuid.uuid4())
    return self._guid

  @guid.setter
  def guid(self, g):
    assert self._guid is None
    self._guid = g

  @property
  def has_guid(self):
    return self._guid is not None

  def AsDictOrReference(self):
    if self._guid:
      return self._guid
    return self.AsDict()

  def AsDict(self):
    dct = {'type': self.__class__.__name__}
    if self._guid:
      dct['guid'] = self._guid
    self._AsDictInto(dct)
    return dct

  def _AsDictInto(self, unused_dct):
    raise NotImplementedError

  @staticmethod
  def FromDict(dct):
    cls = all_diagnostics.GetDiagnosticClassForName(dct['type'])
    if not cls:
      raise ValueError('Unrecognized diagnostic type: ' + dct['type'])
    diagnostic = cls.FromDict(dct)
    if 'guid' in dct:
      diagnostic.guid = dct['guid']
    return diagnostic

  def ResetGuid(self):
    self._guid = str(uuid.uuid4())

  def Inline(self):
    """Inlines a shared diagnostic.

    Any diagnostic that has a guid will be serialized as a reference, because it
    is assumed that diagnostics with guids are shared. This method removes the
    guid so that the diagnostic will be serialized by value.

    Inling is used for example in the dashboard, where certain types of shared
    diagnostics that vary on a per-upload basis are inlined for efficiency
    reasons.
    """
    self._guid = None

  def CanAddDiagnostic(self, unused_other_diagnostic):
    return False

  def AddDiagnostic(self, unused_other_diagnostic):
    raise Exception('Abstract virtual method: subclasses must override '
                    'this method if they override canAddDiagnostic')
