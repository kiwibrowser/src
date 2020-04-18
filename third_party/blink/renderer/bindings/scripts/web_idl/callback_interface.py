# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from .extended_attribute import ExtendedAttributeList
from .utilities import assert_no_extra_args


# https://heycam.github.io/webidl/#idl-interfaces
class CallbackInterface(object):

    def __init__(self, **kwargs):
        self._identifier = kwargs.pop('identifier')
        self._attributes = tuple(kwargs.pop('attributes', []))
        self._operations = tuple(kwargs.pop('operations', []))
        self._constants = tuple(kwargs.pop('constants', []))
        self._inherited_interface_name = kwargs.pop('inherited_interface_name', None)
        self._extended_attribute_list = kwargs.pop('extended_attribute_list', ExtendedAttributeList())
        assert_no_extra_args(kwargs)

        if any(attribute.is_static for attribute in self.attributes):
            raise ValueError('Static attributes must not be defined on a callback interface')
        if any(operation.is_static for operation in self.operations):
            raise ValueError('Static operations must not be defined on a callback interface')

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
    def inherited_interface_name(self):
        return self._inherited_interface_name

    @property
    def extended_attribute_list(self):
        return self._extended_attribute_list
