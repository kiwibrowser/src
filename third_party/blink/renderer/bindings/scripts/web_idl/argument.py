# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from .extended_attribute import ExtendedAttributeList
from .utilities import assert_no_extra_args


class Argument(object):

    def __init__(self, **kwargs):
        self._identifier = kwargs.pop('identifier')
        self._type = kwargs.pop('type')
        self._is_optional = kwargs.pop('is_optional', False)
        self._is_variadic = kwargs.pop('is_variadic', False)
        self._default_value = kwargs.pop('default_value', None)
        self._extended_attribute_list = kwargs.pop('extended_attribute_list', ExtendedAttributeList())
        assert_no_extra_args(kwargs)

    @property
    def identifier(self):
        return self._identifier

    @property
    def type(self):
        return self._type

    @property
    def is_optional(self):
        return self._is_optional

    @property
    def is_variadic(self):
        return self._is_variadic

    @property
    def default_value(self):
        return self._default_value

    @property
    def extended_attribute_list(self):
        return self._extended_attribute_list
