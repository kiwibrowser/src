# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from .idl_types import TypeBase
from .utilities import assert_no_extra_args


class EcmaScriptType(TypeBase):
    """
    EcmascriptType represents an EcmaScript type, which appears in Chromium IDL files.
    @param string type_name   : the identifier of a named definition to refer
    @param bool   is_nullable : True if the type is nullable (optional)
    """
    _ALLOWED_TYPE_NAMES = frozenset(['Date'])

    def __init__(self, **kwargs):
        self._type_name = kwargs.pop('type_name')
        self._is_nullable = kwargs.pop('is_nullable', False)
        assert_no_extra_args(kwargs)

        # Now we use only 'Date' type.
        if self.type_name not in EcmaScriptType._ALLOWED_TYPE_NAMES:
            raise ValueError('Unknown type name: %s' % self.type_name)

    @property
    def type_name(self):
        return self._type_name

    @property
    def is_nullable(self):
        return self._is_nullable
