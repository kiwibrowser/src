# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from .extended_attribute import ExtendedAttributeList
from .utilities import assert_no_extra_args


# https://heycam.github.io/webidl/#idl-dictionaries
class Dictionary(object):

    def __init__(self, **kwargs):
        self._identifier = kwargs.pop('identifier')
        self._members = kwargs.pop('members', {})
        self._inherited_dictionary_name = kwargs.pop('inherited_dictionary_name', None)
        self._is_partial = kwargs.pop('is_partial', False)
        self._extended_attribute_list = kwargs.pop('extended_attribute_list', ExtendedAttributeList())
        assert_no_extra_args(kwargs)

    @property
    def identifier(self):
        return self._identifier

    @property
    def members(self):
        return self._members

    @property
    def inherited_dictionary_name(self):
        return self._inherited_dictionary_name

    @property
    def is_partial(self):
        return self._is_partial

    @property
    def extended_attribute_list(self):
        return self._extended_attribute_list


class DictionaryMember(object):

    def __init__(self, **kwargs):
        self._identifier = kwargs.pop('identifier')
        self._type = kwargs.pop('type')
        self._default_value = kwargs.pop('default_value', None)
        self._is_required = kwargs.pop('is_required', False)
        self._extended_attribute_list = kwargs.pop('extended_attribute_list', ExtendedAttributeList())
        assert_no_extra_args(kwargs)

    @property
    def identifier(self):
        return self._identifier

    @property
    def type(self):
        return self._type

    @property
    def default_value(self):
        return self._default_value

    @property
    def is_required(self):
        return self._is_required

    @property
    def extended_attribute_list(self):
        return self._extended_attribute_list
