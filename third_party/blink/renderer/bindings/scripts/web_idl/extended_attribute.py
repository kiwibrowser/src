# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from .utilities import assert_no_extra_args


# https://heycam.github.io/webidl/#idl-extended-attributes
# To work with [Exposed], [Constructor], [CustomConstrucotr], and [NamedConstructor] easily, we define some classes
# for them in this file.

class ExtendedAttributeList(object):

    def __init__(self, **kwargs):
        self._extended_attributes = kwargs.pop('extended_attributes', {})
        self._exposures = tuple(kwargs.pop('exposures', []))
        constructors = kwargs.pop('constructors', [])
        self._constructors = tuple([ctor for ctor in constructors if type(ctor) == Constructor])
        self._named_constructors = tuple([ctor for ctor in constructors if type(ctor) == NamedConstructor])

        if self.exposures:
            self._extended_attributes['Exposed'] = self.exposures
        if self.named_constructors:
            self._extended_attributes['NamedConstructor'] = self.named_constructors
        if any(ctor.is_custom for ctor in self.constructors):
            self._extended_attributes['CustomConstructor'] = [ctor for ctor in self.constructors if ctor.is_custom]
        if any(not ctor.is_custom for ctor in self.constructors):
            self._extended_attributes['CustomConstructor'] = [ctor for ctor in self.constructors if not ctor.is_custom]

    def get(self, key):
        return self.extended_attributes.get(key)

    def has(self, key):
        return key in self.extended_attributes

    @property
    def extended_attributes(self):
        """
        [Exposed], [Constructor], [CustomConstrucotr], and [NamedConstructor] can be taken with
        other property methods, but the returned value of this method also includes them.
        """
        return self._extended_attributes

    @property
    def constructors(self):
        return self._constructors

    @property
    def named_constructors(self):
        return self._named_constructors

    @property
    def exposures(self):
        return self._exposures


# https://heycam.github.io/webidl/#Constructor
class Constructor(object):

    def __init__(self, **kwargs):
        self._arguments = kwargs.pop('arguments', [])
        self._is_custom = kwargs.pop('is_custom', False)
        assert_no_extra_args(kwargs)

    @property
    def arguments(self):
        return self._arguments

    @property
    def is_custom(self):
        return self._is_custom


# https://heycam.github.io/webidl/#NamedConstructor
class NamedConstructor(object):

    def __init__(self, **kwargs):
        self._identifier = kwargs.pop('identifier', None)
        self._arguments = kwargs.pop('arguments', [])
        assert_no_extra_args(kwargs)

    @property
    def identifier(self):
        return self._identifier

    @property
    def arguments(self):
        return self._arguments


# https://heycam.github.io/webidl/#Exposed
class Exposure(object):
    """Exposure holds an exposed target, and can hold a runtime enabled condition.
    "[Exposed=global_interface]" is represented as Exposure(global_interface), and
    "[Exposed(global_interface runtime_enabled_feature)] is represented as Exposure(global_interface, runtime_enabled_feature).
    """
    def __init__(self, **kwargs):
        self._global_interface = kwargs.pop('global_interface')
        self._runtime_enabled_feature = kwargs.pop('runtime_enabled_feature', None)
        assert_no_extra_args(kwargs)

    @property
    def global_interface(self):
        return self._global_interface

    @property
    def runtime_enabled_feature(self):
        return self._runtime_enabled_feature
