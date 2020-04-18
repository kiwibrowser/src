# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from .extended_attribute import ExtendedAttributeList
from .idl_types import RecordType
from .idl_types import SequenceType
from .utilities import assert_no_extra_args


# https://heycam.github.io/webidl/#idl-attributes
class Attribute(object):
    _INVALID_TYPES = frozenset([SequenceType, RecordType])

    def __init__(self, **kwargs):
        self._identifier = kwargs.pop('identifier')
        self._type = kwargs.pop('type')
        self._is_static = kwargs.pop('is_static', False)
        self._is_readonly = kwargs.pop('is_readonly', False)
        self._extended_attribute_list = kwargs.pop('extended_attribute_list', ExtendedAttributeList())
        assert_no_extra_args(kwargs)

        if type(self.type) in Attribute._INVALID_TYPES:
            raise ValueError('The type of an attribute must not be either of sequence<T> and record<K,V>.')

    @property
    def identifier(self):
        return self._identifier

    @property
    def type(self):
        return self._type

    @property
    def is_static(self):
        return self._is_static

    @property
    def is_readonly(self):
        return self._is_readonly

    @property
    def extended_attribute_list(self):
        return self._extended_attribute_list
