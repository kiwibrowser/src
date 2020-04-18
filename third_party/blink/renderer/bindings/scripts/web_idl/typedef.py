# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from .utilities import assert_no_extra_args


# https://heycam.github.io/webidl/#idl-typedefs
class Typedef(object):

    def __init__(self, **kwargs):
        self._identifier = kwargs.pop('identifier')
        self._type = kwargs.pop('type')
        assert_no_extra_args(kwargs)

    @property
    def identifier(self):
        return self._identifier

    @property
    def type(self):
        return self._type
