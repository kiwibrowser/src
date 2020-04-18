# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from .utilities import assert_no_extra_args


class LiteralToken(object):
    """Literal class represents literal tokens in Web IDL. It appears
    - default values of dictionary members
    - default values of arguments in operations
    - constant values in interfaces (string and [] are not allowed)
    - arguments of some extended attributes
    """

    def __init__(self, **kwargs):
        self._type_name = kwargs.pop('type_name')
        self._value = kwargs.pop('value')
        assert_no_extra_args(kwargs)

    @property
    def type_name(self):
        return self._type_name

    @property
    def value(self):
        return self._value

NULL_TOKEN = LiteralToken(type_name='NULL', value='null')
