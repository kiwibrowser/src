# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from .extended_attribute import ExtendedAttributeList
from .utilities import assert_no_extra_args


# https://heycam.github.io/webidl/#idl-enums
class Enumeration(object):

    def __init__(self, **kwargs):
        self._identifier = kwargs.pop('identifier')
        self._values = kwargs.pop('values', [])
        # Extended attributes on enumerations are not allowed in spec, but Blink uses them.
        self._extended_attribute_list = kwargs.pop('extended_attribute_list', ExtendedAttributeList())
        assert_no_extra_args(kwargs)

    @property
    def identifier(self):
        return self._identifier

    @property
    def values(self):
        return self._values

    @property
    def extended_attribute_list(self):
        return self._extended_attribute_list
