# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Abstract ObjectFactory class used for injection of external dependencies."""

from __future__ import print_function

import functools


class ObjectFactoryIllegalOperation(Exception):
  """Raised when attemping an illegal ObjectFactory operation."""

_NO_SINGLETON_INSTANCE = object()

class ObjectFactory(object):
  """Abstract object factory, used for injection of external dependencies.

  A call to Setup(...) is necessary before a call to GetInstance().
  """

  _object_name = ''
  _is_setup = False
  _setup_type = None
  _setup_instance = None
  _types = {}

  def __init__(self, object_name, setup_types, allowed_transitions=None):
    """ObjectFactory constructor.

    Args:
      object_name: Human readable name for the type of object that this factory
                   generates.
      setup_types: A (set up type name -> generator function) dictionary, which
                   teaches ObjectFactory how to construct instances after setup
                   has been called. For set up types where a singleton instance
                   is specified at setup(...) time, generator function should be
                   None.
      allowed_transitions: Optional function, where
                           allowed_transitions(from_type, to_type) specifies
                           whether transition from |from_type| to |to_type| is
                           allowed.

                           If unspecified, no transitions are allowed.
    """

    self._object_name = object_name
    self._types = setup_types
    self._allowed_transitions = allowed_transitions

  def Setup(self, setup_type, singleton_instance=_NO_SINGLETON_INSTANCE):
    # Prevent set up to unknown types.
    if setup_type not in self._types:
      raise ObjectFactoryIllegalOperation(
          'Unknown %s setup_type %s' % (self._object_name, setup_type))

    # Prevent illegal setup transitions.
    if self._is_setup:
      if self._allowed_transitions:
        if not self._allowed_transitions(self._setup_type, setup_type):
          raise ObjectFactoryIllegalOperation(
              'Illegal set up transition from %s to %s.' % (self._setup_type,
                                                            setup_type))
      else:
        raise ObjectFactoryIllegalOperation(
            '%s already set up.' % self._object_name)

    # Allow singleton_instance if and only if factory method for this type is
    # None.
    instance_supplied = (singleton_instance != _NO_SINGLETON_INSTANCE)
    factory_is_none = (self._types[setup_type] is None)
    if instance_supplied != factory_is_none:
      raise ObjectFactoryIllegalOperation(
          'singleton_instance should be supplied if and only if setup_type has '
          'a factory that is None.')

    self._setup_type = setup_type
    self._setup_instance = singleton_instance
    self._is_setup = True

  @property
  def is_setup(self):
    """Returns True iff a call to get_instance is expected to succeed."""
    return self._is_setup

  @property
  def setup_type(self):
    """Returns the setup_type."""
    return self._setup_type

  def GetInstance(self):
    """Returns an object instance iff setup has been called.

    Raises:
      ObjectFactoryIllegalOperation: if setup has not yet been called.
    """
    if not self.is_setup:
      raise ObjectFactoryIllegalOperation(
          '%s is not set up.' % self._object_name)
    if self._setup_instance != _NO_SINGLETON_INSTANCE:
      return self._setup_instance
    return self._types[self.setup_type]()

  def _clear_setup(self):
    """Clear setup, for testing purposes only."""
    self._setup_type = None
    self._is_setup = False
    self._setup_instance = _NO_SINGLETON_INSTANCE


def CachedFunctionCall(function):
  """Wraps a parameterless |function| in a cache."""
  cached_value = []

  @functools.wraps(function)
  def wrapper():
    if not cached_value:
      cached_value.append(function())
    return cached_value[0]

  return wrapper
