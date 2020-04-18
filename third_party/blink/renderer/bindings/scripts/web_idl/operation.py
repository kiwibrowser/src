# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from .extended_attribute import ExtendedAttributeList
from .utilities import assert_no_extra_args


# https://heycam.github.io/webidl/#idl-operations
class Operation(object):
    # https://www.w3.org/TR/WebIDL-1/#idl-special-operations
    _SPECIAL_KEYWORDS = frozenset(['deleter', 'getter', 'setter', 'stringifier', 'serializer'])

    def __init__(self, **kwargs):
        self._identifier = kwargs.pop('identifier')
        self._return_type = kwargs.pop('return_type')
        self._arguments = tuple(kwargs.pop('arguments', []))
        self._special_keywords = frozenset(kwargs.pop('special_keywords', []))
        self._is_static = kwargs.pop('is_static', False)
        self._extended_attribute_list = kwargs.pop('extended_attribute_list', ExtendedAttributeList())
        assert_no_extra_args(kwargs)

        if any(keyword not in Operation._SPECIAL_KEYWORDS for keyword in self._special_keywords):
            raise ValueError('Unknown keyword is specified in special keywords')

    @property
    def identifier(self):
        return self._identifier

    @property
    def return_type(self):
        return self._return_type

    @property
    def arguments(self):
        return self._arguments

    @property
    def special_keywords(self):
        return self._special_keywords

    @property
    def is_static(self):
        return self._is_static

    @property
    def extended_attribute_list(self):
        return self._extended_attribute_list

    @property
    def is_regular(self):
        return self.identifier and not self.is_static

    @property
    def is_special(self):
        return bool(self.special_keywords)
