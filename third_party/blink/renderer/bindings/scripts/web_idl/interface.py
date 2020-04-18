# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from .extended_attribute import ExtendedAttributeList
from .utilities import assert_no_extra_args


# https://heycam.github.io/webidl/#idl-interfaces
class Interface(object):

    def __init__(self, **kwargs):
        self._identifier = kwargs.pop('identifier')
        self._attributes = tuple(kwargs.pop('attributes', []))
        self._operations = tuple(kwargs.pop('operations', []))
        self._constants = tuple(kwargs.pop('constants', []))
        self._iterable = kwargs.pop('iterable', None)
        self._maplike = kwargs.pop('maplike', None)
        self._setlike = kwargs.pop('setlike', None)
        # BUG(736332): Remove support of legacy serializer members.
        self._serializer = kwargs.pop('serializer', None)
        self._inherited_interface_name = kwargs.pop('inherited_interface_name', None)
        self._is_partial = kwargs.pop('is_partial', False)
        self._extended_attribute_list = kwargs.pop('extended_attribute_list', ExtendedAttributeList())
        assert_no_extra_args(kwargs)

        num_declaration = (1 if self.iterable else 0) + (1 if self.maplike else 0) + (1 if self.setlike else 0)
        if num_declaration > 1:
            raise ValueError('At most one of iterable<>, maplike<>, or setlike<> must be applied.')

    @property
    def identifier(self):
        return self._identifier

    @property
    def attributes(self):
        return self._attributes

    @property
    def operations(self):
        return self._operations

    @property
    def constants(self):
        return self._constants

    @property
    def iterable(self):
        return self._iterable

    @property
    def maplike(self):
        return self._maplike

    @property
    def setlike(self):
        return self._setlike

    @property
    def serializer(self):
        return self._serializer

    @property
    def inherited_interface_name(self):
        return self._inherited_interface_name

    @property
    def is_partial(self):
        return self._is_partial

    @property
    def constructors(self):
        return self.extended_attribute_list.constructors

    @property
    def named_constructors(self):
        return self.extended_attribute_list.named_constructors

    @property
    def exposures(self):
        return self.extended_attribute_list.exposures

    @property
    def extended_attribute_list(self):
        return self._extended_attribute_list


# https://heycam.github.io/webidl/#idl-iterable
class Iterable(object):

    def __init__(self, **kwargs):
        self._key_type = kwargs.pop('key_type', None)
        self._value_type = kwargs.pop('value_type')
        self._extended_attribute_list = kwargs.pop('extended_attribute_list', ExtendedAttributeList())
        assert_no_extra_args(kwargs)

    @property
    def key_type(self):
        return self._key_type

    @property
    def value_type(self):
        return self._value_type

    @property
    def extended_attribute_list(self):
        return self._extended_attribute_list


# https://heycam.github.io/webidl/#idl-maplike
class Maplike(object):

    def __init__(self, **kwargs):
        self._key_type = kwargs.pop('key_type')
        self._value_type = kwargs.pop('value_type')
        self._is_readonly = kwargs.pop('is_readonly', False)
        assert_no_extra_args(kwargs)

    @property
    def key_type(self):
        return self._key_type

    @property
    def value_type(self):
        return self._value_type

    @property
    def is_readonly(self):
        return self._is_readonly


# https://heycam.github.io/webidl/#idl-setlike
class Setlike(object):

    def __init__(self, **kwargs):
        self._value_type = kwargs.pop('value_type')
        self._is_readonly = kwargs.pop('is_readonly', False)
        assert_no_extra_args(kwargs)

    @property
    def value_type(self):
        return self._value_type

    @property
    def is_readonly(self):
        return self._is_readonly


# https://www.w3.org/TR/WebIDL-1/#idl-serializers
# BUG(736332): Remove support of legacy serializer.
# We support styles only used in production code. i.e.
# - serializer;
# - serializer = { attribute };
# - serializer = { inherit, attribute };
class Serializer(object):

    def __init__(self, **kwargs):
        self._is_map = kwargs.pop('is_map', False)
        self._has_attribute = kwargs.pop('has_attribute', False)
        self._has_inherit = kwargs.pop('has_inherit', False)
        assert_no_extra_args(kwargs)

        if (self.has_attribute or self.has_inherit) and not self._is_map:
            raise ValueError('has_attribute and has_inherit must be set with is_map')

    @property
    def is_map(self):
        return self._is_map

    @property
    def has_attribute(self):
        return self._has_attribute

    @property
    def has_inherit(self):
        return self._has_inherit
