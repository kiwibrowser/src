# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from .extended_attribute import ExtendedAttributeList
from .idl_types import RecordType
from .idl_types import SequenceType
from .utilities import assert_no_extra_args


# https://heycam.github.io/webidl/#idl-constants
class Constant(object):
    _INVALID_IDENTIFIERS = frozenset(['length', 'name', 'prototype'])
    _INVALID_TYPES = frozenset([SequenceType, RecordType])

    def __init__(self, **kwargs):
        self._identifier = kwargs.pop('identifier')
        self._type = kwargs.pop('type')
        self._value = kwargs.pop('value')
        self._extended_attribute_list = kwargs.pop('extended_attribute_list', ExtendedAttributeList())
        assert_no_extra_args(kwargs)

        if self.identifier in Constant._INVALID_IDENTIFIERS:
            raise ValueError('Invalid identifier for a constant: %s' % self.identifier)
        if type(self.type) in Constant._INVALID_TYPES:
            raise ValueError('sequence<T> must not be used as the type of a constant.')

    @property
    def identifier(self):
        return self._identifier

    @property
    def type(self):
        return self._type

    @property
    def value(self):
        return self._value

    @property
    def extended_attribute_list(self):
        return self._extended_attribute_list
